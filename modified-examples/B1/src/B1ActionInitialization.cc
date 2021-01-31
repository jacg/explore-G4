#include "B1ActionInitialization.hh"
#include "B1PrimaryGeneratorAction.hh"
#include "run_action.hh"
#include "event_action.hh"
#include "B1SteppingAction.hh"

B1ActionInitialization::B1ActionInitialization() : G4VUserActionInitialization() {}


B1ActionInitialization::~B1ActionInitialization() {}


// See README for explanation of the role of this method in multi-threaded mode.
void B1ActionInitialization::BuildForMaster() const {
  SetUserAction(new run_action);
}


void B1ActionInitialization::Build() const {
  SetUserAction(new B1PrimaryGeneratorAction);

  auto action = new run_action;

  SetUserAction                      (action);
  auto eventAction = new event_action(action);

  SetUserAction                     (eventAction);
  SetUserAction(new B1SteppingAction(eventAction));
}
