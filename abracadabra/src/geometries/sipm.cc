#include "geometries/sipm.hh"

#include <G4Box.hh>

#include <G4OpticalSurface.hh>
#include <G4PVPlacement.hh>


// Only needed during refactoring?
#include <G4LogicalSkinSurface.hh>



sipm::Active& sipm::Active::skin(std::string const& n) {
  auto active_surface = new G4OpticalSurface{n, unified, polished, dielectric_metal};
  return *this;
}


using nain4::material;
using nain4::material_from_elements_N;
using nain4::place;
using nain4::scale_by;
using nain4::volume;
using nain4::vis_attributes;



G4PVPlacement* sipm::build() {
  auto vol_body = volume<G4Box>(    name_,     mat,     half.x(),     half.y(),     half.z());
  auto vol_act  = volume<G4Box>(act.name_, act.mat, act.half.x(), act.half.y(), act.half.z());
  vol_act->SetSensitiveDetector(new sipm_sensitive("/does/this/matter?"));
  // --------------------------------------------------------------------------------
  // TODO most of this needs to be moved to the client, once the builder interface caters for it
  // ----- Cover active window with optical surface -------------------------------
  //         --- create optical surface with properties table ---
  auto active_surface = new G4OpticalSurface{"SIPM_OPTSURF", unified, polished, dielectric_metal};
  auto active_props   = new G4MaterialPropertiesTable{};
  active_surface -> SetMaterialPropertiesTable(active_props);
  //         --- insert properties into the table ---
  auto photon_energy = scale_by(CLHEP::eV,
    { 1.37760, 1.54980, 1.79687, 1.90745, 1.99974, 2.06640, 2.21400, 2.47968, 2.75520
    , 2.91727, 3.09960, 3.22036, 3.44400, 3.54240, 3.62526, 3.73446, 3.87450});
  active_props -> AddProperty("EFFICIENCY"  , photon_energy,
     { 0.0445, 0.1045 , 0.208  , 0.261  , 0.314  , 0.3435 , 0.420  , 0.505  , 0.528
     , 0.502 , 0.460  , 0.4195 , 0.3145 , 0.2625 , 0.211  , 0.1055 , 0.026  });
  active_props -> AddProperty("REFLECTIVITY", photon_energy, std::vector<double>(photon_energy.size(), 0));

  //         --- wrap active volume in optical surface ---
  new G4LogicalSkinSurface{"SIPM_OPTSURF", vol_act, active_surface};

  //         --- attach sensitive detector to the active logical volume ---
  vol_act -> SetSensitiveDetector(new sipm_sensitive("/does/this/matter?"));

  // ----- visibility -------------------------------------------------------------

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
  // Store the min and max y and z positions of particles reaching the detector
  hits.push_back(*step);
  return true; // TODO what is the meaning of this?
}

#include <G4SystemOfUnits.hh>


G4PVPlacement* sipm_hamamatsu_blue(G4bool /*visible*/) {

  auto fr4 = material_from_elements_N("FR4", 1.85 * g / cm3, kStateSolid, {{"H", 12},
                                                                           {"C", 18},
                                                                           {"O", 3}});


  return sipm("Hama_Blue")
    .material("G4_Si")
    .size(6 * mm, 6 * mm, 0.6 * mm)
    .active_window()
        .name("PHOTODIODES")
        .size(6 * mm, 6 * mm, 0.1 * mm)
        .material(fr4)
        .skin("SIPM_OPTSURF")
    .end_active_window()
    .build();
}
