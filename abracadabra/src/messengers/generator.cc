#include "messengers/generator.hh"
#include "utils/map_set.hh"

generator_messenger::generator_messenger(std::map<G4String, n4::generator::function>& choices)
  : dir{new G4UIdirectory     ("/generator/")}
  , cmd{new G4UIcmdWithAString("/generator/choose", this)}
  , generate{[] (auto) { throw "No generator has been chosen"; }}
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
