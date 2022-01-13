#ifndef messengers_density_map_hh
#define messengers_density_map_hh

#include <G4RunManager.hh>
#include <G4ThreeVector.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIcmdWith3Vector.hh>
#include <G4UIcmdWithoutParameter.hh>
#include <G4UImessenger.hh>

#include <memory>

struct density_map_messenger : G4UImessenger {
  density_map_messenger(G4RunManager*);
  void SetNewValue(G4UIcommand* command, G4String value);
  G4String filename() const { return filename_; }
  using d = G4double;
  using u = unsigned;
  std::tuple<d,d,d> full_widths() const { auto w = full_widths_; return {w.x(), w.y(), w.z()}; }
  std::tuple<u,u,u> n_voxels   () const { auto n = n_voxels_   ; return {n.x(), n.y(), n.z()}; }
private:
  std::unique_ptr<G4UIdirectory>           dir;
  std::unique_ptr<G4UIcmdWithAString>      cmd_filename;
  std::unique_ptr<G4UIcmdWith3Vector>      cmd_full_widths;
  std::unique_ptr<G4UIcmdWith3Vector>      cmd_n_voxels;
  std::unique_ptr<G4UIcmdWithoutParameter> cmd_generate;

  G4String      filename_   {"density-map.raw"};
  G4ThreeVector full_widths_{301,301,301};
  G4ThreeVector n_voxels_   {301,301,301};

  G4RunManager* run_manager;
  void generate_density_map() const;
};


#endif
