#ifndef event_action_hh
#define event_action_hh 1

#include <G4UserEventAction.hh>
#include <globals.hh>

class run_action;

class event_action : public G4UserEventAction {
public:
  event_action(run_action* runAction);
  ~event_action() override {}

  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;

  void AddEdep(G4double edep_) { edep += edep_; }

private:
  run_action* action;
  G4double    edep;
};

#endif
