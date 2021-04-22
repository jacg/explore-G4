// clang-format off
#include "geometries/sipm.hh"
#include "g4-mandatory/event_action.hh"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4EventManager.hh>

using nain4::material_from_elements_N;
using nain4::place;
using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;


G4LogicalVolume* sipm::build() {
  auto act_half_x = half.x() - act.margin_x;
  auto act_half_y = half.y() - act.margin_y;
  auto act_half_z = act.dz / 2;
  auto vol_body = volume<G4Box>(    name,     mat,     half.x(),     half.y(),     half.z());
  auto vol_act  = volume<G4Box>(act.name, act.mat, act_half_x  , act_half_y  , act_half_z);

  auto sens_det = new sipm_sensitive{"/does/this/matter?", h5_filename};
  sens_det->Activate(true);
  vol_act->SetSensitiveDetector(sens_det);

  // ----- visibility -------------------------------------------------------------
  vol_body -> SetVisAttributes(    vis_attributes);
  vol_act  -> SetVisAttributes(act.vis_attributes);

  // ----- geometrical relationship between components ----------------------------
  auto z_act_in_body = act.dz/2 - half.z();
  place(vol_act).in(vol_body).at(0,0,z_act_in_body).now();
  return vol_body;
}

G4bool sipm_sensitive::ProcessHits(G4Step* step, G4TouchableHistory* /*deprecated_parameter*/) {
  hits.push_back(*step);
  if (io) {
    auto pos  = step -> GetPreStepPoint() -> GetPosition();
    auto time = step -> GetPreStepPoint() -> GetGlobalTime();
    io -> write_hit_info(0, pos.getX(), pos.getY(), pos.getZ(), time);
  }
  return true; // TODO what is the meaning of this?
}

void sipm_sensitive::EndOfEvent(G4HCofThisEvent* hc){
  std::cout << "end of action" << std::endl;
  auto current_evt = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
  auto evt_data = dynamic_cast<event_data*>(current_evt -> GetUserInformation());
  evt_data -> set_hits(hits);
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

using std::optional;   using std::string;

G4LogicalVolume* sipm_hamamatsu_blue(G4bool visible, optional<string> h5_filename) {

  auto fr4 = material_from_elements_N("FR4", 1.85 * g / cm3, kStateSolid, {{"H", 12},
                                                                           {"C", 18},
                                                                           {"O", 3}});

  using va = nain4::vis_attributes;       using col = G4Colour;

  auto vis_body = visible ?    col::Yellow()                  : va().visible(false);
  auto vis_act  = visible ? va(col::Blue()).force_solid(true) : va().visible(false);

  auto active = sipm_active_window("PHOTODIODES")
    .thickness(0.1*mm)
    .material(fr4)
    .skin("SIPM_OPTSURF", fr4_surface_properties(), unified, polished, dielectric_metal)
    .vis(vis_act);

  return sipm("Hamamatsu_Blue")
    .material("G4_Si")
    .size(6*mm, 6*mm, 0.6*mm)
    .active(active)
    .vis(vis_body)
    .write(h5_filename)
    .build();

}
