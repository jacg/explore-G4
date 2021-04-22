#include "event_action.hh"
#include "run_action.hh"

#include "io/hdf5.hh"
#include "nain4.hh"

#include <G4Event.hh>
#include <G4Run.hh>
#include <G4RunManager.hh>


event_action::event_action(run_action* runAction)
: G4UserEventAction()
, action(runAction) {}

void event_action::EndOfEventAction(const G4Event* event) {
  auto evt_data = dynamic_cast<event_data*>(event -> GetUserInformation());

  for (auto hit: evt_data->get_hits()) {
    auto pos  = hit.GetPreStepPoint() -> GetPosition();
    auto time = hit.GetPreStepPoint() -> GetGlobalTime();
    //TODO: Decide whether to write here or in sensitive detector
  }
}
