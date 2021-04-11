#ifndef primary_generator_action_hh
#define primary_generator_action_hh 1

#include <G4ParticleGun.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <globals.hh>

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
  ~primary_generator_action() override {}

  void GeneratePrimaries(G4Event*) override;

  const G4ParticleGun& get_particle_gun() const { return gun; }

private:
  G4ParticleGun gun;
  G4Box* envelope_box;
};

#endif
