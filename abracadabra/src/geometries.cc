#include "geometries.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Tubs.hh>
#include <G4Sphere.hh>

#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>

#include <G4LogicalSkinSurface.hh>
#include <G4OpticalSurface.hh>

#include <initializer_list>
#include <vector>
#include <string>
#include <cmath>

using nain4::material;
using nain4::material_from_elements_F;
using nain4::material_from_elements_N;
using nain4::place;
using nain4::scale_by;
using nain4::volume;

G4PVPlacement* imas_demonstrator() {

  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");
  auto steel   = material("G4_STAINLESS-STEEL");
  auto vacuum  = material("G4_Galactic");
  auto sensors = material("G4_WATER");    // TODO
  auto quartz  = material("G4_WATER");    // TODO
  auto LXe     = material("G4_lXe");
  auto housing = steel;

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

  auto envelope_length = 1.1 * length;
  auto envelope_width  = 1.1 * r_steel_out;

  // ----- Logical volumes making up the geometry ---------------------------------

  // Bind invariant args (3, 5, 6 and 7) of volume
  auto vol = [half_length, two_pi](auto name, auto material, auto radius) {
    return volume<G4Tubs>(name, material, 0.0, radius, half_length, 0.0, two_pi);
  };

  auto vol_inner_space = vol("Inner_space" , air    , inner_radius);
  auto vol_housing     = vol("Housing"     , housing, r_housing   );
  auto vol_vacuum_in   = vol("Inner_vacuum", vacuum , r_vacuum_in );
  auto vol_steel_in    = vol("Inner_steel" , steel  , r_steel_in  );
  auto vol_LXe         = vol("LXe"         , LXe    , r_LXe       );
  auto vol_quartz      = vol("Quartz"      , quartz , r_quartz    );
  auto vol_sensors     = vol("Sensors"     , sensors, r_sensors   );
  auto vol_vacuum_out  = vol("Outer_vacuum", vacuum , r_vacuum_out);
  auto vol_steel_out   = vol("Outer_steel" , steel  , r_steel_out );
  auto vol_envelope = volume<G4Box>("Envelope", air, envelope_width, envelope_width, envelope_length);

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
  place(vol_steel_out  ).in(vol_envelope  ).now();
  return place(vol_envelope).now();
}

G4PVPlacement* nema_phantom(std::vector<G4double> diameters /* = default value in header */) {
  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");

  // ----- Dimensions -------------------------------------------------------------
  auto inner_radius = 114.4 * mm;
  auto outer_radius = 152   * mm;

  for (auto& d: diameters) { d *= mm; }

  auto pi     = 180 * deg;
  auto two_pi = 360 * deg;
  auto length = 113 * mm, half_length = length / 2;

  auto envelope_length = 1.1 * length;
  auto envelope_width  = 1.1 * outer_radius;

  // Bind invariant args (3, 5, 6, 7 and 8) of volume
  auto sphere = [pi, two_pi](auto name, auto material, auto diameter) {
    return volume<G4Sphere>(name, material, 0.0, diameter/2, 0.0, two_pi, 0.0, pi);
  };

  auto cylinder = volume<G4Tubs>("Cylinder", air, 0.0, outer_radius, half_length, 0.0, two_pi);

  auto vol_envelope = volume<G4Box>("Envelope", air, envelope_width, envelope_width, envelope_length);

  // Build and place spheres
  int count = 0; // TODO move into for, once we switch to C++ 20
  for (auto diameter: diameters) {
	  std::string name = "Sphere_" + std::to_string(count);
	  auto ball  = sphere(name, air, diameter);
	  auto angle = count * 360 * deg / diameters.size();
	  auto x     = inner_radius * sin(angle);
	  auto y     = inner_radius * cos(angle);
	  place(ball).in(cylinder).at(x, y, 0).now();
	  ++count;
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(cylinder).in(vol_envelope).now();

  return place(vol_envelope).now();
}

G4PVPlacement* sipm_hamamatsu_blue() {
  // ----- Materials --------------------------------------------------------------
  auto silicon = material("G4_Si");

  auto fr4 = material_from_elements_N("FR4", 1.85 * g/cm3, kStateSolid, {{"H", 12},
                                                                         {"C", 18},
                                                                         {"O",  3}});

  // ----- Dimensions -------------------------------------------------------------
  auto x_half_sipm = 6   * mm / 2;    auto x_half_active = x_half_sipm;
  auto y_half_sipm = 6   * mm / 2;    auto y_half_active = y_half_sipm;
  auto z_half_sipm = 0.6 * mm / 2;    auto z_half_active = 0.1 * mm / 2;
  // Active region to be placed at the top of SiPM in the z-direction
  auto z_active_at_top_of_sipm = z_half_sipm - z_half_active;

  // ----- Logical volumes making up the geometry ---------------------------------
  auto vol_sipm   = volume<G4Box>("SiPM_Hamamatsu_Blue", fr4, x_half_sipm,   y_half_sipm,   z_half_sipm);
  auto vol_active = volume<G4Box>("PHOTODIODES",     silicon, x_half_active, y_half_active, z_half_active);

  // ----- Cover active window with optical surface -------------------------------
  //         --- create optical surface with properties table ---
  auto active_surface = new G4OpticalSurface{"SIPM_OPTSURF", unified, polished, dielectric_metal};
  auto active_props   = new G4MaterialPropertiesTable{};
  active_surface -> SetMaterialPropertiesTable(active_props);
  //         --- insert properties into the table ---
  auto photon_energy = scale_by(eV,
    { 1.37760, 1.54980, 1.79687, 1.90745, 1.99974, 2.06640, 2.21400, 2.47968, 2.75520
    , 2.91727, 3.09960, 3.22036, 3.44400, 3.54240, 3.62526, 3.73446, 3.87450});
  active_props -> AddProperty("EFFICIENCY"  , photon_energy,
     { 0.0445, 0.1045 , 0.208  , 0.261  , 0.314  , 0.3435 , 0.420  , 0.505  , 0.528
     , 0.502 , 0.460  , 0.4195 , 0.3145 , 0.2625 , 0.211  , 0.1055 , 0.026  });
  active_props -> AddProperty("REFLECTIVITY", photon_energy,
     std::vector<G4double>(0, photon_energy.size()));

  //         --- wrap active volume in optical surface ---
  new G4LogicalSkinSurface{"SIPM_OPTSURF", vol_active, active_surface};

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_active).in(vol_sipm).at(0, 0, z_active_at_top_of_sipm).now();
  return place(vol_sipm).now();
}
