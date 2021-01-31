#include "B1SteppingAction.hh"
#include "B1EventAction.hh"
#include "detector_construction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

B1SteppingAction::B1SteppingAction(B1EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fScoringVolume(nullptr)
{}


void B1SteppingAction::UserSteppingAction(const G4Step* step) {
  if (!fScoringVolume) {
    const detector_construction* detectorConstruction
      = static_cast<const detector_construction*>
      (G4RunManager::GetRunManager() -> GetUserDetectorConstruction());
    fScoringVolume = detectorConstruction -> GetScoringVolume();
  }

  // get volume of the current step
  G4LogicalVolume* volume =
    step
    -> GetPreStepPoint()
    -> GetTouchableHandle()
    -> GetVolume()
    -> GetLogicalVolume();

  if (volume == fScoringVolume) {
    // collect energy deposited in this step
    fEventAction -> AddEdep(step -> GetTotalEnergyDeposit());
  }
}
