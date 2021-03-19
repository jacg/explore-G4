#include "g4-mandatory/action_initialization.hh"
#include "g4-mandatory/detector_construction.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>

#include <QBBC.hh>
#include <Randomize.hh>

#include <memory>

using std::make_unique;
using std::unique_ptr;

int main(int argc, char** argv) {
  // Detect interactive mode (if no arguments) and define UI session
  auto ui = argc == 1
    ? make_unique<G4UIExecutive>(argc, argv)
    : unique_ptr <G4UIExecutive>{nullptr};

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  // Construct the default run manager
  auto run_manager = unique_ptr<G4RunManager>
    {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default)};

  // Set mandatory initialization classes

  // run_manager takes ownership of detector_construction
  run_manager -> SetUserInitialization(new detector_construction{});

  { // Physics list
    auto verbosity = 1;
    auto physics_list = new QBBC(verbosity); // SetVerboseLevel method also exists
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager -> SetUserInitialization(new action_initialization{});

  // Initialize visualization
  auto vis_manager = make_unique<G4VisExecutive>();
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // G4VisManager* visManager = new G4VisExecutive{"Quiet"};
  vis_manager -> Initialize();

  // Get the pointer to the User Interface manager
  auto ui_manager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  if (!ui) {
    // batch mode
    G4String command = "/control/execute ";
    G4String file_name = argv[1];
    ui_manager -> ApplyCommand(command + file_name);
  } else {
    // interactive mode
    ui_manager -> ApplyCommand("/control/execute init_vis.mac");
    ui         -> SessionStart();
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !
}
