#include "geometries/imas.hh"
#include "geometries/sipm.hh"
#include "materials/LXe.hh"

#include "io/hdf5.hh"
#include "nain4.hh"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4RotationMatrix.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4Types.hh>
#include <G4SystemOfUnits.hh>

#include <initializer_list>
#include <tuple>
#include <optional>

using CLHEP::pi;
using CLHEP::twopi;
using nain4::material;
using nain4::place;
using nain4::volume;
using std::make_tuple;
using std::optional;

G4PVPlacement* imas_demonstrator(n4::sensitive_detector* sd,
                                 G4double length,
                                 G4double drQtz,
                                 G4double drLXe,
                                 bool vacuum_before_xenon) {
  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");
  auto steel   = material("G4_STAINLESS-STEEL");
  auto vacuum  = material("G4_Galactic");
  auto Quartz  = quartz_with_properties();
  auto LXe     = LXe_with_properties();

  // For trials where we want a cleaner signal in the Xenon
  if (vacuum_before_xenon) {
    steel = vacuum;
    air   = vacuum;
  }

  // ----- Utility for wrapping smaller cylinder inside a larger one --------------
  G4LogicalVolume*   log_out = nullptr; // Current outermost logical volume
  G4VPhysicalVolume* phy_prv = nullptr; // Physical volume directly inside log_out
  auto radius = 0.0;
  auto layer = [=, &radius, &log_out, &phy_prv](auto& name, auto material, auto dr) {
    if (name == "Quartz" && drQtz == 0) { return; }
    radius += dr;
    auto log_new = volume<G4Tubs>(name, material, 0.0, radius, length/2, 0.0, twopi);
    if (log_out) { phy_prv = place(log_out).in(log_new).now(); }
    log_out = log_new;
  };

  // ----- Build geometry by adding concentric cylinders of increasing radius -----
  layer("Cavity"      , air   , 325   * mm);
  layer("Steel_0"     , steel ,   1.5 * mm);
  layer("Inner_vacuum", vacuum,  25   * mm);
  layer("Steel_1"     , steel ,   1.5 * mm);
  layer("LXe"         , LXe   , drLXe * mm); auto xenon_l = log_out;
  layer("Quartz"      , Quartz, drQtz * mm); auto quartz  = log_out;
  layer("Outer_vacuum", vacuum, 200   * mm);
  layer("Steel_2"     , steel ,   5   * mm);

  // Helper for placing sensors in different layers according to detector design version
  auto place_sipms_in = [&sd](auto layer, optional<G4double> radius = {}) {
    line_cylinder_with_tiles(layer, sipm_hamamatsu_blue(true, sd), 1 * mm, radius);
  };

  // If Quartz layer missing, place sipms directly in LXe
  if (drQtz > 0) { place_sipms_in(quartz ); }
  else           { place_sipms_in(xenon_l); }

  auto env_length = 1.1 * length / 2;
  auto env_width  = 1.1 * radius;

  auto vol_envelope = volume<G4Box>("Envelope", air, env_width, env_width, env_length);
  place(log_out).in(vol_envelope).now();
  return place(vol_envelope).now();
}

auto box_dimensions(G4LogicalVolume* tile) {
  auto box = dynamic_cast<G4Box*>(tile -> GetSolid());
  if (!tile) { throw "SiPM should be a G4Box"; }
  auto dx = box -> GetXHalfLength() * 2;
  auto dy = box -> GetYHalfLength() * 2;
  auto dz = box -> GetZHalfLength() * 2;
  return make_tuple(dx, dy, dz);
}

auto axial_positioning(double gap, double length, double dx) {
  auto pitch    = dx + gap;
  auto N        = floor((length - gap) / pitch);
  auto z_margin = (length - (N * pitch) + gap) / 2;
  auto first_z = -(length - dx) / 2 + z_margin;
  struct z { size_t N; double first; double pitch; };
  return z{(size_t)N, first_z, pitch};
}

auto angular_positioning(double outer_r, double dz, double pitch) {
  auto reduced_r = outer_r - dz;
  auto circumference = twopi * reduced_r;
  auto N_phi = (size_t)floor(circumference / pitch);
  auto d_phi = twopi / N_phi;
  struct phi { double r; size_t N; double delta; };
  return phi{reduced_r, N_phi, d_phi};
}

// TODO: this is not adequately tested
void line_cylinder_with_tiles(G4LogicalVolume* cylinder, G4LogicalVolume* tile, G4double gap, optional<G4double> r) {

  // Cylinder dimensions
  auto tub = dynamic_cast<G4Tubs*>(cylinder -> GetSolid());
  if (!tub) { throw "Cylinder should be a G4Tubs"; }
  auto length  = tub -> GetDz() * 2;
  auto outer_r = r.value_or(tub->GetRMax());

  // Tile dimensions
  auto [dx, dy, dz] = box_dimensions(tile);
  if (dx != dy) { throw "The tile should have equal x and y dimensions"; }

  // Tile positions
  auto z   =   axial_positioning(gap, length, dx);
  auto phi = angular_positioning(outer_r, dz, z.pitch);

  //return;
  G4ThreeVector x_axis{1, 0, 0};
  G4ThreeVector z_axis{0, 0, 1};
  G4RotationMatrix around_x_axis_90_degrees{x_axis, -pi / 2};
  auto const situate_sipm = place(tile).in(cylinder)
    .rotate(around_x_axis_90_degrees) // active region facing inwards
    .at(0, phi.r, 0);                 // set distance now; rotation in phi comes later

  size_t N_copy = 0;
  for (size_t n_phi = 0; n_phi<phi.N; ++n_phi) {
    G4RotationMatrix around_z_axis_phi{z_axis, n_phi*phi.delta};
    for (size_t nz=0; nz<z.N; ++nz) {
      auto zz = z.first + nz * z.pitch;
      situate_sipm.clone()
        .at(0, 0, zz)
        .rotate(around_z_axis_phi)
        .copy_no(N_copy)
        .now();
      ++N_copy;
    }
  }
}
