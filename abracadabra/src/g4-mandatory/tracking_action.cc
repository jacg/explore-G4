#include "detector_construction.hh"
#include "event_action.hh"
#include "tracking_action.hh"
#include "trajectory.hh"
#include "trajectory_map.hh"

#include <G4Track.hh>
#include <G4TrackingManager.hh>
#include <G4OpticalPhoton.hh>


tracking_action::tracking_action(stepping_action* action)
: G4UserTrackingAction()
, action(action) {}

void tracking_action::PreUserTrackingAction(const G4Track* track) {
  // Do nothing if the track is an optical photon
  if (track->GetDefinition() == G4OpticalPhoton::Definition()) {
      fpTrackingManager->SetStoreTrajectory(false);
  } else {
	  // Create a new trajectory associated to the track.
	  // N.B. If the processesing of a track is interrupted to be resumed
	  // later on (to process, for instance, its secondaries) more than
	  // one trajectory associated to the track will be created, but
	  // the event manager will merge them at some point.
	  G4VTrajectory* trj = new trajectory{track};

	  // Set the trajectory in the tracking manager
	  fpTrackingManager->SetStoreTrajectory(true);
	  fpTrackingManager->SetTrajectory(trj);
  }
}

void tracking_action::PostUserTrackingAction(const G4Track* track) {
  // Do nothing if the track is an optical photon or an ionization electron
  if (track->GetDefinition() == G4OpticalPhoton::Definition()) return;

  trajectory* trj = (trajectory*) trajectory_map::Get(track->GetTrackID());

  // Do nothing if the track has no associated trajectory in the map
  if (!trj) return;

  // Record final time and position of the track
  trj->SetFinalPosition(track->GetPosition());
  trj->SetFinalTime(track->GetGlobalTime());
  trj->SetTrackLength(track->GetTrackLength());
  trj->SetFinalVolume(track->GetVolume()->GetName());
  trj->SetFinalMomentum(track->GetMomentum());

  // Record last process of the track
  G4String proc_name = track->GetStep()->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
  trj->SetFinalProcess(proc_name);
}
