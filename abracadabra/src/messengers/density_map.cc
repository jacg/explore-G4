#include "messengers/density_map.hh"
#include "geometries/inspect.hh"

density_map_messenger::density_map_messenger(G4RunManager* run_manager)
  : dir            {new G4UIdirectory          {"/density_map/"}}
  , cmd_filename   {new G4UIcmdWithAString     {"/density_map/filename"   , this}}
  , cmd_full_widths{new G4UIcmdWith3Vector     {"/density_map/full_widths", this}}
  , cmd_n_voxels   {new G4UIcmdWith3Vector     {"/density_map/n_voxels"   , this}}
  , cmd_generate   {new G4UIcmdWithoutParameter{"/density_map/generate"   , this}}
  , run_manager{run_manager}
{
  dir -> SetGuidance("Generating a density map from the geometry");
  cmd_filename -> SetGuidance("The filename to which the map should be written.");

  // This appears to have no effect, so set defaults in class header instead.
  // cmd_filename    -> SetDefaultValue("density-map.raw");
  // cmd_full_widths -> SetDefaultValue({301, 301, 301});
  // cmd_n_voxels    -> SetDefaultValue({301, 301, 301});

  cmd_filename    -> AvailableForStates(G4State_PreInit, G4State_Idle);
  cmd_full_widths -> AvailableForStates(G4State_PreInit, G4State_Idle);
  cmd_n_voxels    -> AvailableForStates(G4State_PreInit, G4State_Idle);
}

void density_map_messenger::SetNewValue(G4UIcommand* cmd, G4String value) {
  std::cout << "Density map command value: " << value << std::endl;
  if      (cmd == cmd_filename   .get()) { filename_    =                                       value ; }
  else if (cmd == cmd_full_widths.get()) { full_widths_ = cmd_full_widths -> GetNew3VectorValue(value); }
  else if (cmd == cmd_n_voxels   .get()) { n_voxels_    = cmd_n_voxels    -> GetNew3VectorValue(value); }
  else if (cmd == cmd_generate   .get()) { generate_density_map(); }
}

void density_map_messenger::generate_density_map() const {
  auto [nx, ny, nz] = n_voxels();
  auto [dx, dy, dz] = full_widths();

  auto inspect = std::make_unique<world_geometry_inspector>(run_manager);
  inspect -> density_map({dx,dy,dz}, {nx,ny,nz}, filename_);
}
