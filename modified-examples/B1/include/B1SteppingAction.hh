#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class B1EventAction;

class G4LogicalVolume;

class B1SteppingAction : public G4UserSteppingAction {
public:
  B1SteppingAction(B1EventAction* eventAction);
  virtual ~B1SteppingAction() {}

  virtual void UserSteppingAction(const G4Step*) override;

private:
  B1EventAction*   fEventAction;
  G4LogicalVolume* fScoringVolume;
};

#endif
