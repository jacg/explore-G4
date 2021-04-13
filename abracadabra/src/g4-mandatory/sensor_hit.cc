#include "sensor_hit.hh"

G4Allocator<sensor_hit> sensor_hit_allocator;

sensor_hit::sensor_hit(const G4ThreeVector& position)
: G4VHit()
, position_{position} {}


sensor_hit::sensor_hit(const sensor_hit& other): G4VHit() { *this = other; }


const sensor_hit& sensor_hit::operator=(const sensor_hit& other) {
  position_ = other.position_;
  return *this;
}

