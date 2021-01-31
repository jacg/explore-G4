#ifndef primary_generator_action_h
#define primary_generator_action_h 1

#include <memory>

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;

/// The primary generator action class with particle gun.
///
/// The default kinematic is a 6 MeV gamma, randomly distribued
/// in front of the phantom across 80% of the (X,Y) phantom size.

class primary_generator_action : public G4VUserPrimaryGeneratorAction {
public:
  primary_generator_action();
  virtual ~primary_generator_action() override {}

  virtual void GeneratePrimaries(G4Event*) override;

  const G4ParticleGun* GetParticleGun() const { return fParticleGun.get(); }

private:
  std::unique_ptr<G4ParticleGun> fParticleGun;
  G4Box* fEnvelopeBox;
};

#endif
