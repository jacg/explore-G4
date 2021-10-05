#include "geometries/imas.hh"
#include "nain4.hh"

#include "geometries/nema.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

TEST_CASE("NEMA4 phantom geometry", "[nema4][geometry]") {
  const G4double z_offset =  34.5 * mm;
  const G4double y_offset = -45.0 * mm;
  auto geometry = nema_4_phantom(z_offset).geometry();
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 3); // Envelope + cylinder + source

  for (auto volume : geometry) { CHECK(volume->CheckOverlaps(1000, 0, false) == false); }

  bool verbose = false;
  auto cylinder = n4::find_physical("Cylinder"   , verbose);
  auto source   = n4::find_physical("Line_source", verbose);
  CHECK(cylinder->GetTranslation() == G4ThreeVector{0,        0, z_offset});
  CHECK(source  ->GetTranslation() == G4ThreeVector{0, y_offset,        0});

  auto cylinder_solid = n4::find_logical("Cylinder", verbose);
  auto cylinder_material = cylinder_solid    -> GetMaterial();
  auto cylinder_density  = cylinder_material -> GetDensity();
  CHECK(cylinder_density / (g / mL) == 0.96);
}

TEST_CASE("NEMA4 phantom generate vertex", "[nema4][generator]") {
  const G4double y_offset =  45   * mm;
  const G4double z_offset =  23.4 * mm;
  const G4double length   = 700   * mm;

  auto phantom = nema_4_phantom{z_offset};

  // ----- Space for gathering statistics ----------------------------------------
  auto  x_min =  std::numeric_limits<G4double>::infinity();
  auto  x_max = -std::numeric_limits<G4double>::infinity();
  auto  y_min =  std::numeric_limits<G4double>::infinity();
  auto  y_max = -std::numeric_limits<G4double>::infinity();
  auto  z_min =  std::numeric_limits<G4double>::infinity();
  auto  z_max = -std::numeric_limits<G4double>::infinity();

  // ----- Generate sample data --------------------------------------------------
  for (unsigned i=0; i<1e6; ++i) {
    auto vertex = phantom.generate_vertex();

    // Keep track of maximum vertex distances from centre
    auto [x, y, z] = std::make_tuple(vertex.x(), vertex.y(), vertex.z());
    x_min  = std::min(x_min, x);
    x_max  = std::max(x_max, x);
    y_min  = std::min(y_min, y);
    y_max  = std::max(y_max, y);
    z_min  = std::min(z_min, z);
    z_max  = std::max(z_max, z);
  }

  CHECK(x_min ==        0);
  CHECK(x_max ==        0);
  CHECK(y_min ==       -y_offset);
  CHECK(y_max ==       -y_offset);
  CHECK(z_min == Approx(z_offset - length / 2));
  CHECK(z_max == Approx(z_offset + length / 2));
}

TEST_CASE("NEMA7 phantom geometry", "[nema7][geometry]") {

  using std::setw;

  auto geometry =
    build_nema_7_phantom{}
    .sphereD(10*mm, 2.8)
    .sphereD(13*mm, 2.8)
    .sphereD(17*mm, 2.8)
    .sphereD(22*mm, 2.8)
    .sphereD(28*mm, 0)
    .sphereD(37*mm, 0)
    .inner_radius(50*mm)
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
  CHECK(std::distance(begin(geometry), end(geometry)) == 9);

  for (auto volume: geometry) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
  }

  bool verbose = false;
  auto lung_density = n4::find_logical("Lung", verbose) -> GetMaterial() -> GetDensity();
  CHECK(lung_density / (g / mL) == Approx(0.3));

}

TEST_CASE("NEMA7 phantom generate vertex", "[nema7][generator]") {

  // Activities (intensities), radii and cylinder length
  G4double a = 10,  r =  4 * mm;               // Inner spheres basis
  G4double A = .4, tr = 46 * mm, cr = 30 * mm; // Phantom body
  G4double         lr = 18 * mm;               // Lung radius
  G4double H  = 20 * mm;                       // Length of body

  auto phantom = build_nema_7_phantom{} // inner hot/cold spheres
    .sphereR(r    ,     a) // region 0  (compare hit rates to this sphere)
    .sphereR(2 * r,     0) //        1
    .sphereR(r    , 2 * a) //        2
    .sphereR(2 * r,     a) //        3
    .lungR(lr)         //         4
    // main cylinder              5
    .activity(A)
    .inner_radius(28 * mm)
    .top_radius(tr)
    .corner_radius(cr)
    .length(H)
    .spheres_from_end(H / 2)
    .build();

  // ----- Expected ratio of vertices in   body : (sphere 1) ---------------------
  auto pi = 180 * deg;
  auto sphere_0_vol = 4*pi/3 *  r *  r * r;
  auto     lung_vol =   pi   * lr * lr *       H;
  auto      top_vol =   pi   * tr * tr *       H / 2;
  auto  corners_vol =   pi   * cr * cr *       H / 2;
  auto     base_vol =    2  * (tr - cr) * cr * H;
  auto all_spheres_vol = sphere_0_vol * 18; // 2(1^3) + 2(2^3) = 2 x 9 = 18
  auto     body_vol =   top_vol + corners_vol + base_vol - all_spheres_vol - lung_vol;
  auto body_to_0_ratio = A * body_vol / (a * sphere_0_vol);

  // ----- Ensure no volume overlaps ---------------------------------------------
  for (auto volume: phantom.geometry()) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
  }

  // ----- Space for gathering statistics ----------------------------------------
  auto  z_min =  std::numeric_limits<G4double>::infinity();
  auto  z_max = -std::numeric_limits<G4double>::infinity();
  auto r2_max =  0.0;
  auto r2_min = std::numeric_limits<double>::max();
  std::vector<double> hit_count(6, 0); // 4 spheres + 1 lung + 1 body

  // ----- Generate sample data --------------------------------------------------
  for (unsigned i=0; i<2e7; ++i) {
    auto vertex = phantom.generate_vertex();
    auto region = phantom.in_which_region(vertex);

    // Keep track of how many times each region was hit
    hit_count[region.value()]++; // TODO remove hard-wired .value()

    // Keep track of maximum vertex distances from centre
    auto [x, y, z] = std::make_tuple(vertex.x(), vertex.y(), vertex.z());
    z_min  = std::min( z_min, z);
    z_max  = std::max( z_max, z);
    r2_max = std::max(r2_max, x*x + y*y);
    r2_min = std::min(r2_min, x*x + y*y);
  }

  // ----- Vertices approach edges of the phantom, but all are inside ------------
  // With the non-cylindrical phantom body, most of these are probably not worth
  // the effort any more
  // CHECK( z_min == Approx(-H/2 ).epsilon(0.001));
  // CHECK( z_max == Approx( H/2 ).epsilon(0.001));
  // CHECK(r2_max == Approx( R*R ).epsilon(0.001));
  CHECK(r2_min == Approx(lr*lr).epsilon(0.001));

  // ----- Verify expected distribution of vertices among regions ----------------
  CHECK(hit_count[1] == 0); // Inactive sphere should generate nothing
  CHECK(hit_count[2] / hit_count[0] == Approx(2).epsilon(0.002)); // 2 x activity -> 2 x weight
  CHECK(hit_count[3] / hit_count[0] == Approx(8).epsilon(0.002)); // 2 x radius   -> 8 x weight
  CHECK(hit_count[4] / hit_count[0] == 0); // Inactive should generate nothing     Region 4: lung
  CHECK(hit_count[5] / hit_count[0] == Approx(body_to_0_ratio).epsilon(0.005)); // Region 5: body

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
  //vertex.Print();

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
