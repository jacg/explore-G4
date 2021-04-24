#ifndef event_action_hh
#define event_action_hh 1

#include <G4UserEventAction.hh>
#include <globals.hh>

#include <G4VUserEventInformation.hh>
#include <G4Step.hh>
#include <vector>

class run_action;


// Container to store everything we need to store later for a given event
class event_data : public G4VUserEventInformation {
public:
  event_data(std::vector<G4Step>&& hits) : G4VUserEventInformation(), hits{std::move(hits)} {}
  ~event_data() override {};

  void Print() const override {/* purely virtual in superclass */};

  void set_hits(std::vector<G4Step>&& sensor_hits) { hits = std::move(sensor_hits); }
  std::vector<G4Step>& get_hits() { return hits; }

private:
  std::vector<G4Step> hits;
};


class event_action : public G4UserEventAction {
public:
  event_action(run_action* runAction);
  ~event_action() override {}

  void EndOfEventAction  (const G4Event* event) override;

private:
  run_action* action;
};


#endif
