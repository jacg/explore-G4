#include "detector_construction.hh"

#include "nain4.hh"

#include <G4Tubs.hh>

#include <G4SystemOfUnits.hh>

using nain4::material;
using nain4::volume;
using nain4::place;

G4VPhysicalVolume* detector_construction::Construct() {


  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");
  auto steel   = material("G4_STAINLESS-STEEL");
  auto vacuum  = material("G4_Galactic");
  auto sensors = material("G4_WATER");    // TODO
  auto quartz  = material("G4_WATER");    // TODO
  auto LXe     = material("G4_lXe");
  auto housing = material("G4_WATER");    // TODO

  // ----- Dimensions -------------------------------------------------------------
  auto inner_radius = 325 * mm;

  // Thicknesses of each layer       // Outer radii of each layer
  auto dr_housing    =   3 * mm;     auto r_housing    = inner_radius + dr_housing;
  auto dr_vacuum_in  =  25 * mm;     auto r_vacuum_in  = r_housing    + dr_vacuum_in;
  auto dr_steel_in   =   3 * mm;     auto r_steel_in   = r_vacuum_in  + dr_steel_in;
  auto dr_LXe        =  40 * mm;     auto r_LXe        = r_steel_in   + dr_LXe;
  auto dr_quartz     =  20 * mm;     auto r_quartz     = r_LXe        + dr_quartz;
  auto dr_sensors    = 666 * mm;     auto r_sensors    = r_quartz     + dr_sensors;
  auto dr_vacuum_out = 200 * mm;     auto r_vacuum_out = r_sensors    + dr_vacuum_out;
  auto dr_steel_out  =   5 * mm;     auto r_steel_out  = r_vacuum_out + dr_steel_out;

  auto two_pi = 360 * deg;
  auto length = 70 * cm, half_length = length / 2;

  // ----- Logical volumes making up the geometry ---------------------------------

  // Bind invariant args (3, 5, 6 and 7) of volume
  auto vol = [half_length, two_pi](auto name, auto material, auto radius) {
    return volume<G4Tubs>(name, material, 0.0, radius, half_length, 0.0, two_pi);
  };

  auto vol_inner_space = vol("Inner space" , air    , inner_radius);
  auto vol_housing     = vol("Housing"     , housing, r_housing   );
  auto vol_vacuum_in   = vol("Inner vacuum", vacuum , r_vacuum_in );
  auto vol_steel_in    = vol("Inner steel" , steel  , r_steel_in  );
  auto vol_LXe         = vol("LXe"         , LXe    , r_LXe       );
  auto vol_quartz      = vol("Quartz"      , quartz , r_quartz    );
  auto vol_sensors     = vol("Sensors"     , sensors, r_sensors   );
  auto vol_vacuum_out  = vol("Outer vacuum", vacuum , r_vacuum_out);
  auto vol_steel_out   = vol("Outer steel" , steel  , r_steel_out );

  // TODO world volume ?

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_inner_space).in(vol_housing   ).now();
  place(vol_housing    ).in(vol_vacuum_in ).now();
  place(vol_vacuum_in  ).in(vol_steel_in  ).now();
  place(vol_steel_in   ).in(vol_LXe       ).now();
  place(vol_LXe        ).in(vol_quartz    ).now();
  place(vol_quartz     ).in(vol_sensors   ).now();
  place(vol_sensors    ).in(vol_vacuum_out).now();
  place(vol_vacuum_out ).in(vol_steel_out ).now();

  return place(vol_vacuum_out).now();

  //this->scoring_volume = trapezoid; // TODO

}
