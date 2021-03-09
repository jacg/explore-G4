#include "action_initialization.hh"
#include "detector_construction.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisManager.hh>

#include <QBBC.hh>
#include <Randomize.hh>

#include <catch2/catch.hpp>

#include <memory>

using std::make_unique;
using std::unique_ptr;

TEST_CASE("G4 stuff", "[tag1][tag2]") {

  // ----- Fake CLI input --------------------------------------------------
  char  arg0[] = "the-executable";
  char* argv[] = {&arg0[0], nullptr};
  int   argc   = (int)(sizeof(argv) / sizeof(argv[0])) - 1;

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

  // Smart pointers should clean up all the stuff we made for G4

}
