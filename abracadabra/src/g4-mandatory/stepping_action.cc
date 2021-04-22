#include "detector_construction.hh"
#include "event_action.hh"
#include "stepping_action.hh"

#include <G4Event.hh>
#include <G4LogicalVolume.hh>
#include <G4RunManager.hh>
#include <G4Step.hh>

stepping_action::stepping_action(event_action* action)
: G4UserSteppingAction()
, action(action)
{}
