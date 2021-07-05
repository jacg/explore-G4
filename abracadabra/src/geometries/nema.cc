#include "geometries/nema.hh"

#include "random/random.hh"
#include "utils/enumerate.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Orb.hh>
#include <G4Tubs.hh>
#include <G4MultiUnion.hh>

#include <G4RandomDirection.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using nain4::material;
using nain4::place;
using nain4::volume;

// ===== Section 3: Spatial Resolution =======================================================

nema_3_phantom::nema_3_phantom(G4double fov_length)
  : vertices{{0,  1*cm, 0},
             {0, 10*cm, 0},
             {0, 20*cm, 0},
             {0,  1*cm, fov_length * 3 / 8},
             {0, 10*cm, fov_length * 3 / 8},
             {0, 20*cm, fov_length * 3 / 8}}
{}

void nema_3_phantom::generate_primaries(G4Event* event) const {
  G4ThreeVector position = vertices[fair_die(6)];
  G4double time = 0;
  generate_back_to_back_511_keV_gammas(event, position, time);
}

G4PVPlacement* nema_3_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air = material("G4_AIR");

  // Find extent of bounding box to deduce envelope size.
  G4double x_max = 0, y_max = 0, z_max = 0;
  for (auto p: vertices) {
    x_max = std::max(x_max, std::abs(p.getX()));
    y_max = std::max(y_max, std::abs(p.getY()));
    z_max = std::max(z_max, std::abs(p.getZ()));
  }

  // Envelope size:
  // - 10% margin around content bounding box,
  // - minimum 1cm in any dimension
  // - must be centred at origin (when used as world) otherwise G4 crashes
  auto half_x = std::max(x_max, 9*mm) * 1.1;
  auto half_y = std::max(y_max, 9*mm) * 1.1;
  auto half_z = std::max(z_max, 9*mm) * 1.1;

  auto container = volume<G4Box>("Cylinder", air, half_x      , half_y      , half_z);
  auto envelope  = volume<G4Box>("Envelope", air, half_x * 1.1, half_y * 1.1, half_z * 1.1);

  // Indicate positions of point sources with finite spheres
  auto marker_radius = 10 * mm;
  for (auto [count, position]: enumerate(vertices)) {
    std::string name = "Source_" + std::to_string(count);
    auto ball  = volume<G4Orb>(name, air, marker_radius);
    place(ball).in(container).at(position).now();
  }
  place(container).in(envelope).now();
  return place(envelope).now();
}

// ===== Section 7: Image Qualitiy, Accuracy of Corrections ==================================

build_nema_7_phantom& build_nema_7_phantom::sphereR(G4double radius, G4double activity) {
  spheres.emplace_back(radius, activity);
  return *this;
};

build_nema_7_phantom& build_nema_7_phantom::length(G4double l) { half_length = l / 2;  return *this; }
build_nema_7_phantom& build_nema_7_phantom::inner_radius(G4double r) { inner_r = r;    return *this; }
build_nema_7_phantom& build_nema_7_phantom::outer_radius(G4double r) { outer_r = r;    return *this; }
build_nema_7_phantom& build_nema_7_phantom::lungR       (G4double r) {  lung_r = r;    return *this; }
build_nema_7_phantom& build_nema_7_phantom::activity    (G4double a) { background = a; return *this; }

nema_7_phantom build_nema_7_phantom::build() {

  //

  // We care about the relative, not absolute, volumes: ignore factors of pi
  G4double spheres_total_volume = 0;
  std::vector<G4double> weights{};
  weights.reserve(spheres.size() + 1); // Extra element for phantom body
  for (auto& sphere : spheres) {
    auto r = sphere.radius;
    auto volume = r * r * r * 4/3;
    spheres_total_volume += volume;
    weights.push_back(volume * sphere.activity);
  }
  {
    auto r = outer_r;
    auto l =  lung_r;
    auto h = half_length * 2;
    auto cylinder_volume = r * r * h;
    auto     lung_volume = l * l * h;
    auto body_volume = cylinder_volume - spheres_total_volume - lung_volume;
    auto body_weight = body_volume * background;
    weights.push_back(body_weight);
  }
  pick_region = biased_choice(weights);

  return std::move(*this);
}

G4ThreeVector nema_7_phantom::sphere_position(int n) const {
  auto angle = (n+1) * 360 * deg / spheres.size();
  auto x     = inner_r * cos(angle);
  auto y     = inner_r * sin(angle);
  return {x, y, 0};
}

