#include "B1ActionInitialization.hh"
#include "primary_generator_action.hh"
#include "run_action.hh"
#include "event_action.hh"
#include "stepping_action.hh"

B1ActionInitialization::B1ActionInitialization() : G4VUserActionInitialization() {}


B1ActionInitialization::~B1ActionInitialization() {}


// See README for explanation of the role of this method in multi-threaded mode.
void B1ActionInitialization::BuildForMaster() const {
  SetUserAction(new run_action);
}


void B1ActionInitialization::Build() const {
  SetUserAction(new primary_generator_action);

  auto run_action_ = new run_action;

  SetUserAction                        (run_action_);
  auto event_action_ = new event_action{run_action_};

  SetUserAction                    (event_action_);
  SetUserAction(new stepping_action{event_action_});
}
