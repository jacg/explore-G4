#ifndef action_initialization_hh
#define action_initialization_hh 1

#include <G4VUserActionInitialization.hh>

/// Action initialization class.

class action_initialization : public G4VUserActionInitialization {
public:
  action_initialization() : G4VUserActionInitialization() {}
  ~action_initialization() override{};

  void BuildForMaster() const override;
  void Build()          const override;
};

#endif
