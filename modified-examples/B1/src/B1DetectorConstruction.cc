#include "B1DetectorConstruction.hh"

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


B1DetectorConstruction::B1DetectorConstruction()
: G4VUserDetectorConstruction()
, fScoringVolume(nullptr)
{ }

// Create logical volume from solid and material
G4LogicalVolume* RENAME_ME(G4VSolid* solid, G4Material* material) {
  return new G4LogicalVolume(solid, material, solid->GetName());
}

G4VPhysicalVolume* B1DetectorConstruction::Construct() {
  // Lookup-by-name of materials from NIST database
  G4NistManager* nist = G4NistManager::Instance();

  auto material = [nist] (auto const& name) { return nist->FindOrBuildMaterial(name); };

  auto place = [] (G4ThreeVector position, G4LogicalVolume* logical, G4LogicalVolume* parent) {
    auto name = logical -> GetName();
    bool bool_op = false;
    bool check_overlaps = true;
    return new G4PVPlacement(0, position, logical, name, parent, bool_op, check_overlaps);
  };

  // Envelope parameters
  G4double env_sizeXY = 20 * cm;
  G4double env_sizeZ  = 30 * cm;

  // World ----------------------------------------------------------------------
  G4double world_sizeXY = 1.2 * env_sizeXY;
  G4double world_sizeZ  = 1.2 * env_sizeZ;

  auto logicWorld = RENAME_ME
    (new G4Box("World",
               0.5 * world_sizeXY,
               0.5 * world_sizeXY,
               0.5 * world_sizeZ  ),
     material("G4_AIR"));

  G4VPhysicalVolume* physWorld = place({}, logicWorld, nullptr);

  // Envelope ----------------------------------------------------------------------
  auto logicEnv = RENAME_ME
    (new G4Box("Envelope",
               0.5 * env_sizeXY,
               0.5 * env_sizeXY,
               0.5 * env_sizeZ),
     material("G4_WATER"));

  place({}, logicEnv, logicWorld);

  // Shape 1 ----------------------------------------------------------------------

  // Conical section shape
  G4double shape1_rmina = 0 * cm,    shape1_rmaxa = 2 * cm;
  G4double shape1_rminb = 0 * cm,    shape1_rmaxb = 4 * cm;
  G4double shape1_hz    = 3 * cm;
  G4double shape1_phimin = 0 * deg, shape1_phimax = 360 * deg;

  auto logicShape1 = RENAME_ME
    (new G4Cons("TissueCone", shape1_rmina, shape1_rmaxa, shape1_rminb,
                shape1_rmaxb, shape1_hz, shape1_phimin,
                shape1_phimax),
     material("G4_A-150_TISSUE"));

  place({0, 2*cm, -7*cm}, logicShape1, logicEnv);

  // Shape 2 ----------------------------------------------------------------------

  // Trapezoid shape
  G4double shape2_dxa = 12*cm, shape2_dxb = 12*cm;
  G4double shape2_dya = 10*cm, shape2_dyb = 16*cm;
  G4double shape2_dz  = 6*cm;

  auto logicShape2 = RENAME_ME
    (new G4Trd("BoneTrapezoid", // its name
               0.5 * shape2_dxa, 0.5 * shape2_dxb, 0.5 * shape2_dya,
               0.5 * shape2_dyb, 0.5 * shape2_dz),
     material("G4_BONE_COMPACT_ICRU"));

  place({0, -1*cm, 7*cm}, logicShape2, logicEnv);

  // --------------------------------------------------------------------------------

  // Set Shape2 as scoring volume
  this -> fScoringVolume = logicShape2;

  //always return the physical World
  return physWorld;
}
