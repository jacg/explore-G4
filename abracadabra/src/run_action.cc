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

run_action::run_action()
: G4UserRunAction()
, fEdep(0)
, fEdep2(0) {
  // add new units for dose
  new G4UnitDefinition{"milligray", "milliGy" , "Dose", 1.e-3  * gray};
  new G4UnitDefinition{"microgray", "microGy" , "Dose", 1.e-6  * gray};
  new G4UnitDefinition{ "nanogray",  "nanoGy" , "Dose", 1.e-9  * gray};
  new G4UnitDefinition{ "picogray",  "picoGy" , "Dose", 1.e-12 * gray};

  // Register accumulable to the accumulable manager
  auto accumulableManager = G4AccumulableManager::Instance();
  accumulableManager -> RegisterAccumulable(fEdep);
  accumulableManager -> RegisterAccumulable(fEdep2);
}

void run_action::BeginOfRunAction(const G4Run*) {
  // inform the runManager to save random number seed
  G4RunManager::GetRunManager() -> SetRandomNumberStore(false);

  // reset accumulables to their initial values
  G4AccumulableManager::Instance() -> Reset();
}


void run_action::EndOfRunAction(const G4Run* run) {
  G4int nofEvents = run -> GetNumberOfEvent();
  if (nofEvents == 0) return;

  // Merge accumulables
  auto accumulableManager = G4AccumulableManager::Instance();
  accumulableManager -> Merge();

  // Compute dose = total energy deposit in a run and its variance
  G4double edep  = fEdep .GetValue();
  G4double edep2 = fEdep2.GetValue();

  G4double rms = edep2 - edep * edep / nofEvents;
  if (rms > 0) rms = std::sqrt(rms); else rms = 0;

  const auto* detectorConstruction
   = static_cast<const detector_construction*>
     (G4RunManager::GetRunManager() -> GetUserDetectorConstruction());
  G4double    mass = detectorConstruction -> GetScoringVolume() -> GetMass();
  G4double    dose = edep / mass;
  G4double rmsDose = rms  / mass;

  // Run conditions
  //  note: There is no primary generator action object for "master"
  //        run manager for multi-threaded mode.
  const auto* generatorAction
   = static_cast<const primary_generator_action*>
     (G4RunManager::GetRunManager() -> GetUserPrimaryGeneratorAction());
  G4String runCondition;
  if (generatorAction) {
    const auto* particleGun = generatorAction -> GetParticleGun();
    runCondition += particleGun -> GetParticleDefinition() -> GetParticleName();
    runCondition += " of ";
    G4double particleEnergy = particleGun -> GetParticleEnergy();
    runCondition += G4BestUnit(particleEnergy, "Energy");
  }

  // Print
  if (IsMaster()) {
    G4cout << G4endl << "--------------------End of Global Run-----------------------";
  } else {
    G4cout << G4endl << "--------------------End of Local Run------------------------";
  }

  G4cout                                                                   << G4endl
     << " The run consists of " << nofEvents << " " << runCondition        << G4endl
     << " Cumulated dose per run, in scoring volume : "
     << G4BestUnit(dose,"Dose") << " rms = " << G4BestUnit(rmsDose,"Dose") << G4endl
     << "------------------------------------------------------------"     << G4endl << G4endl;
}


void run_action::AddEdep(G4double edep) {
  this -> fEdep  += edep;
  this -> fEdep2 += edep * edep;
}
