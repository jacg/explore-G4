#include "nain4.hh"
#include "g4-mandatory.hh"

#include "geometries/imas.hh"
#include "geometries/samples.hh"
#include "geometries/sipm.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
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
    {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial)};

  // Set mandatory initialization classes

  // run_manager takes ownership of geometry
  run_manager -> SetUserInitialization(new n4::geometry{[]() -> G4VPhysicalVolume* {
    // Pick one ...
    return a_nema_phantom();
    return cylinder_lined_with_hamamatsus(30*mm, 70*mm);
    return imas_demonstrator(nullptr);
    return square_array_of_sipms();
    return nain4::place(sipm_hamamatsu_blue(true, nullptr)).now();
  }});

  { // Physics list
    auto verbosity = 1;
    auto physics_list = new QBBC(verbosity); // SetVerboseLevel method also exists
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager -> SetUserInitialization(new n4::actions{new n4::generator});

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
