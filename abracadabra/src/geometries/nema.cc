#include "geometries/nema.hh"

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
  int count = 0; // TODO move into for, once we switch to C++ 20
  for (auto& sphere: spheres) {
	  std::string name = "Sphere_" + std::to_string(count);
	  auto ball  = orb(name, air, sphere.diameter);
	  auto angle = count * 360 * deg / spheres.size();
	  auto x     = inner_radius * sin(angle);
	  auto y     = inner_radius * cos(angle);
	  place(ball).in(cylinder).at(x, y, 0).now();
	  ++count;
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

  G4ThreeVector position{10*mm, 20*mm, 30*mm};
  G4double time = 0;

  generate_back_to_back_511_keV_gammas(event, position, time);
}
