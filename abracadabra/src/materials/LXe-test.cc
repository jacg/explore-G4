#include <nain4.hh>
#include <g4-mandatory.hh>
#include <materials/LXe.hh>
#include <geometries/nema.hh>

#include <G4SystemOfUnits.hh>

#include <G4Orb.hh>
#include <G4Box.hh>

#include <catch2/catch.hpp>

TEST_CASE("liquid xenon properties", "[xenon][properties]") {

  // --- Geometry -----
  auto LXe = LXe_with_properties();
  CHECK(LXe -> GetDensity() / (g / cm3) == Approx(2.98));

  auto xe_sphere = [&LXe] (auto radius) {
    return [&LXe, radius] () {
      auto air = n4::material("G4_AIR");
      auto l2 = 1.1 * radius;
      auto sphere = n4::volume<G4Orb>("Sphere", LXe, radius);
      auto lab    = n4::volume<G4Box>("LAB"   , air, l2, l2, l2);
      n4::place(sphere).in(lab).now();
      return n4::place(lab).now();
    };
  };

  // --- Generator -----
  auto two_gammas_at_origin = [](auto event){generate_back_to_back_511_keV_gammas(event, {}, 0);};

  // --- Count unscathed gammas (in stepping action) -----
  size_t unscathed = 0;
  auto count_unscathed = [&unscathed](auto step) {
    auto energy = step -> GetTrack() -> GetTotalEnergy();
    if (energy == Approx(511*keV)) { // ignore post-Compton gammas
      auto name = step -> GetPreStepPoint() -> GetTouchable() -> GetVolume() -> GetName();
      if (name == "LAB") { unscathed++; }
    }
  };

  // --- Eliminate secondaries (in stacking action)  -----
  auto kill_secondaries = [](auto track) {
    auto kill = track -> GetParentID() > 0;
    return kill > 0 ? G4ClassificationOfNewTrack::fKill : G4ClassificationOfNewTrack::fUrgent;
  };

  // ----- Initialize and run Geant4 -------------------------------------------
  {
    n4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    n4::use_our_optical_physics(run_manager);
    run_manager -> SetUserInitialization((new n4::actions{two_gammas_at_origin})
         -> set((new n4::stacking_action) -> classify(kill_secondaries))
         -> set (new n4::stepping_action{count_unscathed}));

    // --- Infer attenuation length by gathering statistics for given radius -------------
    auto check_attlength = [&unscathed, run_manager](auto build, auto radius, auto events) {
      unscathed = 0;
      n4::clear_geometry();
      run_manager -> SetUserInitialization(new n4::geometry{build(radius)});
      run_manager -> Initialize();
      run_manager -> BeamOn(events);
      auto gammas_sent = 2.0 * events;
      auto ratio = unscathed / gammas_sent;
      auto expected_attenuation_length = 3.74 * cm;
      auto attenuation_length = - radius / log(ratio);
      CHECK(attenuation_length == Approx(expected_attenuation_length).epsilon(0.05));
    };
    // --- Check attenuation length across range of radii --------------------------------
    size_t events = 10000;
    for (auto r: {1,2,3,4,5,6,7,8}) { check_attlength(xe_sphere, r*cm, events); }
  }
}
