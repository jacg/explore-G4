#include "B1PrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"


primary_generator_action::primary_generator_action()
: G4VUserPrimaryGeneratorAction{},
  fParticleGun{new G4ParticleGun(1)}, // shoot 1 particle per invocation
  fEnvelopeBox{nullptr}
{
  // default particle kinematic
  G4ParticleDefinition* particle
    = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  fParticleGun->SetParticleEnergy(6*MeV);
}


void primary_generator_action::GeneratePrimaries(G4Event* anEvent) {
  //this function is called at the begining of each event

  // In order to avoid dependence of PrimaryGeneratorAction
  // on DetectorConstruction class we get Envelope volume
  // from G4LogicalVolumeStore.

  G4double envSizeXY = 0;
  G4double envSizeZ  = 0;

  if (!fEnvelopeBox) {
    G4LogicalVolume* envLV
      = G4LogicalVolumeStore::GetInstance()->GetVolume("Envelope");
    if (envLV) { fEnvelopeBox = dynamic_cast<G4Box*>(envLV->GetSolid()); }
  }

  if (fEnvelopeBox) {
    envSizeXY = fEnvelopeBox->GetXHalfLength() * 2;
    envSizeZ  = fEnvelopeBox->GetZHalfLength() * 2;
  } else {
    G4ExceptionDescription msg;
    msg << "Envelope volume of box shape not found.\n"
        << "Perhaps you have changed geometry.\n"
        << "The gun will be place at the center.";
    G4Exception("B1PrimaryGeneratorAction::GeneratePrimaries()",
                "MyCode0002", JustWarning, msg);
  }

  G4double size = 0.8;
  G4double x0 = size * envSizeXY * (G4UniformRand() - 0.5);
  G4double y0 = size * envSizeXY * (G4UniformRand() - 0.5);
  G4double z0 = -0.5 * envSizeZ;

  fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));
  fParticleGun->GeneratePrimaryVertex(anEvent);
}
