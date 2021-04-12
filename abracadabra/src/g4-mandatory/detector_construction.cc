#include "detector_construction.hh"

#include "nain4.hh"

#include <G4Box.hh>

#include "geometries/nema.hh"
#include "geometries/sipm_hamamatsu_blue.hh"

G4VPhysicalVolume* detector_construction::Construct() {

  auto air = nain4::material("G4_AIR");
  auto sipm = sipm_hamamatsu_blue(true)->GetLogicalVolume();
  auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
      nain4::place(sipm).in(world).at(x*mm, y*mm, 30*mm).now();
    }
  }
  return nain4::place(world).now();

  return sipm_hamamatsu_blue(true);

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
