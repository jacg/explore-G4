#include "detector_construction.hh"

#include "nain4.hh"

#include "geometries/nema.hh"

G4VPhysicalVolume* detector_construction::Construct() {
  return build_nema_phantom{}
    .sphere(10*mm, 2.8)
    .sphere(13*mm, 2.8)
    .sphere(17*mm, 2.8)
    .sphere(22*mm, 2.8)
    .sphere(28*mm, 0)
    .sphere(37*mm, 0)
    .build()
    .geometry();
}
