#include "B1EventAction.hh"
#include "B1RunAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"


B1EventAction::B1EventAction(B1RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fEdep(0)
{}


void B1EventAction::BeginOfEventAction(const G4Event*) { fEdep = 0; }

void B1EventAction::EndOfEventAction(const G4Event*) {
  // accumulate statistics in run action
  fRunAction->AddEdep(fEdep);
}
