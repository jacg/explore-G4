#ifndef run_action_hh
#define run_action_hh 1

#include <G4Accumulable.hh>
#include <G4UserRunAction.hh>
#include <globals.hh>

class G4Run;

/// EndOfRunAction(), calculates the dose in the selected volume from the energy
/// deposit accumulated via stepping and event actions. The computed dose is
/// then printed on the screen.

class run_action : public G4UserRunAction {
public:
  run_action();
  ~run_action() override {}

  // G4Run* GenerateRun() override;
  void BeginOfRunAction(const G4Run*) override;
  void   EndOfRunAction(const G4Run*) override;
};

#endif
