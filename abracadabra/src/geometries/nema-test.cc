#include "nain4.hh"

#include "geometries/nema.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

TEST_CASE("NEMA phantom geometry", "[nema][geometry]") {

  using std::setw;

  auto geometry = build_nema_phantom{}
    .sphere(10*mm, 2.8)
    .sphere(13*mm, 2.8)
    .sphere(17*mm, 2.8)
    .sphere(22*mm, 2.8)
    .sphere(28*mm, 0)
    .sphere(37*mm, 0)
    .build()
    .geometry();

  std::cout << std::endl;
  for (auto v: geometry) {
    std::cout << setw(15) << v->GetName() << ": ";
    auto l = v->GetLogicalVolume();
    std::cout
      << setw(18) << l->GetMaterial()->GetName()
      << setw(12) << G4BestUnit(l->GetMass(), "Mass")
      << setw(12) << G4BestUnit(l->GetSolid()->GetCubicVolume(), "Volume")
      << std::endl;
  }
  std::cout << std::endl;

  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 8);

  for (auto volume: geometry) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
  }

}

TEST_CASE("generate 511 keV gammas", "[generate][511][gamma]") {
  // Vertex location and time
  auto where_x =  1.2*mm;
  auto where_y = -3.4*mm;
  auto where_z =  9.8*mm;
  G4ThreeVector where{where_x, where_y, where_z};
  auto when = 123.456 * ns;

  // Generate the vertex
  G4Event event;
  generate_back_to_back_511_keV_gammas(&event, where, when);
  auto& vertex = *event.GetPrimaryVertex();
  vertex.Print();

  // Get all the particles in the vertex
  // AAAARGH! Geant4 stores them in an ad-hoc linked list
  // TODO write a nain4 iterator for this?
  std::vector<G4PrimaryParticle*> particles;
  auto particle = vertex.GetPrimary();
  while (particle) {
    particles.push_back(particle);
    particle = particle -> GetNext();
  }

  // Verify expected properties
  CHECK(vertex.GetPosition() == where);
  CHECK(vertex.GetT0()       == when);
  CHECK(particles.size() == 2);
  CHECK(particles[0]->GetMomentum() +
        particles[1]->GetMomentum() == G4ThreeVector{});
  CHECK(particles[0]->GetMomentum().mag() == 511*keV);

}
