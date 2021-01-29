#include <memory>

#include <Geant4/G4RunManagerFactory.hh>
#include <Geant4/G4UImanager.hh>

//#include "DetectorConstruction.hh"

int main() {

  // G4 boilerplate
  auto runManager = std::unique_ptr<G4RunManager>(G4RunManagerFactory::CreateRunManager());

  // // Set mandatory initialization classes
  // runManager->SetUserInitialization(new DetectorConstruction);
  // runManager->SetUserInitialization(new PhysicsList);
  // runManager->SetUserInitialization(new ActionInitialization);

  // Initialize G4 kernel
  runManager->Initialize();

  auto UI = G4UImanager::GetUIpointer();
  UI->ApplyCommand("/run/verbose 1");
  UI->ApplyCommand("/event/verbose 1");
  UI->ApplyCommand("/tracking/verbose 1");

  int numberOfEvent = 3;
  runManager->BeamOn(numberOfEvent);
}
