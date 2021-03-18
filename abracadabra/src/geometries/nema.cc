#include "geometries.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Tubs.hh>
#include <G4Sphere.hh>

#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>

#include <vector>
#include <string>
#include <cmath>

using nain4::material;
using nain4::place;
using nain4::volume;

G4PVPlacement* nema_phantom(std::vector<G4double> diameters /* = default value in header */) {
  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");

  // ----- Dimensions -------------------------------------------------------------
  auto inner_radius = 114.4 * mm;
  auto outer_radius = 152   * mm;

  for (auto& d: diameters) { d *= mm; }

  auto pi     = 180 * deg;
  auto two_pi = 360 * deg;
  auto length = 113 * mm, half_length = length / 2;

  auto envelope_length = 1.1 * length;
  auto envelope_width  = 1.1 * outer_radius;

  // Bind invariant args (3, 5, 6, 7 and 8) of volume
  auto sphere = [pi, two_pi](auto name, auto material, auto diameter) {
    return volume<G4Sphere>(name, material, 0.0, diameter/2, 0.0, two_pi, 0.0, pi);
  };

  auto cylinder = volume<G4Tubs>("Cylinder", air, 0.0, outer_radius, half_length, 0.0, two_pi);

  auto vol_envelope = volume<G4Box>("Envelope", air, envelope_width, envelope_width, envelope_length);

  // Build and place spheres
  int count = 0; // TODO move into for, once we switch to C++ 20
  for (auto diameter: diameters) {
	  std::string name = "Sphere_" + std::to_string(count);
	  auto ball  = sphere(name, air, diameter);
	  auto angle = count * 360 * deg / diameters.size();
	  auto x     = inner_radius * sin(angle);
	  auto y     = inner_radius * cos(angle);
	  place(ball).in(cylinder).at(x, y, 0).now();
	  ++count;
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(cylinder).in(vol_envelope).now();

  return place(vol_envelope).now();
}
