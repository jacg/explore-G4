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

void event_action::EndOfEventAction(const G4Event* event) {
  return; // TODO XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  auto evt_data = dynamic_cast<event_data*>(event -> GetUserInformation());
  if (!evt_data) { throw "Failed to get event data"; }
  for (auto hit: evt_data->get_hits()) {
    auto pos  = hit.GetPreStepPoint() -> GetPosition();
    auto time = hit.GetPreStepPoint() -> GetGlobalTime();
  }
}

// ----- actions --------------------------------------------------------------------
void actions::Build() const {
  SetUserAction(generator);
  SetUserAction(new stepping_action);
  SetUserAction(new    event_action);
  SetUserAction(new      run_action);
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
