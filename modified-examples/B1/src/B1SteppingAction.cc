#include "B1SteppingAction.hh"
#include "event_action.hh"
#include "detector_construction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

B1SteppingAction::B1SteppingAction(event_action* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  scoring_volume(nullptr)
{}


void B1SteppingAction::UserSteppingAction(const G4Step* step) {
  if (!scoring_volume) {
    const detector_construction* detectorConstruction
      = static_cast<const detector_construction*>
      (G4RunManager::GetRunManager() -> GetUserDetectorConstruction());
    scoring_volume = detectorConstruction -> GetScoringVolume();
  }

  // get volume of the current step
  G4LogicalVolume* volume =
    step
    -> GetPreStepPoint()
    -> GetTouchableHandle()
    -> GetVolume()
    -> GetLogicalVolume();

  if (volume == scoring_volume) {
    // collect energy deposited in this step
    fEventAction -> AddEdep(step -> GetTotalEnergyDeposit());
  }
}
