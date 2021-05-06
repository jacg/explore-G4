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
  auto LXe = LXe_with_properties();
  CHECK(LXe -> GetDensity() / (g / cm3) == Approx(2.98));

  auto xe_sphere = [&LXe, &air] (auto radius) {
    return [&LXe, &air, radius] () {
      auto l2 = 1.2 * radius;
      auto sphere = n4::volume<G4Orb>("Sphere", LXe, radius);
      auto out    = n4::volume<G4Box>("Out"   , air, l2, l2, l2);
      n4::place(sphere).in(out).now();
      return n4::place(out).now();
    };
  };

  // --- Generator -----
  auto two_gammas_at_origin = [](auto event){generate_back_to_back_511_keV_gammas(event, {}, 0);};

  // --- Counters -----
  size_t events = 0;
  size_t passed = 0;
  auto count_unscathed = [&passed](auto step) {
    auto name = step->GetPreStepPoint()->GetTouchable()->GetVolume()->GetName();
    if (name == "Out") { passed++; }
  };
  auto count_events = [&events](auto) { events++; };

  // --- Eliminate secondaries and post-Compton gammas -----
  auto kill_secondaries = [](auto track) {
    auto parent_id = track->GetParentID();
    return parent_id > 0 ? G4ClassificationOfNewTrack::fKill : G4ClassificationOfNewTrack::fUrgent;
  };

  // ----- Initialize and run Geant4 -------------------------------------------
  // TODO loop over various values of radius.
  auto xenon_radius = 40 * mm;
  {
    n4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    run_manager -> SetUserInitialization(new n4::geometry{xe_sphere(xenon_radius)});
    auto physics_list = new FTFP_BERT{0};
    physics_list -> ReplacePhysics(new G4EmStandardPhysics_option4());
    physics_list -> RegisterPhysics(new G4OpticalPhysics{});
    run_manager -> SetUserInitialization(physics_list);
    run_manager -> SetUserInitialization((new n4::actions{new n4::generator{two_gammas_at_origin}})
      -> set((new n4::stacking_action) -> classify(kill_secondaries))
      -> set((new    n4::event_action) -> end(count_events))
      -> set (new n4::stepping_action{count_unscathed}));
    run_manager -> Initialize();
    run_manager -> BeamOn(10000);
  }

  auto gammas_sent = 2.0 * events;
  auto ratio = passed / gammas_sent;
  auto xenon_attenuation_length = 3.7 * cm;
  auto attenuation_length = - xenon_radius / log(ratio);
  CHECK(attenuation_length == Approx(xenon_attenuation_length).epsilon(0.01));

}
