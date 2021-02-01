#ifndef detector_construction_hh
#define detector_construction_hh 1

#include <G4VUserDetectorConstruction.hh>
#include <globals.hh>

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class to define materials and geometry.

class detector_construction : public G4VUserDetectorConstruction {
public:
  detector_construction() : G4VUserDetectorConstruction(), scoring_volume(nullptr) {}
  virtual ~detector_construction() override {}
  virtual G4VPhysicalVolume* Construct() override;
  G4LogicalVolume* GetScoringVolume() const { return scoring_volume; }

protected:
  // Who owns this? I can't see where it's deleted!
  G4LogicalVolume* scoring_volume;
};

#endif
