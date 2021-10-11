#include "geometries/generate_primaries.hh"

#include "nain4.hh"

#include <G4SystemOfUnits.hh>
#include <G4RandomDirection.hh>

void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time) {
  auto gamma = nain4::find_particle("gamma");
  auto p = 511*keV * G4RandomDirection();
  auto vertex =      new G4PrimaryVertex(position, time);
  vertex->SetPrimary(new G4PrimaryParticle(gamma,  p.x(),  p.y(),  p.z()));
  vertex->SetPrimary(new G4PrimaryParticle(gamma, -p.x(), -p.y(), -p.z()));
  event -> AddPrimaryVertex(vertex);
}
