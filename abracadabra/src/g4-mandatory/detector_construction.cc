#include "detector_construction.hh"

#include "geometries/samples.hh"

G4VPhysicalVolume* detector_construction::Construct() {
  return a_nema_phantom();
}
