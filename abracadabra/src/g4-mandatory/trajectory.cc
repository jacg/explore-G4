#include "trajectory.hh"
#include "trajectory_point.hh"
#include "trajectory_map.hh"

#include <G4Track.hh>
#include <G4VProcess.hh>


G4Allocator<trajectory> TrjAllocator;

trajectory::trajectory(const G4Track* track)
: G4VTrajectory{}
, pdef_{0}
, trackId_{-1}
, parentId_{-1}
, initial_time_{0.}
, final_time_{0}
, length_{0.}
, edep_{0.}
, record_trjpoints_{true}
, trjpoints_{0} {

  pdef_     = track->GetDefinition();
  trackId_  = track->GetTrackID();
  parentId_ = track->GetParentID();

  creator_process_ = (parentId_ == 0) ? "none" : track->GetCreatorProcess()->GetProcessName();

  initial_momentum_ = track->GetMomentum();
  initial_position_ = track->GetVertexPosition();
  initial_time_     = track->GetGlobalTime();
  initial_volume_   = track->GetVolume()->GetName();

  trjpoints_ = new TrajectoryPointContainer();

  // Add this trajectory in the map, but only if no other
  // trajectory for this track id has been registered yet
  if (!trajectory_map::Get(track->GetTrackID())) {
	trajectory_map::Add(this);
  }
}


trajectory::trajectory(const trajectory& other): G4VTrajectory{}
, pdef_{other.pdef_} {}


trajectory::~trajectory() {
  for (unsigned int i=0; i<trjpoints_->size(); ++i) {
    delete (*trjpoints_)[i];
  }
  trjpoints_->clear();
  delete trjpoints_;
}



void trajectory::AppendStep(const G4Step* step) {
  if (!record_trjpoints_) return;

  trjpoints_->push_back(new trajectory_point(step->GetPostStepPoint()->GetPosition(),
                                             step->GetPostStepPoint()->GetGlobalTime()));
}



void trajectory::MergeTrajectory(G4VTrajectory* second) {
  if (!second) return;

  if (!record_trjpoints_) return;

  trajectory* tmp = (trajectory*) second;
  G4int entries = tmp->GetPointEntries();

  // initial point of the second trajectory should not be merged
  for (G4int i=1; i<entries ; ++i) {
    trjpoints_->push_back((*(tmp->trjpoints_))[i]);
  }

  delete (*tmp->trjpoints_)[0];
  tmp->trjpoints_->clear();
}


