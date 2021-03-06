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
  bool     print      = false;
  G4double y_offset   = 0;
  G4double z_offset   = 0;
  G4double quartz_thickness =   0; // mm
  G4double xenon_thickness  =  40; // mm
  G4double cylinder_length  =  15; // mm
  G4double cylinder_radius  = 200; // mm
  bool vac_pre_lxe = false;
  size_t nema5_sleeves = 1;
private:
  std::unique_ptr<G4GenericMessenger> messenger;
};

#endif
