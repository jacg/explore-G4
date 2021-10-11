#include "messengers/generator.hh"
#include "utils/map_set.hh"

#include "nain4.hh"

generator_messenger::generator_messenger(std::map<G4String, n4::generator::function>& choices)
  : dir{new G4UIdirectory     {"/generator/"}}
  , cmd{new G4UIcmdWithAString{"/generator/choose", this}}
  , generate{[] (auto) { FATAL("No generator has been chosen"); }}
  , choices{choices}
{
  auto default_ = "phantom";
  bool omittable;
  dir -> SetGuidance("Primary generator control");
  cmd -> SetGuidance("the primary generator to be used");
  cmd -> SetParameterName("choose", omittable=false);
  cmd -> SetDefaultValue(default_);
  cmd -> SetCandidates(""); // TODO get these from the generator map
  cmd -> AvailableForStates(G4State_PreInit, G4State_Idle);
  generate = choices[default_];
}

void generator_messenger::SetNewValue(G4UIcommand* command, G4String choice) {
  if (command == cmd.get()) {
    // TODO use proper exceptions
    if (!contains(choices, choice)) { FATAL(("Unrecoginzed generator " + choice).c_str()); }
    generate = choices[choice];
  }
}
