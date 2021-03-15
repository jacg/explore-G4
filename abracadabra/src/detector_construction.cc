#include "detector_construction.hh"

#include "nain4.hh"

#include "geometries.hh"

G4VPhysicalVolume* detector_construction::Construct() {
  //this->scoring_volume = trapezoid; // TODO
  // return imas_demonstrator();
  return nema_phantom();
}
