// clang-format off
#include "geometries/sipm.hh"

#include <G4Box.hh>

using nain4::material_from_elements_N;
using nain4::place;
using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;


G4PVPlacement* sipm::build() {
  auto vol_body = volume<G4Box>(    name_,     mat,     half.x(),     half.y(),     half.z());
  auto vol_act  = volume<G4Box>(act.name_, act.mat, act.half.x(), act.half.y(), act.half.z());
  vol_act->SetSensitiveDetector(new sipm_sensitive("/does/this/matter?"));

  // ----- visibility -------------------------------------------------------------
  // TODO provide builder methods for this
  auto visible = true;
  if (visible) {
    vol_body -> SetVisAttributes(               G4Colour::Yellow()                 );
    vol_act  -> SetVisAttributes(vis_attributes(G4Colour::Blue()).force_solid(true));
  } else {
    vol_body -> SetVisAttributes(G4VisAttributes::Invisible);
    vol_act  -> SetVisAttributes(G4VisAttributes::Invisible);
  }

  // --------------------------------------------------------------------------------
  auto z_act_in_body = half.z() - act.half.z();
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

using vec = std::vector<G4double>;

G4PVPlacement* sipm_hamamatsu_blue(G4bool /*visible*/) {

  auto fr4 = material_from_elements_N("FR4", 1.85 * g / cm3, kStateSolid, {{"H", 12},
                                                                           {"C", 18},
                                                                           {"O", 3}});

  auto photon_energy = scale_by(eV,
    { 1.37760, 1.54980, 1.79687, 1.90745, 1.99974, 2.06640, 2.21400, 2.47968, 2.75520
    , 2.91727, 3.09960, 3.22036, 3.44400, 3.54240, 3.62526, 3.73446, 3.87450});

  auto matprop = nain4::material_properties()
    .add("EFFICIENCY",   photon_energy,
         { 0.0445, 0.1045 , 0.208  , 0.261  , 0.314  , 0.3435 , 0.420  , 0.505  , 0.528
         , 0.502 , 0.460  , 0.4195 , 0.3145 , 0.2625 , 0.211  , 0.1055 , 0.026  })
    .add("REFLECTIVITY", photon_energy, 0)
    .done();

  return sipm("Hama_Blue")
    .material("G4_Si")
    .size(6 * mm, 6 * mm, 0.6 * mm)
    .active_window()
        .name("PHOTODIODES")
        .size(6 * mm, 6 * mm, 0.1 * mm)
        .material(fr4)
        .skin(matprop, "SIPM_OPTSURF", unified, polished, dielectric_metal)
    .end_active_window()
    .build();
}
