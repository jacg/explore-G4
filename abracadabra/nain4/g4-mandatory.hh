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
  void EndOfEventAction  (const G4Event* event) override;
};

// ----- stepping_action ------------------------------------------------------------
struct stepping_action : public G4UserSteppingAction {};

// ----- actions --------------------------------------------------------------------
struct actions : public G4VUserActionInitialization {
  actions(G4VUserPrimaryGeneratorAction* generator) : generator{generator} {}
  // See B1 README for explanation of the role of BuildForMaster in multi-threaded mode.
  //void BuildForMaster() const override;
  void Build() const override;

  actions* run  (G4UserRunAction     * a) { run_   = a; return this; }
  actions* event(G4UserEventAction   * a) { event_ = a; return this; }
  actions* step (G4UserSteppingAction* a) { step_  = a; return this; }
  actions* track(G4UserTrackingAction* a) { track_ = a; return this; }

private:
  G4VUserPrimaryGeneratorAction* generator;
  G4UserRunAction              * run_   = nullptr;
  G4UserEventAction            * event_ = nullptr;
  G4UserSteppingAction         * step_  = nullptr;
  G4UserTrackingAction         * track_ = nullptr;
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

// ----- primary generator ----------------------------------------------------------

struct generator : public G4VUserPrimaryGeneratorAction {
  using function = std::function<void(G4Event*)>;
  generator(function fn = geantino_along_x) : doit{fn} {}
  void GeneratePrimaries(G4Event* event) override { doit(event); };
private:
  function const doit;
  static void geantino_along_x(G4Event*);
};

} // namespace nain4

namespace n4 { using namespace nain4; }

#endif
