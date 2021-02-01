#ifndef event_action_h
#define event_action_h 1

#include <G4UserEventAction.hh>
#include <globals.hh>

class run_action;

class event_action : public G4UserEventAction {
public:
  event_action(run_action* runAction);
  virtual ~event_action() override {}

  virtual void BeginOfEventAction(const G4Event* event) override;
  virtual void EndOfEventAction(const G4Event* event) override;

  void AddEdep(G4double edep_) { edep += edep_; }

private:
  run_action* action;
  G4double    edep;
};

#endif
