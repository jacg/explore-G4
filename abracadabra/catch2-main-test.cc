#include <G4Box.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserDetectorConstruction.hh>

#include <QBBC.hh>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <fstream>
#include <memory>

using std::unique_ptr;

class dummy_detector : public G4VUserDetectorConstruction {
public:
  dummy_detector() : G4VUserDetectorConstruction() {}
  virtual G4VPhysicalVolume* Construct() override { return nullptr; };
};

class dummy_action_init : public G4VUserActionInitialization {
public:
  dummy_action_init() : G4VUserActionInitialization() {}
  virtual void BuildForMaster() const override {}
  virtual void Build         () const override {}
};

int main(int argc, char** argv) {

  // ----- Pre-testing setup: G4 boilerplate -------------------------------

  // Redirect G4cout to /dev/null while banner is printed
  auto g4cout_buf = G4cout.rdbuf();
  G4cout.rdbuf(std::ofstream{"/dev/null"}.rdbuf());

  // Construct the default run manager
  auto run_manager = unique_ptr<G4RunManager>
    {G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly)};

  // Stop redicecting G4cout to /dev/null
  G4cout.rdbuf(g4cout_buf);

  // Set mandatory initialization classes

  // run_manager takes ownership of detector_construction
  run_manager -> SetUserInitialization(new dummy_detector{});

  { // Physics list
    auto verbosity = 0;
    auto physics_list = new QBBC(verbosity); // SetVerboseLevel method also exists
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
