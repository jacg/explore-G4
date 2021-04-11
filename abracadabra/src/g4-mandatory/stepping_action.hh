#ifndef B1SteppingAction_hh
#define B1SteppingAction_hh 1

#include <G4UserSteppingAction.hh>
#include <globals.hh>

class event_action;

class G4LogicalVolume;

class stepping_action : public G4UserSteppingAction {
public:
  stepping_action(event_action* eventAction);
  ~stepping_action() override {}

  void UserSteppingAction(const G4Step*) override;

private:
  event_action*    action;
  G4LogicalVolume* scoring_volume;
};

#endif
