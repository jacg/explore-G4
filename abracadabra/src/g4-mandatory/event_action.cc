#include "event_action.hh"
#include "run_action.hh"

#include <G4Event.hh>
#include <G4RunManager.hh>

event_action::event_action(run_action* runAction)
: G4UserEventAction()
, action(runAction)
, edep(0) {}

void event_action::BeginOfEventAction(const G4Event*) { edep = 0; }

void event_action::EndOfEventAction(const G4Event*) {
  // accumulate statistics in run action
  action->AddEdep(edep);
}
