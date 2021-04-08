#include "trajectory_point.hh"

G4Allocator<trajectory_point> TrjPointAllocator;

trajectory_point::trajectory_point()
: position_(0.,0.,0.)
, time_(0.) {}


trajectory_point::trajectory_point(G4ThreeVector pos, G4double t)
: position_(pos)
, time_(t) {}

trajectory_point::trajectory_point(const trajectory_point& other) { *this = other; }


const trajectory_point& trajectory_point::operator=(const trajectory_point& other)
{
  position_ = other.position_;
  time_     = other.time_;

  return *this;
}

