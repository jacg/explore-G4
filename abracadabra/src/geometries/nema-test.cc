#include "nain4.hh"

#include "geometries/nema.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

TEST_CASE("NEMA phantom geometry", "[nema][geometry]") {

  using std::setw;

  auto geometry =
    build_nema_phantom{}
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

TEST_CASE("NEMA phantom generate vertex", "[nema][generator]") {

  // Activities (intensities), radii and cylinder length
  G4double a = 10, r = 10*mm;            // Inner spheres basis
  G4double A =  1, R = 40*mm, H = 50*mm; // Phantom body cylinder

  auto phantom = build_nema_phantom{}
    // inner hot/cold spheres
    .sphere(2*r,   0) // region 0
    .sphere(  r,   a) //        1
    .sphere(2*r,   a) //        2
    .sphere(  r, 2*a) //        3
    // main cylinder            4
    .activity(A)
    .inner_radius(20*mm)
    .outer_radius(40*mm)
    .length(H)
    .build();

  // ----- Calculate expected ratio of intensities of regions --------------------
  auto pi = 3.14; // The PIs cancel, value irrelevant
  auto sphere_1_vol = 4*pi/3 * r * r * r;
  auto     body_vol =   pi   * R * R * H;
  auto all_spheres = sphere_1_vol * 18; // 2(1^3) + 2(2^3) = 2 x 9 = 18
  auto body_to_1_ratio = A * (body_vol - all_spheres) / (a * sphere_1_vol);

  for (auto volume: phantom.geometry()) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
  }

  auto z_min =  std::numeric_limits<G4double>::infinity();
  auto z_max = -std::numeric_limits<G4double>::infinity();
  auto r2_max =  0.0;

  // ----- Generate sample data --------------------------------------------------
  std::vector<float> hit_count(5, 0); // 4 spheres + 1 body
  for (unsigned i=0; i<1e6; ++i) {
    auto vertex = phantom.generate_vertex();
    auto region = phantom.in_which_region(vertex);
    //std::cout << region.value() << ' ' << vertex << std::endl;

    // Keep track of how many times each region was hit
    hit_count[region.value()]++; // TODO remove hard-wired .value()

    // Keep track of maximum vertex distances from centre
    auto [x, y, z] = std::make_tuple(vertex.x(), vertex.y(), vertex.z());
    z_min  = std::min( z_min, z);
    z_max  = std::max( z_max, z);
    r2_max = std::max(r2_max, x*x + y*y);

  }

  // ----- Vertices approach edges of the phantom, but all are inside ------------
  CHECK( z_min == Approx(-H/2).epsilon(0.001));
  CHECK( z_max == Approx( H/2).epsilon(0.001));
  CHECK(r2_max == Approx( R*R).epsilon(0.001));

  // ----- Verify expected distribution of vertices among regions ----------------
  CHECK(hit_count[0] == 0); // Inactive sphere should get no hits
  CHECK(hit_count[2] / hit_count[1] == Approx(8).epsilon(0.05)); // 2 x radius   -> 8 x weight
  CHECK(hit_count[3] / hit_count[1] == Approx(2).epsilon(0.05)); // 2 x activity -> 2 x weight
  CHECK(hit_count[4] / hit_count[1] == Approx(body_to_1_ratio).epsilon(0.05)); // Region 4: body

  // ----- Check that hits cover the whole subregions ----------------------------
  // TODO
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
