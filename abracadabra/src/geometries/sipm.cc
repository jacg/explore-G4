// clang-format off
#include "geometries/sipm.hh"

#include <G4Box.hh>

using nain4::material_from_elements_N;
using nain4::place;
using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;


G4PVPlacement* sipm::build() {
  auto act_half_x = half.x() - act.margin_x;
  auto act_half_y = half.y() - act.margin_y;
  auto act_half_z = act.dz / 2;
  auto vol_body = volume<G4Box>(    name,     mat,     half.x(),     half.y(),     half.z());
  auto vol_act  = volume<G4Box>(act.name, act.mat, act_half_x  , act_half_y  , act_half_z);
  vol_act->SetSensitiveDetector(new sipm_sensitive("/does/this/matter?"));

  // ----- visibility -------------------------------------------------------------
  vol_body -> SetVisAttributes(    vis_attributes);
  vol_act  -> SetVisAttributes(act.vis_attributes);

  // --------------------------------------------------------------------------------
  auto z_act_in_body = half.z() - act.dz/2;
  place(vol_act).in(vol_body).at(0,0,z_act_in_body).now();
  return place(vol_body).now();
}

G4bool sipm_sensitive::ProcessHits(G4Step* step, G4TouchableHistory* /*deprecated_parameter*/) {
  hits.push_back(*step);
  return true; // TODO what is the meaning of this?
}



// ------------------------------------------------------------------------------------------
// Hamamatsu Blue: one example of a SiPM
#include <G4SystemOfUnits.hh>

G4MaterialPropertiesTable* fr4_surface_properties() {
  auto photon_energy = scale_by(eV,
    { 1.37760, 1.54980, 1.79687, 1.90745, 1.99974, 2.06640, 2.21400, 2.47968, 2.75520
    , 2.91727, 3.09960, 3.22036, 3.44400, 3.54240, 3.62526, 3.73446, 3.87450});

  return nain4::material_properties()
    .add("EFFICIENCY",   photon_energy,
         { 0.0445, 0.1045 , 0.208  , 0.261  , 0.314  , 0.3435 , 0.420  , 0.505  , 0.528
         , 0.502 , 0.460  , 0.4195 , 0.3145 , 0.2625 , 0.211  , 0.1055 , 0.026  })
    .add("REFLECTIVITY", photon_energy, 0)
    .done();
}

G4PVPlacement* sipm_hamamatsu_blue(G4bool visible) {

  auto fr4 = material_from_elements_N("FR4", 1.85 * g / cm3, kStateSolid, {{"H", 12},
                                                                           {"C", 18},
                                                                           {"O", 3}});

  using va = nain4::vis_attributes;
  using col = G4Colour;

  return sipm("Hamamatsu_Blue")
    .material("G4_Si")
    .size(6*mm, 6*mm, 0.6*mm)
    .active_window(sipm_active_window("PHOTODIODES")
                   .thickness(0.1*mm)
                   //.margin(0, 0) // Unnecessary: 0 by default // TODO make it 1 micro?
                   .material(fr4)
                   .skin("SIPM_OPTSURF", fr4_surface_properties(), unified, polished, dielectric_metal)
                   .vis(visible ? va(col::Blue  ()) : va().visible(false)))
    .vis(visible ? col::Yellow() : va().visible(false))
    .build();

}
