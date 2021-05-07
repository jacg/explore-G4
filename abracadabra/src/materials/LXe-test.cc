// clang-format off
#include <nain4.hh>
#include <g4-mandatory.hh>
#include <materials/LXe.hh>
#include <geometries/nema.hh>

#include <G4SystemOfUnits.hh>

#include <FTFP_BERT.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4OpticalPhysics.hh>

#include <G4Orb.hh>
#include <G4Box.hh>

#include <catch2/catch.hpp>


#define DBG(stuff) std::cout << stuff << std::endl;
#define  DB(stuff) std::cout << stuff << ' ';

TEST_CASE("liquid xenon properties", "[xenon][properties]") {
  // --- Geometry -----
  auto air = n4::material("G4_AIR");
  //auto LXe = LXe_with_properties();
  auto LXe = n4::material("G4_lXe");
  CHECK(LXe -> GetDensity() / (g / cm3) == Approx(2.98));

  auto xe_sphere = [&LXe, &air] (auto radius, bool use_box) {
    return [&LXe, &air, radius, use_box] () {
      auto l2 = 1.1 * radius;
      auto sphere = n4::volume<G4Orb>("Sphere", LXe, radius);
      auto box    = n4::volume<G4Box>("Box"   , LXe, radius, radius, radius);
      auto lab    = n4::volume<G4Box>("LAB"   , air, l2, l2, l2);
      n4::place(use_box ? box : sphere).in(lab).now();
      return n4::place(lab).now();
    };
  };

  auto xe_slab = [](auto thickness) {
    return [thickness]() {
      G4double lab_size = 1 * m;
      G4Box* lab_solid =
        new G4Box("LAB", lab_size/2., lab_size/2., lab_size/2.);

      G4LogicalVolume* lab_logic =
        new G4LogicalVolume(lab_solid, G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"), "LAB");
      //this->SetLogicalVolume(lab_logic);

      G4Box* lxe_solid = new G4Box("LXE", 80.*cm/2., 80.*cm/2., thickness/2.);
      G4Material* LXe = G4NistManager::Instance()->FindOrBuildMaterial("G4_lXe");
      G4LogicalVolume* lxe_logic = new G4LogicalVolume(lxe_solid, LXe, "LXE");
      new G4PVPlacement(0, G4ThreeVector(0., 0., -2.*cm),
                        lxe_logic, "LXE", lab_logic, false, 0, true);
      return new G4PVPlacement(0, {}, lab_logic, "LAB", nullptr, false, 0, true);
    };
  };

  auto n4_slab = [](auto thickness) {
    return [thickness]() {
      auto l2 =    1* m/2;
      auto xel2 = 80*cm/2;
      auto air = n4::material("G4_AIR");
      auto LXe = n4::material("G4_lXe");
      auto lab = n4::volume<G4Box>("LAB", air,   l2,   l2, l2);
      auto lxe = n4::volume<G4Box>("LXE", LXe, xel2, xel2, thickness/2);
      n4::place(lxe).in(lab).at(0,0,thickness/2).now();
      return n4::place(lab).now();
    };
  };

  // --- Generator -----
  auto two_gammas_at_origin = [](auto event){generate_back_to_back_511_keV_gammas(event, {}, 0);};

  auto single_gamma = [](G4Event * event) {
    auto gamma  = nain4::find_particle("gamma");
    auto vertex = new G4PrimaryVertex({}, 0);
    vertex->SetPrimary(new G4PrimaryParticle(gamma, 0, 0, 511*keV));
    event->AddPrimaryVertex(vertex);
  };

  // --- Counters -----
  size_t events = 0;
  size_t passed = 0;
  auto count_unscathed = [&passed](auto step) {
    auto name = step->GetPreStepPoint()->GetTouchable()->GetVolume()->GetName();
    if (name == "LAB") {
      G4Track* track = step->GetTrack();
      auto maybe_creator = track -> GetCreatorProcess();
      auto creator = maybe_creator ? maybe_creator -> GetProcessName() : "no-creator";
      auto particle = track -> GetDefinition();
      auto particle_name = particle -> GetParticleName();
      auto energy = track -> GetTotalEnergy();
      if (energy == 511*keV) {
        passed++;
        //DBG("---------------------------> COUNTING " << creator << ' ' << particle_name << ' ' << energy / keV);
      }
    }
  };
  auto count_events = [&events](auto) { events++; };

  // --- Eliminate secondaries and post-Compton gammas -----
  auto kill_secondaries = [](auto track) {
    auto maybe_creator = track -> GetCreatorProcess();
    auto creator = maybe_creator ? maybe_creator -> GetProcessName() : "no-creator";
    auto name = track -> GetDefinition() -> GetParticleName();
    //DB(creator << ' ' << name);
    auto parent_id = track->GetParentID();
    //if    (parent_id > 0 ) {DBG("kill")} else {DBG("keep")}
    return parent_id > 0 ? G4ClassificationOfNewTrack::fKill : G4ClassificationOfNewTrack::fUrgent;
  };

  std::vector<double> xs;
  std::vector<double> ys;

  // ----- Initialize and run Geant4 -------------------------------------------
  {
    n4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    auto physics_list = new FTFP_BERT{0};
    physics_list -> ReplacePhysics(new G4EmStandardPhysics_option4());
    physics_list -> RegisterPhysics(new G4OpticalPhysics{});
    run_manager -> SetUserInitialization(physics_list);
    run_manager -> SetUserInitialization((new n4::actions{new n4::generator{single_gamma}})
      -> set((new n4::stacking_action) -> classify(kill_secondaries))
      -> set((new    n4::event_action) -> end(count_events) /*-> begin([](auto){DBG("")})*/)
      -> set (new n4::stepping_action{count_unscathed}));

    auto check_attlength = [&events, &passed, run_manager, &xs, &ys](auto build, auto distance_in_xenon) {
      events = passed = 0;
      n4::clear_geometry();
      run_manager -> SetUserInitialization(new n4::geometry{build});
      run_manager -> Initialize();
      run_manager -> BeamOn(100000);
      auto gammas_sent = 1.0 * events;
      auto ratio = passed / gammas_sent;
      auto xenon_attenuation_length = 3.7 * cm;
      auto attenuation_length = - distance_in_xenon / log(ratio);
      CHECK(ratio == distance_in_xenon);
      xs.push_back(distance_in_xenon);
      ys.push_back(ratio);
      //CHECK(attenuation_length == Approx(xenon_attenuation_length).epsilon(0.01));
    };

    // // TODO loop over various values of radius.
    // auto xenon_radius = 40 * mm;
    // check_attlength(xe_sphere(xenon_radius, 0), xenon_radius);
    // check_attlength(xe_sphere(xenon_radius, 1), xenon_radius);
    // check_attlength(n4_slab  (xenon_radius   ), xenon_radius);

    for (size_t r = 1; r<=100; r+=1) {
      auto xenon_radius = r*mm;
      check_attlength(n4_slab  (xenon_radius   ), xenon_radius);
      //check_attlength(xe_sphere(xenon_radius, 1), xenon_radius);
    }

    // auto xenon_radius = 40 * mm;
    // check_attlength(xe_sphere(xenon_radius, false), xenon_radius);

}

  for(auto x: xs) { DB(x << ","); } DBG(' ');
  for(auto y: ys) { DB(y << ","); } DBG(' ');

}
