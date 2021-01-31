#ifndef B1DetectorConstruction_h
#define B1DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class to define materials and geometry.

class B1DetectorConstruction : public G4VUserDetectorConstruction {
public:
  B1DetectorConstruction() : G4VUserDetectorConstruction(), fScoringVolume(nullptr) {}
  virtual ~B1DetectorConstruction() override {}
  virtual G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

protected:
  // Who owns this? I can't see where it's deleted!
  G4LogicalVolume* fScoringVolume;
};

#endif
