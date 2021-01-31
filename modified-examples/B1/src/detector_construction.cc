#include "detector_construction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Orb.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include <G4ThreeVector.hh>


// Create logical volume from solid and material
G4LogicalVolume* logical(G4VSolid* solid, G4Material* material) {
  return new G4LogicalVolume(solid, material, solid->GetName());
}

G4VPhysicalVolume* detector_construction::Construct() {
  // Lookup-by-name of materials from NIST database
  G4NistManager* nist = G4NistManager::Instance();

  auto material = [nist] (auto const& name) { return nist->FindOrBuildMaterial(name); };

  auto place = [] (G4ThreeVector position, G4LogicalVolume* logical, G4LogicalVolume* parent=nullptr) {
    auto name = logical -> GetName();
    bool bool_op = false;
    bool check_overlaps = true;
    return new G4PVPlacement(nullptr, position, logical, name, parent, bool_op, check_overlaps);
  };

  // Envelope parameters
  G4double env_sizeXY = 20 * cm;
  G4double env_sizeZ  = 30 * cm;

  // World ----------------------------------------------------------------------
  G4double world_sizeXY = 1.2 * env_sizeXY;
  G4double world_sizeZ  = 1.2 * env_sizeZ;

  auto logicWorld = logical
    (new G4Box("World",
               0.5 * world_sizeXY,
               0.5 * world_sizeXY,
               0.5 * world_sizeZ  ),
     material("G4_AIR"));

  G4VPhysicalVolume* physWorld = place({}, logicWorld);

  // Envelope ----------------------------------------------------------------------
  auto logicEnv = logical
    (new G4Box("Envelope",
               0.5 * env_sizeXY,
               0.5 * env_sizeXY,
               0.5 * env_sizeZ),
     material("G4_WATER"));

  place({}, logicEnv, logicWorld);

  // Cone --------------------------------------------------------------------------
  G4double cone_rmina = 0 * cm,    cone_rmaxa = 2 * cm;
  G4double cone_rminb = 0 * cm,    cone_rmaxb = 4 * cm;
  G4double cone_hz    = 3 * cm;
  G4double cone_phimin = 0 * deg, cone_phimax = 360 * deg;

  place({0, 2*cm, -7*cm},
        logical(new G4Cons("TissueCone",
                           cone_rmina, cone_rmaxa,
                           cone_rminb, cone_rmaxb,
                           cone_hz, cone_phimin, cone_phimax),
                material("G4_A-150_TISSUE")),
        logicEnv);

  // Trapezoid -------------------------------------------------------------------
  G4double trapezoid_dxa = 12*cm, trapezoid_dxb = 12*cm;
  G4double trapezoid_dya = 10*cm, trapezoid_dyb = 16*cm;
  G4double trapezoid_dz  = 6*cm;

  auto trapezoid = logical
    (new G4Trd("BoneTrapezoid",
               0.5 * trapezoid_dxa, 0.5 * trapezoid_dxb, 0.5 * trapezoid_dya,
               0.5 * trapezoid_dyb, 0.5 * trapezoid_dz),
     material("G4_BONE_COMPACT_ICRU"));

  place({0, -1*cm, 7*cm}, trapezoid, logicEnv);

  // --------------------------------------------------------------------------------

  // Set Shape2 as scoring volume
  this -> fScoringVolume = trapezoid;

  //always return the physical World
  return physWorld;
}
