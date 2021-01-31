#ifndef action_initialization_h
#define action_initialization_h 1

#include "G4VUserActionInitialization.hh"

/// Action initialization class.

class action_initialization : public G4VUserActionInitialization
{
  public:
    action_initialization();
    virtual ~action_initialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;
};

#endif
