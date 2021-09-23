#ifndef messengers_generator_hh
#define messengers_generator_hh

#include "g4-mandatory.hh"
#include "utils/map_set.hh"

#include <G4UIcmdWithAString.hh>
#include <G4UImessenger.hh>

#include <memory>

struct generator_messenger : G4UImessenger {
  generator_messenger(std::map<G4String, n4::generator::function>& choices);

  void SetNewValue(G4UIcommand* command, G4String choice) {
    if (command == cmd.get()) {
      // TODO use proper exceptions
      if (!contains(choices, choice)) { throw "Unrecoginzed generator " + choice; }
      generate = choices[choice];
    }
  }
  n4::generator::function generator() { return generate; }
private:
  std::unique_ptr<G4UIdirectory>               dir;
  std::unique_ptr<G4UIcmdWithAString>          cmd;
  n4::generator::function                      generate;
  std::map<G4String, n4::generator::function>& choices;
};

#endif
