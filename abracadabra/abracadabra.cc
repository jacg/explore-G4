#include "nain4.hh"
#include "g4-mandatory.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
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
#include <string>

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
    return cylinder_lined_with_hamamatsus(70*mm, 70*mm);
    return imas_demonstrator(nullptr);
    return a_nema_phantom();
    return square_array_of_sipms();
    return nain4::place(sipm_hamamatsu_blue(true, nullptr)).now();
  }});

  { // Physics list
    auto verbosity = 1;
    auto physics_list = new QBBC(verbosity); // SetVerboseLevel method also exists
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager->SetUserInitialization(new n4::actions{
      new n4::generator{[](G4Event* event) { generate_back_to_back_511_keV_gammas(event, {}, 0); }}});

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
    {
      ui_manager -> ApplyCommand("/control/execute init_vis.mac");
      ui_manager -> ApplyCommand("/run/beamOn 10");
      //nain4::silence _{G4cout};
      int PHI = 150; int THETA = 160;
      auto view = [&ui_manager](auto theta, auto phi) {
        ui_manager->ApplyCommand("/vis/viewer/set/viewpointThetaPhi "
                                 + std::to_string(theta) + ' ' + std::to_string(phi));
      };
      {
        nain4::silence _{G4cout};
        for (int phi  =PHI  ; phi  <360+PHI  ; ++phi  ) { view(THETA, phi); }
        for (int theta=THETA; theta<360+THETA; ++theta) { view(theta, PHI); }
      }
      ui -> SessionStart();
    }
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !
}