G4PVPlacement* nema_7_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air  = material("G4_AIR");
  auto lung = material("G4_AIR"); // TODO low atomic number material with density 0.3 ± 0.1 g/mL
  // TODO source and body materials should not be air
  auto body_material = air;

  auto pi = 180 * deg;

  auto    top_r = 147 * mm;
  auto corner_r =  77 * mm;

  auto z_offset = half_length - 70*mm; // NEMA requires shperes at 7cm from phantom end
  auto env_half_length = 1.1 * half_length + z_offset;
  auto env_half_width  = 1.1 * top_r;

  auto vol_envelope = volume<G4Box> ("Envelope", air , env_half_width, env_half_width, env_half_length);
  auto vol_lung     = volume<G4Tubs>("Lung"    , lung, 0.0,  lung_r, half_length, 0.0, 2*pi);

  auto corner_c_x = top_r - corner_r;
  auto corner_c_y = - corner_c_x / 2;
  auto base_half_x = corner_c_x;
  auto base_half_y = corner_r / 2;
  auto top_half = new G4Tubs("Top"   , 0.0, top_r   , half_length, 0, pi);
  auto corner   = new G4Tubs("Corner", 0.0, corner_r, half_length, pi, pi);
  auto base     = new G4Box ("Base"  , base_half_x, base_half_y, half_length);
  G4Transform3D top_half_trasform{{}, {        0  , corner_c_y              , 0}};
  G4Transform3D  corner1_trasform{{}, {-corner_c_x, corner_c_y              , 0}};
  G4Transform3D  corner2_trasform{{}, { corner_c_x, corner_c_y              , 0}};
  G4Transform3D     base_trasform{{}, {        0  , corner_c_y - base_half_y, 0}};

  auto body_solid = new G4MultiUnion("Body");
  body_solid -> AddNode(*top_half, top_half_trasform);
  body_solid -> AddNode(*corner  ,  corner1_trasform);
  body_solid -> AddNode(*corner  ,  corner2_trasform);
  body_solid -> AddNode(*base    ,     base_trasform);
  body_solid -> Voxelize();

  auto vol_body = new G4LogicalVolume(body_solid, body_material, "Body");

  // Build and place spheres
  for (auto [count, sphere]: enumerate(spheres)) {
    std::string name = "Sphere_" + std::to_string(count);
    auto ball  = volume<G4Orb>(name, air, sphere.radius);
    auto position = sphere_position(count) + G4ThreeVector{0, 0, z_offset};
    place(ball).in(vol_body).at(position).now();
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_body).in(vol_envelope).at(0,0, -z_offset).now();
  place(vol_lung).in(vol_body).now();
  return place(vol_envelope).now();
}

void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time) {
  auto gamma = nain4::find_particle("gamma");
  auto p = 511*keV * G4RandomDirection();
  auto vertex =      new G4PrimaryVertex(position, time);
  vertex->SetPrimary(new G4PrimaryParticle(gamma,  p.x(),  p.y(),  p.z()));
  vertex->SetPrimary(new G4PrimaryParticle(gamma, -p.x(), -p.y(), -p.z()));
  event -> AddPrimaryVertex(vertex);
}

void nema_7_phantom::generate_primaries(G4Event* event) const {
  auto position = generate_vertex();        G4double time = 0;
  generate_back_to_back_511_keV_gammas(event, position, time);
}

G4ThreeVector nema_7_phantom::generate_vertex() const {
  G4ThreeVector offset; // TODO adjust for physical placement of logical geometry
  G4ThreeVector local_position;
  auto region = pick_region();
  if (region < spheres.size()) { // One of the spheres
    auto centre = sphere_position(region);
    local_position = centre + random_in_sphere(spheres[region].radius);
  } else { // The phantom's body
    do {
      auto [x, y] = random_on_disc(outer_r);
      auto z = uniform(-half_length, half_length);
      local_position = {x, y, z};
    } while (inside_lung(local_position) || inside_a_sphere(local_position));
  }
  return local_position + offset; // TODO: rotation!
}

bool nema_7_phantom::inside_lung(G4ThreeVector& position) const {
  auto r2 = lung_r * lung_r;
  auto x = position.x();
  auto y = position.y();
  return x*x + y*y < r2;
}

bool nema_7_phantom::inside_this_sphere(size_t n, G4ThreeVector& position) const {
    auto r = spheres[n].radius;
    auto r2 = r*r;
    auto centre = sphere_position(n);
    return (position - centre).mag2() < r2;
}

bool nema_7_phantom::inside_a_sphere(G4ThreeVector& position) const {
  auto region = in_which_region(position);
  return region && region.value() < spheres.size();
}

std::optional<size_t> nema_7_phantom::in_which_region(G4ThreeVector& position) const {
  for (size_t n=0; n<spheres.size(); ++n) {
    if (inside_this_sphere(n, position)) { return n; }
  }
  if (inside_lung (position)) { return spheres.size(); }
  if (inside_whole(position)) { return spheres.size() + 1; }
  return {};

}
