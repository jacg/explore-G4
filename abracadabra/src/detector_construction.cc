#include "detector_construction.hh"

#include <G4Box.hh>
#include <G4Cons.hh>
#include <G4LogicalVolume.hh>
#include <G4NistManager.hh>
#include <G4Orb.hh>
#include <G4PVPlacement.hh>
#include <G4RunManager.hh>
#include <G4Sphere.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4Trd.hh>

// Create logical volume from solid and material
G4LogicalVolume* logical(G4Material* material, G4VSolid* solid) {
  return new G4LogicalVolume{solid, material, solid->GetName()};
}

G4VPhysicalVolume* detector_construction::Construct() {
  // Lookup-by-name of materials from NIST database
  G4NistManager* nist = G4NistManager::Instance();

  auto material = [nist](auto const& name) { return nist->FindOrBuildMaterial(name); };

  auto place = [](G4ThreeVector position, G4LogicalVolume* logical, G4LogicalVolume* parent = nullptr) {
    auto name           = logical -> GetName();
    bool bool_op        = false;
    bool check_overlaps = true;
    return new G4PVPlacement{nullptr, position, logical, name, parent, bool_op, check_overlaps};
  };

  // ----- Materials --------------------------------------------------------------
  auto air    = material("G4_AIR");
  auto water  = material("G4_WATER");
  auto tissue = material("G4_A-150_TISSUE");
  auto bone   = material("G4_BONE_COMPACT_ICRU");

  // ----- Dimensions -------------------------------------------------------------
  // Size of the detector
  G4double length_xy = 20 * cm;
  G4double length_z  = 30 * cm;

  // G4Box requires half-lengths
  G4double e_xy = 0.5 * length_xy;
  G4double e_z  = 0.5 * length_z;

  // World volume needs a margin around everything inside it
  G4double w_xy = 1.2 * e_xy;
  G4double w_z  = 1.2 * e_z;

  // Trapezoid ---------------------------------------------------------------------
  G4double t_dxa = 12 * cm / 2, t_dxb = 12 * cm / 2;
  G4double t_dya = 10 * cm / 2, t_dyb = 16 * cm / 2;
  G4double t_dz  =  6 * cm / 2;

  // Cone --------------------------------------------------------------------------
  G4double c_rmin_a  = 0 * cm,   c_rmax_a = 2 * cm;
  G4double c_rmin_b  = 0 * cm,   c_rmax_b = 4 * cm;
  G4double c_hz      = 3 * cm;
  G4double c_phi_min = 0 * deg,  c_phi_max = 360 * deg;

  // ----- Create the shapes -------------------------------------------------------
  auto world     = logical(air   , new G4Box{"World"   , w_xy, w_xy, w_z});
  auto envelope  = logical(water , new G4Box{"Envelope", e_xy, e_xy, e_z});
  auto trapezoid = logical(bone  , new G4Trd{"BoneTrapezoid", t_dxa, t_dxb, t_dya, t_dyb, t_dz});
  auto cone      = logical(tissue, new G4Cons{"TissueCone",
                                              c_rmin_a, c_rmax_a, c_rmin_b, c_rmax_b,
                                              c_hz, c_phi_min, c_phi_max});

  // ----- Place the shapes at specific points in space ----------------------------
  auto root = place({              }, world    , nullptr);
  /*  */      place({              }, envelope , world);
  /*  */      place({0, -1*cm, 7*cm}, trapezoid, envelope);
  /*  */      place({0,  2*cm,-7*cm}, cone     , envelope);

  // --------------------------------------------------------------------------------

  // Set Shape2 as scoring volume
  this -> scoring_volume = trapezoid;

  //always return the physical World
  return root;
}
