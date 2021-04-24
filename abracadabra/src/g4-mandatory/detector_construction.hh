#ifndef detector_construction_hh
#define detector_construction_hh 1

#include <G4VUserDetectorConstruction.hh>
#include <globals.hh>

class G4VPhysicalVolume;

/// Detector construction class to define materials and geometry.

class detector_construction : public G4VUserDetectorConstruction {
public:
  detector_construction() : G4VUserDetectorConstruction{} {}
  G4VPhysicalVolume* Construct() override;
};

#endif
