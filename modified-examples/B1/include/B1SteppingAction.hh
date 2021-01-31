#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class event_action;

class G4LogicalVolume;

class B1SteppingAction : public G4UserSteppingAction {
public:
  B1SteppingAction(event_action* eventAction);
  virtual ~B1SteppingAction() {}

  virtual void UserSteppingAction(const G4Step*) override;

private:
  event_action*   fEventAction;
  G4LogicalVolume* scoring_volume;
};

#endif
