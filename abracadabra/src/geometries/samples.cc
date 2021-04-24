#include "nain4.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/sipm.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"

using nain4::volume;
using nain4::place;

G4VPhysicalVolume* a_nema_phantom() {
  // Use build_nema_phantom to create one realization of the cylyndrical NEMA
  // phantom pattern
  return build_nema_phantom{}
    .activity(1)
    .length(140*mm)
    .inner_radius(114.4*mm)
    .outer_radius(152.0*mm)
    .sphere(10*mm / 2, 2.8)
    .sphere(13*mm / 2, 2.8)
    .sphere(17*mm / 2, 2.8)
    .sphere(22*mm / 2, 2.8)
    .sphere(28*mm / 2, 0)
    .sphere(37*mm / 2, 0)
    .build()
    .geometry();
}

G4VPhysicalVolume* square_array_of_sipms() {
  // 10 x 10 array array of SiPMs
  auto air = nain4::material("G4_AIR");
  auto sipm = sipm_hamamatsu_blue(true, nullptr);
  auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
      nain4::place(sipm).in(world).at((x+3.5)*mm, (y+3.5)*mm, 30*mm).now();
    }
  }
  return nain4::place(world).now();

}

G4VPhysicalVolume* cylinder_lined_with_hamamatsus(double length, double radius) {
  // Cylinder lined with Hamamatsus
  auto material = nain4::material("G4_AIR");
  auto cylinder = volume<G4Tubs>("Cylinder", material, 0.0, radius, length/2, 0.0, CLHEP::twopi);
  auto envelope = volume<G4Box> ("Envelope", material, 1.1*radius, 1.1*radius, 1.1*length/2);
  line_cylinder_with_tiles(cylinder, sipm_hamamatsu_blue(true, nullptr), 1*mm);
  place(cylinder).in(envelope).now();
  return place(envelope).now();
}
