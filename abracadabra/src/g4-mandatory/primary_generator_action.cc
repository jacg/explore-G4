#include "primary_generator_action.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4SystemOfUnits.hh>

#include <Randomize.hh>

primary_generator_action::primary_generator_action()
: G4VUserPrimaryGeneratorAction{}
, particle_gun{new G4ParticleGun{1}} // shoot 1 particle per invocation
, envelope_box{nullptr} {
  // default particle kinematic
  G4ParticleDefinition* particle = nain4::find_particle("gamma");
  particle_gun -> SetParticleDefinition(particle);
  particle_gun -> SetParticleMomentumDirection({0, 0, 1});
  particle_gun -> SetParticleEnergy(6 * MeV);
}

void primary_generator_action::GeneratePrimaries(G4Event* anEvent) {
  // this function is called at the begining of each event

  // In order to avoid dependence of PrimaryGeneratorAction
  // on DetectorConstruction class we get Envelope volume
  // from G4LogicalVolumeStore.

  G4double envSizeXY = 0;
  G4double envSizeZ  = 0;

  if (!envelope_box) {
    G4LogicalVolume* envLV = nain4::find_logical("Envelope");
    if (envLV) { envelope_box = dynamic_cast<G4Box*>(envLV -> GetSolid()); }
  }

  if (envelope_box) {
    envSizeXY = envelope_box -> GetXHalfLength() * 2;
    envSizeZ  = envelope_box -> GetZHalfLength() * 2;
  } else {
    G4ExceptionDescription msg;
    msg << "Envelope volume of box shape not found.\n"
        << "Perhaps you have changed geometry.\n"
        << "The gun will be place at the center.";
    G4Exception("primary_generator_action::GeneratePrimaries()", "MyCode0002", JustWarning, msg);
  }

  G4double size = 0.8;
  G4double x0   = size * envSizeXY * (G4UniformRand() - 0.5);
  G4double y0   = size * envSizeXY * (G4UniformRand() - 0.5);
  G4double z0   = -0.5 * envSizeZ;

  particle_gun -> SetParticlePosition({x0, y0, z0});
  particle_gun -> GeneratePrimaryVertex(anEvent);
}
