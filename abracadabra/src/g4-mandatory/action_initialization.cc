#include "action_initialization.hh"
#include "event_action.hh"
#include "primary_generator_action.hh"
#include "run_action.hh"
#include "stepping_action.hh"
#include "tracking_action.hh"

// See README for explanation of the role of this method in multi-threaded mode.
void action_initialization::BuildForMaster() const { SetUserAction(new run_action); }

void action_initialization::Build() const {

  // Utility which sets the action, but also returns it, so that it can be
  // passed on to the constructor of another action. This allows us to write the
  // code much more clearly, by avoiding naming the objects, and showing their
  // dependence explicitly by nesting.
  // TODO: maybe provide this via a nain4 subclass of G4VUserActionInitialization
  auto set_and_return = [this](auto action) {
    SetUserAction(action);
    return action;
  };

  // Use the above to set the actions
  set_and_return(new primary_generator_action);

  set_and_return(new tracking_action {
      set_and_return(new stepping_action {
          set_and_return(new event_action {
              set_and_return(new run_action)})})});
}
