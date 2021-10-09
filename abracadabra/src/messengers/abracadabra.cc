#include "messengers/abracadabra.hh"

abracadabra_messenger::abracadabra_messenger()
  : messenger{new G4GenericMessenger{this, "/abracadabra/", "It's maaaaagic!"}} {
  // TODO units, ranges etc.
  messenger -> DeclareProperty("event_number_offset", offset, "Starting value for event ids");
  messenger -> DeclareProperty("outfile"   , outfile   , "file to which hdf5 tables well be written");
  messenger -> DeclareProperty("geometry"  , geometry  , "Geometry to be instantiated");
  messenger -> DeclareProperty("detector"  , detector  , "Detector to be instantiated");
  messenger -> DeclareProperty("phantom"   , phantom   , "Phantom to be used");
  messenger -> DeclareProperty("spin_view" , spin      , "Spin geometry view");
  messenger -> DeclareProperty("spin_speed", spin_speed, "Spin geometry speed");
  messenger -> DeclareProperty("verbosity" , verbosity , "Print live event information");
  messenger -> DeclareProperty("y_offset"  , y_offset  , "Used in NEMA5, for now");
  messenger -> DeclareProperty("z_offset"  , z_offset  , "Used in NEMA4, for now");
  messenger -> DeclareProperty("xenon_thickness" , xenon_thickness,  "Thickness of LXe layer (mm)");
  messenger -> DeclareProperty("quartz_thickness", quartz_thickness, "Thickness of quartz layer (mm)");
  messenger -> DeclareProperty("cylinder_length" , cylinder_length,  "Length of cylinder");
  messenger -> DeclareProperty("cylinder_radius" , cylinder_radius,  "Radius of cylinder");
  messenger -> DeclareProperty("clear_pre_lxe"   , vac_pre_lxe    ,  "Remove obstacles before LXe");
  messenger -> DeclareProperty("nema5_sleeves"   , nema5_sleeves  ,  "Number of sleeves in NEMA5 phantom");
}
