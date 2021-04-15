#include "geometries/sipm.hh"

#include "nain4.hh"

#include <G4Box.hh>

#include <G4SystemOfUnits.hh>

#include <G4LogicalSkinSurface.hh>
#include <G4OpticalSurface.hh>

using nain4::material;
using nain4::material_from_elements_N;
using nain4::place;
//using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;


G4PVPlacement* sipm_hamamatsu_blue(G4bool /*visible*/) {

  auto fr4 = material_from_elements_N("FR4", 1.85 * g/cm3, kStateSolid, {{"H", 12},
                                                                         {"C", 18},
                                                                         {"O",  3}});
  return sipm("Hama_Blue")
    .material("G4_Si")
    .size(6*mm, 6*mm, 0.6*mm)
    .active_window()
        .name("PHOTODIODES")
        .size(6*mm, 6*mm, 0.1*mm)
        .material(fr4)
        .skin("SIPM_OPTSURF")
    .end_active_window()
    .build();
}



#include "geometries/sipm_hamamatsu_blue.hh"
// ===== Geometry of one SiPM =====================================================
G4PVPlacement* sipm_hamamatsu_blue_old(G4bool visible) {
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
  auto photon_energy = nain4::scale_by(eV,
    { 1.37760, 1.54980, 1.79687, 1.90745, 1.99974, 2.06640, 2.21400, 2.47968, 2.75520
    , 2.91727, 3.09960, 3.22036, 3.44400, 3.54240, 3.62526, 3.73446, 3.87450});
  active_props -> AddProperty("EFFICIENCY"  , photon_energy,
     { 0.0445, 0.1045 , 0.208  , 0.261  , 0.314  , 0.3435 , 0.420  , 0.505  , 0.528
     , 0.502 , 0.460  , 0.4195 , 0.3145 , 0.2625 , 0.211  , 0.1055 , 0.026  });
  active_props -> AddProperty("REFLECTIVITY", photon_energy, std::vector<double>(photon_energy.size(), 0));

  //         --- wrap active volume in optical surface ---
  new G4LogicalSkinSurface{"SIPM_OPTSURF", vol_active, active_surface};

  //         --- attach sensitive detector to the active logical volume ---
  vol_active -> SetSensitiveDetector(new hamamatsu_sensitive("/does/this/matter?"));

  // ----- visibility -------------------------------------------------------------

  if (visible) {
    vol_sipm  ->SetVisAttributes(               G4Colour::Yellow()                 );
    vol_active->SetVisAttributes(vis_attributes(G4Colour::Blue()).force_solid(true));
  } else {
    vol_sipm  ->SetVisAttributes(G4VisAttributes::Invisible);
    vol_active->SetVisAttributes(G4VisAttributes::Invisible);
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_active).in(vol_sipm).at(0, 0, z_active_at_top_of_sipm).now();
  return place(vol_sipm).now();
}


G4bool hamamatsu_sensitive::ProcessHits(G4Step* step, G4TouchableHistory*) {
  // Store the min and max y and z positions of particles reaching the detector
  hits.push_back(*step);
  return true; // TODO what is the meaning of this?
}


// TODO move colour definitions elsewhere
#include <G4VisAttributes.hh>

inline G4VisAttributes Yellow()       { return { G4Colour::Yellow() }; }
inline G4VisAttributes White()        { return { G4Colour::White () }; }
inline G4VisAttributes Red()          { return { {1   ,  .0 ,  .0 } }; }
inline G4VisAttributes DarkRed()      { return { { .88,  .87,  .86} }; }
inline G4VisAttributes BloodRed()     { return { { .55,  .09,  .09} }; }
inline G4VisAttributes DarkGreen()    { return { { .0 ,  .6 ,  .0 } }; }
inline G4VisAttributes LightGreen()   { return { { .6 ,  .9 ,  .2 } }; }
inline G4VisAttributes DirtyWhite()   { return { {1   , 1   ,  .8 } }; }
inline G4VisAttributes CopperBrown()  { return { { .72,  .45,  .20} }; }
inline G4VisAttributes Brown()        { return { { .93,  .87,  .80} }; }
inline G4VisAttributes Blue()         { return { { .0 ,  .0 , 1   } }; }
inline G4VisAttributes LightBlue()    { return { { .6 ,  .8 ,  .79} }; }
inline G4VisAttributes Lilla()        { return { { .5 ,  .5 ,  .7 } }; }
inline G4VisAttributes DarkGrey()     { return { { .3 ,  .3 ,  .3 } }; }
inline G4VisAttributes LightGrey()    { return { { .7 ,  .7 ,  .7 } }; }
inline G4VisAttributes TitaniumGrey() { return { { .71,  .69,  .66} }; }


// G4PVPlacement* tile_hamamatsu_blue() {
//   // ----- Materials --------------------------------------------------------------
//   auto epoxy = material_from_elements_N("FR4", 1.3 * g/cm3, kStateSolid, {{"H", 44},
//                                                                           {"C", 15},
//                                                                           {"O",  7}});
// }
