#include "nain4.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/sipm.hh"

#include "materials/LXe.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include <CLHEP/Units/PhysicalConstants.h>
#include <G4Types.hh>
#include <G4VPhysicalVolume.hh>

using nain4::volume;
using nain4::place;

using SD = G4VSensitiveDetector;

auto a_nema_phantom() {
  // Use build_nema_phantom to create one realization of the cylyndrical NEMA
  // phantom pattern
  return build_nema_phantom{}
    .activity(0)
    .length(140*mm)
    .inner_radius(114.4*mm)
    .outer_radius(152.0*mm)
    .sphere(10*mm / 2, 2.8)
    .sphere(13*mm / 2, 2.8)
    .sphere(17*mm / 2, 2.8)
    .sphere(22*mm / 2, 2.8)
    .sphere(28*mm / 2, 0)
    .sphere(37*mm / 2, 0)
    .build();
}

G4VPhysicalVolume* square_array_of_sipms(SD* sd) {
  // 10 x 10 array array of SiPMs
  auto air = nain4::material("G4_AIR");
  auto sipm = sipm_hamamatsu_blue(true, sd);
  auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
      nain4::place(sipm).in(world).at((x+3.5)*mm, (y+3.5)*mm, 30*mm).now();
    }
  }
  return nain4::place(world).now();

}
#include <utils/interpolate.hh>
// clang-format off
G4VPhysicalVolume* cylinder_lined_with_hamamatsus(double length, double radius, double dr_Xe, SD* sd) {
  // LXe-filled cylindrical shell, lined with hamamamtus
  auto air = nain4::material("G4_AIR");
  auto lXe = LXe_with_properties();

  auto cavity_r = radius - dr_Xe;

  auto xenon    = volume<G4Tubs>("LXe"     , lXe, cavity_r,   radius, length/2, 0.0, CLHEP::twopi);
  auto cavity   = volume<G4Tubs>("Cavity"  , air, 0.0     , cavity_r, length/2, 0.0, CLHEP::twopi);
  auto envelope = volume<G4Box> ("Envelope", air, 1.1*radius, 1.1*radius, 1.1*length/2);

  line_cylinder_with_tiles(xenon, sipm_hamamatsu_blue(true, sd), 1*mm);
  place(cavity).in(xenon)   .now();
  place(xenon) .in(envelope).now();
  return place(envelope).now();
}

// TODO write (or find) something that walks geometries with pointer dynamic
// casting and pointer deref safety
G4VPhysicalVolume* phantom_in_cylinder(nema_phantom const& phantom, G4double length, double radius, double dr_Xe, SD* sd) {
  auto phantom_envelope = phantom.geometry() -> GetLogicalVolume();
  auto phantom_cylinder = phantom_envelope -> GetDaughter(0) -> GetLogicalVolume();
  auto sensor_envelope = cylinder_lined_with_hamamatsus(length, radius, dr_Xe, sd);
  n4::place(phantom_cylinder).in(sensor_envelope->GetLogicalVolume()).now();
  return sensor_envelope;
}
