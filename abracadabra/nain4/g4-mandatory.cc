#include "g4-mandatory.hh"

#include "nain4.hh"

#include <G4Run.hh>
#include <G4RunManager.hh>

namespace nain4 {

// ----- run_action -----------------------------------------------------------------
// TODO this is garbage at the moment
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
  return; // TODO XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
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
void generator::geantino_along_x(G4Event* event) {
  auto geantino  = nain4::find_particle("geantino");
  auto p         = 1 * CLHEP::MeV;
  auto vertex    = new G4PrimaryVertex();
  vertex->SetPrimary(new G4PrimaryParticle(geantino, p, 0, 0));
  event->AddPrimaryVertex(vertex);
}

} // namespace nain4
