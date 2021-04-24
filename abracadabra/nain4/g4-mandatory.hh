#ifndef nain4_g4_mandatory_hh
#define nain4_g4_mandatory_hh

#include <G4Box.hh>
#include <G4ParticleGun.hh>
#include <G4UserEventAction.hh>
#include <G4UserRunAction.hh>
#include <G4UserSteppingAction.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserEventInformation.hh>
#include <G4VUserDetectorConstruction.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include <globals.hh>

#include <vector>

namespace nain4 {

// ----- run_action -----------------------------------------------------------------
struct run_action : public G4UserRunAction {
  run_action();
  ~run_action() override {}
  // G4Run* GenerateRun() override;
  void BeginOfRunAction(const G4Run*) override;
  void   EndOfRunAction(const G4Run*) override;
};

// --------------------------------------------------------------------------------
// TODO Currently has a hard-wired storing of steps: generalize:

// The subclass via which G4 insists that you manage the information that
// interests you about an event.
struct event_data : public G4VUserEventInformation {
  event_data(std::vector<G4Step>&& hits) : G4VUserEventInformation(), hits{std::move(hits)} {}
  ~event_data() override {};
  void Print() const override {/* purely virtual in superclass */};
  void set_hits(std::vector<G4Step>&& sensor_hits) { hits = std::move(sensor_hits); }
  std::vector<G4Step>& get_hits() { return hits; }
private:
  std::vector<G4Step> hits;
};

// ----- event_action ---------------------------------------------------------------
struct event_action : public G4UserEventAction {
  event_action(run_action* runAction);
  ~event_action() override {}
  void EndOfEventAction  (const G4Event* event) override;
private:
  run_action* action;
};

// ----- stepping_action ------------------------------------------------------------
struct stepping_action : public G4UserSteppingAction {
  stepping_action(event_action* eventAction);
  ~stepping_action() override {}
  //void UserSteppingAction(const G4Step*) override;
private:
  event_action* action;
};

// ----- actions --------------------------------------------------------------------
struct actions : public G4VUserActionInitialization {
  actions(G4VUserPrimaryGeneratorAction* generator) : generator{generator} {}
  // See B1 README for explanation of the role of BuildForMaster in multi-threaded mode.
  //void BuildForMaster() const override;
  void Build() const override;
private:
  G4VUserPrimaryGeneratorAction* generator;
};

// ----- geometry -------------------------------------------------------------------
// Quickly implement G4VUserDetectorConstruction: just instantiate this class
// with a function which returns the geometry
struct geometry : public G4VUserDetectorConstruction {
  using construct_fn = std::function<G4VPhysicalVolume*()>;
  geometry(construct_fn f) : construct{f} {}
  G4VPhysicalVolume* Construct() override { return construct(); }
private:
  construct_fn construct;
};

// --------------------------------------------------------------------------------
class generator : public G4VUserPrimaryGeneratorAction {
public:
  generator();

  void GeneratePrimaries(G4Event* event) override;

private:
  G4ParticleGun gun {1};
};

} // namespace nain4

namespace n4 { using namespace nain4; }

#endif
