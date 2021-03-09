#include "action_initialization.hh"
#include "detector_construction.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisManager.hh>

#include <QBBC.hh>
#include <Randomize.hh>

#include <memory>

using std::make_unique;
using std::unique_ptr;

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main(int argc, char** argv) {

  // ----- Pre-testing setup: G4 boilerplate -------------------------------

  // Detect interactive mode (if no arguments) and define UI session
  auto ui = argc == 1
    ? make_unique<G4UIExecutive>(argc, argv)
    : unique_ptr <G4UIExecutive>{nullptr};

  // Construct the default run manager
  auto run_manager = unique_ptr<G4RunManager>
    {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default)};

  // Set mandatory initialization classes

  // run_manager takes ownership of detector_construction
  run_manager -> SetUserInitialization(new detector_construction{});

  { // Physics list
    auto physics_list = new QBBC;
    physics_list -> SetVerboseLevel(1);
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager -> SetUserInitialization(new action_initialization{});

  std::cout << "Something written to cout BEFORE Catch::Session()" << std::endl;

  // ----- Catch2 session --------------------------------------------------
  int result = Catch::Session().run(argc, argv);
  std::cout << "Something written to cout AFTER Catch::Session()" << std::endl;

  // ----- Post-test cleanup -----------------------------------------------

  // Smart pointers should clean up all the stuff we made for G4

  // ----- Communicate test result to OS -----------------------------------
  return result;
}
