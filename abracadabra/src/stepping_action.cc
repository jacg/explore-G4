#include "detector_construction.hh"
#include "event_action.hh"
#include "stepping_action.hh"

#include <G4Event.hh>
#include <G4LogicalVolume.hh>
#include <G4RunManager.hh>
#include <G4Step.hh>

stepping_action::stepping_action(event_action* action)
: G4UserSteppingAction()
, action(action)
, scoring_volume(nullptr) {}

void stepping_action::UserSteppingAction(const G4Step* step) {
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
    action -> AddEdep(step -> GetTotalEnergyDeposit());
  }
}
