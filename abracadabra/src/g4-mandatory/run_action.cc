#include "detector_construction.hh"
#include "primary_generator_action.hh"
#include "run_action.hh"


#include <G4AccumulableManager.hh>
#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4Run.hh>
#include <G4RunManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

run_action::run_action() : G4UserRunAction{} , evt_number(0) {}

void run_action::BeginOfRunAction(const G4Run*) {
  // inform the runManager to save random number seed
  G4RunManager::GetRunManager() -> SetRandomNumberStore(false);

  // reset accumulables to their initial values
  G4AccumulableManager::Instance() -> Reset();
}

void run_action::EndOfRunAction(const G4Run* run) {
  G4int nofEvents = run -> GetNumberOfEvent();
  if (nofEvents == 0) return;

}
