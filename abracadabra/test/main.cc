#include "nain4.hh"

#include <G4Box.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserDetectorConstruction.hh>

#include <QBBC.hh>

#define CATCH_CONFIG_RUNNER
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
  virtual void BuildForMaster() const override {}
  virtual void Build         () const override {}
};

int main(int argc, char** argv) {

  // ----- Fake CLI input for Geant4 ---------------------------------------
  char  arg0_g4[] = "the-executable";
  char* argv_g4[] = {&arg0_g4[0], nullptr};
  int   argc_g4   = (int)(sizeof(argv_g4) / sizeof(argv_g4[0])) - 1;

  // ----- Pre-testing setup: G4 boilerplate -------------------------------

  // Detect interactive mode (if no arguments) and define UI session
  auto ui = argc == 1
    ? make_unique<G4UIExecutive>(argc_g4, argv_g4)
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

  // ----- Catch2 session --------------------------------------------------
  int result = Catch::Session().run(argc, argv);

  // ----- Post-test cleanup -----------------------------------------------

  // Smart pointers should clean up all the stuff we made for G4

  // ----- Communicate test result to OS -----------------------------------
  return result;
}
