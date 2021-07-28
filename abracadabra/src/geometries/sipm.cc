// clang-format off
#include "geometries/sipm.hh"
#include "g4-mandatory.hh"
#include "materials/LXe.hh"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4EventManager.hh>

using nain4::place;
using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;


G4LogicalVolume* sipm::build() {
  auto act_half_x = half.x() - win.margin_x;
  auto act_half_y = half.y() - win.margin_y;
  auto act_half_z = win.dz / 2;
  auto pre_z = act_half_z / 1000;
  auto prename = "pre_" + win.name;
  auto premat  = pre_active_material_;
  auto vol_body = volume<G4Box>(    name,     mat,     half.x(),     half.y(),     half.z());
  auto vol_act  = volume<G4Box>(win.name, win.mat, act_half_x  , act_half_y  , act_half_z);
  auto vol_pre  = volume<G4Box>( prename,  premat, act_half_x  , act_half_y  ,      pre_z);

  vol_pre -> SetSensitiveDetector(sensitive_detector);

  // ----- visibility -------------------------------------------------------------
  vol_body -> SetVisAttributes(    vis_attributes);
  vol_act  -> SetVisAttributes(win.vis_attributes);
  vol_pre  -> SetVisAttributes(win.vis_attributes);

  // ----- geometrical relationship between components ----------------------------
  auto z_act_in_body = win.dz/2 - half.z();
  auto z_pre_in_body =  pre_z/2 - half.z() + win.dz;
  place(vol_pre).in(vol_body).at(0,0,z_pre_in_body).name("fake_active").now();
  place(vol_act).in(vol_body).at(0,0,z_act_in_body).name("true_active").now();
  return vol_body;
}

// ----- simp_sensitive implementations --------------------------------------------------
sipm_sensitive::sipm_sensitive(G4String name, std::optional<std::string> h5_name)
  : G4VSensitiveDetector{name}
  , io{h5_name}
{
  n4::fully_activate_sensitive_detector(this);
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

void sipm_sensitive::EndOfEvent(G4HCofThisEvent*){
  auto current_evt = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
  auto data = new n4::event_data{std::move(hits)};
  hits = {};
  current_evt->SetUserInformation(data);
}

// ------------------------------------------------------------------------------------------
// Hamamatsu Blue: one example of a SiPM
#include <G4SystemOfUnits.hh>

// XXX This is not being used at the moment: PDE done in post-processing
G4MaterialPropertiesTable* sipm_surface_properties() {
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

G4LogicalVolume* sipm_hamamatsu_blue(G4bool visible, G4VSensitiveDetector* sd) {
  using va = nain4::vis_attributes;       using col = G4Colour;

  auto vis_body = visible ?    col::Yellow()                  : va().visible(false);
  auto vis_act  = visible ? va(col::Blue()).force_solid(true) : va().visible(false);

  auto quartz = quartz_with_properties();
  auto window = sipm_window("Quartz_window")
    .thickness(0.1*mm)
    .material(quartz)
    .vis(vis_act);

  return sipm("Hamamatsu_Blue", sd)
    .material("G4_Si")
    .size(6*mm, 6*mm, 0.6*mm)
    .window(window)
    .fake_active_material(quartz)
    .vis(vis_body)
    .build();

}
