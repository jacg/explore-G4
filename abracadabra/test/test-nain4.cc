#include "nain4.hh"

#include <G4Box.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserDetectorConstruction.hh>

#include <QBBC.hh>

#include <catch2/catch.hpp>

#include <memory>

using std::make_unique;
using std::unique_ptr;

class dummy_detector : public G4VUserDetectorConstruction {
public:
  dummy_detector() : G4VUserDetectorConstruction() {}
  virtual G4VPhysicalVolume* Construct() override {
    auto air = nain4::material("G4_AIR");
    auto l = 10 * cm;
    auto world = nain4::volume<G4Box>("World", air, l, l, l);
    return nain4::place(world).now();
  };
};

class dummy_action_init : public G4VUserActionInitialization {
public:
  dummy_action_init() : G4VUserActionInitialization() {}
  virtual void BuildForMaster() const override { /*SetUserAction(new dummy_run_action);*/ }
  virtual void Build         () const override {
    // SetUserAction(new dummy_primary_generator_action);
    // auto run_action = new dummy_run_action;
    // SetUserAction(run_action);
    // auto event_action = new dummy_event_action{run_action};
    // SetUserAction(event_action);
    // SetUserAction(new dummy_stepping_action{event_action})
  }
};

TEST_CASE("nain4", "[tag1][tag2]") {

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
  run_manager -> SetUserInitialization(new dummy_detector{});

  { // Physics list
    auto physics_list = new QBBC;
    physics_list -> SetVerboseLevel(1);
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager -> SetUserInitialization(new dummy_action_init{});

  // Smart pointers should clean up all the stuff we made for G4
}
