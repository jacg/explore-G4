#include "g4-mandatory.hh"

#include "nain4.hh"

#include <G4Run.hh>
#include <G4RunManager.hh>

namespace nain4 {

// ----- run_action -----------------------------------------------------------------
run_action::run_action() : G4UserRunAction{} {}

void run_action::BeginOfRunAction(const G4Run*) {
  G4RunManager::GetRunManager() -> SetRandomNumberStore(false);
}

void run_action::EndOfRunAction(const G4Run* run) {
  G4int nofEvents = run -> GetNumberOfEvent();
  if (nofEvents == 0) return;
}

// ----- event_action ---------------------------------------------------------------
event_action::event_action(run_action* runAction)
: G4UserEventAction()
, action(runAction) {}

void event_action::EndOfEventAction(const G4Event* event) {
  auto evt_data = dynamic_cast<event_data*>(event -> GetUserInformation());
  if (!evt_data) { throw "Failed to get event data"; }
  for (auto hit: evt_data->get_hits()) {
    auto pos  = hit.GetPreStepPoint() -> GetPosition();
    auto time = hit.GetPreStepPoint() -> GetGlobalTime();
    //TODO: Decide whether to write here or in sensitive detector
  }
}

// ----- stepping_action ------------------------------------------------------------
stepping_action::stepping_action(event_action* action)
: G4UserSteppingAction()
, action(action)
{}

// ----- actions --------------------------------------------------------------------
void actions::Build() const {
  // Utility which sets the action, but also returns it, so that it can be
  // passed on to the constructor of another action. This allows us to write the
  // code much more clearly, by avoiding naming the objects, and showing their
  // dependence explicitly by nesting.
  // TODO: maybe provide this via a nain4 subclass of G4VUserActionInitialization
  auto set_and_return = [this](auto action) {
    SetUserAction(action);
    return action;
  };
  SetUserAction(generator);

  set_and_return(new stepping_action {
      set_and_return(new event_action {
          set_and_return(new run_action)})});
}

// ----- primary generator -----------------------------------------------------------
generator::generator() {
  gun.SetParticleDefinition(nain4::find_particle("geantino"));
  gun.SetParticleMomentumDirection({0, 0, 1});
}

// TODO: not really appropriate as the default generator
void generator::GeneratePrimaries(G4Event* event) {
  for (int x = -35; x < 35; x += 7) {
    for (int y = -35; y < 35; y += 7) {
      gun.SetParticlePosition({x * CLHEP::mm, y * CLHEP::mm, 0 * CLHEP::mm});
      gun.GeneratePrimaryVertex(event);
    }
  }
}

} // namespace nain4
