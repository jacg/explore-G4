#include "detector_construction.hh"
#include "event_action.hh"
#include "tracking_action.hh"

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
	  fpTrackingManager->SetStoreTrajectory(true);
  }
}

void tracking_action::PostUserTrackingAction(const G4Track*) {}
