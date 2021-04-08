#ifndef TrackingAction_hh
#define TrackingAction_hh 1

#include <G4UserTrackingAction.hh>
#include <globals.hh>

class stepping_action;

class tracking_action : public G4UserTrackingAction {
public:
  tracking_action(stepping_action* steppingAction);
  virtual ~tracking_action() override {}

  virtual void PreUserTrackingAction(const G4Track*)  override;
  virtual void PostUserTrackingAction(const G4Track*) override;

private:
  stepping_action* action;
};

#endif
