#ifndef messengers_abracadabra_hh
#define messengers_abracadabra_hh

#include <G4GenericMessenger.hh>

#include <memory>

struct abracadabra_messenger {
  abracadabra_messenger();
  size_t offset = 0;
  G4String outfile    = "default-out.h5";
  G4String geometry   = "both";
  G4String detector   = "imas";
  G4String phantom    = "nema_7";
  bool     spin       = true;
  G4int    spin_speed = 10;
  G4int    verbosity  = 1;
  G4double y_offset   = 0;
  G4double z_offset   = 0;
  G4double quartz_thickness =   0; // mm
  G4double xenon_thickness  =  40; // mm
  G4double cylinder_length  =  15; // mm
  G4double cylinder_radius  = 200; // mm
  G4double E_cut = 0; // keV
  bool steel_is_vacuum = false;
  bool vacuum_phantom  = false;
  size_t magic_level = 0;
  size_t nema5_sleeves = 1;
  G4double jaszczak_activity_sphere = 4.0;
  G4double jaszczak_activity_body   = 1.0;
  G4double jaszczak_activity_rod    = 4.0;
private:
  std::unique_ptr<G4GenericMessenger> messenger;
};

#endif
