#include "event_action.hh"
#include "run_action.hh"

#include "io/hdf5.hh"
#include "nain4.hh"

#include <G4Event.hh>
#include <G4Run.hh>
#include <G4RunManager.hh>


event_action::event_action(run_action* runAction)
: G4UserEventAction()
, action(runAction)
, data{} {}

void event_action::EndOfEventAction(const G4Event*) {
  // accumulate statistics in run action
  std::cout << "event id: " << nain4::event_number() << std::endl;

  for (auto hit: data.get_hits()) {
    auto pos  = hit.GetPreStepPoint() -> GetPosition();
    auto time = hit.GetPreStepPoint() -> GetGlobalTime();
    std::cout << "time: " << time << std::endl;
  }
}
