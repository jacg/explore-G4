#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1

#include <G4UserSteppingAction.hh>
#include <globals.hh>

class event_action;

class G4LogicalVolume;

class stepping_action : public G4UserSteppingAction {
public:
  stepping_action(event_action* eventAction);
  virtual ~stepping_action() override {}

  virtual void UserSteppingAction(const G4Step*) override;

private:
  event_action*    action;
  G4LogicalVolume* scoring_volume;
};

#endif
