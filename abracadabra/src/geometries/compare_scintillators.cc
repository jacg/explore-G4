#include "geometries/compare_scintillators.hh"
#include "materials/LXe.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4SystemOfUnits.hh>
#include <G4Tubs.hh>

using nain4::material;
using nain4::place;
using nain4::volume;

using CLHEP::twopi;


G4PVPlacement* compare_scintillators(
    G4String sci_name,
    G4double length,
    G4double scintillator_inner_radius,
    G4double dr_scintillator
) {
  auto air     = material("G4_AIR");
  auto steel   = material("G4_STAINLESS-STEEL");
  auto vacuum  = material("G4_Galactic");
  auto scintillator =
    (sci_name == "LXe" ) ? LXe_with_properties() :
    (sci_name == "LYSO") ?  LYSO_no_properties() :
    (FATAL(("Unrecoginzed scintillator: " + sci_name).c_str()), nullptr);

  G4double dr_steel_0 = 1.5 * mm;
  G4double dr_vacuum = 25.0 * mm;
  G4double dr_steel_1 = 1.5 * mm;
  if (sci_name != "LXe") {
    dr_steel_0 = dr_vacuum = dr_steel_1 = 0;
  }

  G4double cavity_radius = scintillator_inner_radius - (dr_steel_0 + dr_vacuum + dr_steel_1);

  // ----- Utility for wrapping smaller cylinder inside a larger one --------------
  G4LogicalVolume*   log_out = nullptr; // Current outermost logical volume
  G4VPhysicalVolume* phy_prv = nullptr; // Physical volume directly inside log_out
  auto radius = 0.0;
  auto layer = [=, &radius, &log_out, &phy_prv](auto& name, auto material, auto dr) {
      if (dr == 0) return;
      radius += dr;
      auto log_new = volume<G4Tubs>(name, material, 0.0, radius, length/2, 0.0, twopi);
      if (log_out) { phy_prv = place(log_out).in(log_new).now(); }
      log_out = log_new;
  };

  // ----- Build geometry by adding concentric cylinders of increasing radius -----
  layer("Cavity"      , air         , cavity_radius);
  layer("Steel_0"     , steel       , dr_steel_0);
  layer("Inner_vacuum", vacuum      , dr_vacuum );
  layer("Steel_1"     , steel       , dr_steel_1);
  layer(sci_name      , scintillator, dr_scintillator);

  auto env_length = 1.1 * length / 2;
  auto env_width  = 1.1 * radius;

  auto vol_envelope = volume<G4Box>("Envelope", air, env_width, env_width, env_length);
  place(log_out).in(vol_envelope).now();
  return place(vol_envelope).now();

}
