#include "geometries/nema.hh"

#include "random/random.hh"
#include "utils/enumerate.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Orb.hh>
#include <G4Tubs.hh>

#include <G4RandomDirection.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>

#include <vector>
#include <string>
#include <cmath>

using nain4::material;
using nain4::place;
using nain4::volume;


build_nema_phantom& build_nema_phantom::sphere(G4double radius, G4double activity) {
  spheres.emplace_back(radius, activity);
  return *this;
};

build_nema_phantom& build_nema_phantom::length(G4double l) { half_length = l / 2;  return *this; };
build_nema_phantom& build_nema_phantom::inner_radius(G4double r) { inner_r = r;    return *this; };
build_nema_phantom& build_nema_phantom::outer_radius(G4double r) { outer_r = r;    return *this; };
build_nema_phantom& build_nema_phantom::activity    (G4double a) { background = a; return *this; };



nema_phantom build_nema_phantom::build() {

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
    auto h = half_length * 2;
    auto cylinder_volume = r * r * h;
    auto body_volume = cylinder_volume - spheres_total_volume;
    auto body_weight = body_volume * background;
    weights.push_back(body_weight);
  }
  pick_region.reset(new biased_choice(weights));

  return std::move(*this);
}

G4ThreeVector nema_phantom::sphere_position(size_t n) const {
  auto angle = n * 360 * deg / spheres.size();
  auto x     = inner_r * sin(angle);
  auto y     = inner_r * cos(angle);
  return {x, y, 0};
}

G4PVPlacement* nema_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");

  auto two_pi = 360 * deg;

  auto env_half_length = 1.1 * half_length;
  auto env_half_width  = 1.1 * outer_r;

  // Bind invariant args (3, 5, 6, 7 and 8) of volume
  auto orb = [](auto name, auto material, auto radius) {
    return volume<G4Orb>(name, material, radius);
  };

  auto cylinder     = volume<G4Tubs>("Cylinder", air, 0.0, outer_r, half_length, 0.0, two_pi);
  auto vol_envelope = volume<G4Box> ("Envelope", air, env_half_width, env_half_width, env_half_length);

  // Build and place spheres
  for (auto [count, sphere]: enumerate(spheres)) {
	  std::string name = "Sphere_" + std::to_string(count);
	  auto ball  = orb(name, air, sphere.radius);
    auto position = sphere_position(count);
	  place(ball).in(cylinder).at(position).now();
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(cylinder).in(vol_envelope).now();

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

void nema_phantom::generate_primaries(G4Event* event) const {

  auto position = generate_vertex();
  G4double time = 0;

  generate_back_to_back_511_keV_gammas(event, position, time);
}

G4ThreeVector nema_phantom::generate_vertex() const {
  G4ThreeVector offset; // TODO adjust for physical placement of logical geometry
  G4ThreeVector local_position;
  auto region = (*pick_region)();
  if (region < spheres.size()) { // One of the spheres
    auto centre = sphere_position(region);
    local_position = centre + random_in_sphere(spheres[region].radius);
  } else { // The phantom's body
    do {
      auto [x, y] = random_on_disc(outer_r);
      auto z = uniform(-half_length, half_length);
      local_position = {x, y, z};
    } while (inside_a_sphere(local_position));
  }
  return local_position + offset; // TODO: rotation!
}

bool nema_phantom::inside_sphere(size_t n, G4ThreeVector& position) const {
    auto r = spheres[n].radius;
    auto r2 = r*r;
    auto centre = sphere_position(n);
    return (position - centre).mag2() < r2;
}

bool nema_phantom::inside_a_sphere(G4ThreeVector& position) const {
  auto region = in_which_region(position);
  return region && region.value() < spheres.size();
}

std::optional<size_t> nema_phantom::in_which_region(G4ThreeVector& position) const {
  for (size_t n=0; n<spheres.size(); ++n) {
    if (inside_sphere(n, position)) { return n; }
  }
  if (inside_whole(position)) { return spheres.size(); }
  return {};

}
