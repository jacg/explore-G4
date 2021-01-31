#ifndef detector_construction_h
#define detector_construction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class to define materials and geometry.

class detector_construction : public G4VUserDetectorConstruction {
public:
  detector_construction() : G4VUserDetectorConstruction(), fScoringVolume(nullptr) {}
  virtual ~detector_construction() override {}
  virtual G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

protected:
  // Who owns this? I can't see where it's deleted!
  G4LogicalVolume* fScoringVolume;
};

#endif
