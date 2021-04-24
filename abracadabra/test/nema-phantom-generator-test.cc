// clang-format off

#include "nain4.hh"

#include <G4Box.hh>
#include <G4EventManager.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleGun.hh>
#include <G4RunManager.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4TouchableHistory.hh>
#include <G4UserEventAction.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <QBBC.hh>

#include <Randomize.hh>
#include <catch2/catch.hpp>

#include <memory>
#include <tuple>
#include <algorithm>


// ----- Primary particle generator ----------------------------------------------

// Particles are generated at random points in the source slab, and shot along
// the x axis, towards the target slab.
class primary_generator : public G4VUserPrimaryGeneratorAction {
public:
  primary_generator(unsigned n_gun, unsigned n_loop)
  : gun{new G4ParticleGun{(G4int)n_gun}}
  , primaries_to_generate{n_loop}
  {
    // Geantinos don't interact with anything, so we can easily predict their
    // trajectory, which is very useful for writing the test.
    G4ParticleDefinition* geantino = nain4::find_particle("geantino");
    gun->SetParticleDefinition(geantino);
    // Shoot along the x-axis, so the y and z coordinates won't change during
    // flight; again, very useful for the test.
    gun->SetParticleMomentumDirection({1, 0, 0});
  }

  // G4 calls this when it needs primary particles
  void GeneratePrimaries(G4Event* event) override {
    if (!source) {
      auto source_physical = nain4::find_physical("source");
      auto source_logical = source_physical -> GetLogicalVolume();
      auto box = dynamic_cast<G4Box*>(source_logical->GetSolid());
      if (box) {
        auto t = source_physical -> GetTranslation();
        auto dx = box -> GetXHalfLength() * 2;
        auto dy = box -> GetYHalfLength() * 2;
        auto dz = box -> GetZHalfLength() * 2;
        source = std::make_tuple(t.x(), t.y(), t.z(), dx, dy, dz);
      }
    }
    auto [tx, ty, tz, dx, dy, dz] = source.value();
    for (unsigned n=0; n<primaries_to_generate; ++n) {
      auto x = tx + dx * (G4UniformRand() - 0.5);
      auto y = ty + dy * (G4UniformRand() - 0.5);
      auto z = tz + dz * (G4UniformRand() - 0.5);
      gun -> SetParticlePosition({x, y, z});
      gun -> GeneratePrimaryVertex(event);
    }
  }

private:
  std::unique_ptr<G4ParticleGun> gun;
  std::optional<std::tuple<G4double, G4double, G4double, G4double, G4double, G4double>> source;
  unsigned primaries_to_generate;
};

// ----- TESTS -------------------------------------------------------------------

TEST_CASE("nema generator", "[nema][generator]") {

}
