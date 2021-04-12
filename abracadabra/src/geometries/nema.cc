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


nema_phantom build_nema_phantom::build() {

  //

  // We care about the relative, not absolute, volumes: ignore factor of 4/3 pi
  G4double spheres_total_volume = 0;
  std::vector<G4double> weights{};
  weights.reserve(spheres.size() + 1); // Extra element for phantom body
  for (auto& sphere : spheres) {
    auto d = sphere.diameter;
    auto volume = d * d * d;
    spheres_total_volume += volume;
    weights.push_back(volume * sphere.activity);
  }
  {
    auto d = outer_radius * 2;
    auto d_cubed = d * d * d;
    auto body_volume = d_cubed - spheres_total_volume;
    auto body_weight = body_volume * background;
    weights.push_back(body_weight);
  }
  pick_region.reset(new biased_choice(weights));

  return std::move(*this);
}

G4ThreeVector nema_phantom::sphere_position(unsigned n) const {
  auto angle = n * 360 * deg / spheres.size();
  auto x     = inner_radius * sin(angle);
  auto y     = inner_radius * cos(angle);
  return {x, y, 0};
}

G4PVPlacement* nema_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");

  auto two_pi = 360 * deg;
  auto length = 113 * mm, half_length = length / 2;

  auto envelope_length = 1.1 * length;
  auto envelope_width  = 1.1 * outer_radius;

  // Bind invariant args (3, 5, 6, 7 and 8) of volume
  auto orb = [](auto name, auto material, auto diameter) {
    return volume<G4Orb>(name, material, diameter/2);
  };

  auto cylinder     = volume<G4Tubs>("Cylinder", air, 0.0, outer_radius, half_length, 0.0, two_pi);
  auto vol_envelope = volume<G4Box> ("Envelope", air, envelope_width, envelope_width, envelope_length);

  // Build and place spheres
  for (auto [count, sphere]: enumerate(spheres)) {
	  std::string name = "Sphere_" + std::to_string(count);
	  auto ball  = orb(name, air, sphere.diameter);
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
    local_position = centre + random_in_sphere(spheres[region].diameter / 2);
  } else { // The phantom's body
    do {
      local_position = {};
    } while (inside_a_sphere(local_position));
  }
   return local_position + offset;
}

bool nema_phantom::inside_sphere(unsigned n, G4ThreeVector& position) const {
    auto r = spheres[n].diameter / 2;
    auto r2 = r*r;
    auto centre = sphere_position(n);
    return (position - centre).mag2() < r2;
}

bool nema_phantom::inside_a_sphere(G4ThreeVector& position) const {
  auto region = in_which_region(position);
  return region && region.value() < spheres.size();
}

std::optional<unsigned> nema_phantom::in_which_region(G4ThreeVector& position) const {
  for (unsigned n=0; n<spheres.size(); ++n) {
    if (inside_sphere(n, position)) { return n; }
  }
  if (inside_whole(position)) { return spheres.size(); }
  return {};

}
