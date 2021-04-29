// clang-format off

#include "nain4.hh"
#include "g4-mandatory.hh"

#include "geometries/sipm.hh"
#include "io/hdf5.hh"
#include "utils/enumerate.hh"

#include <G4Box.hh>
#include <G4ParticleGun.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <G4VUserPrimaryGeneratorAction.hh>
#include <FTFP_BERT.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4OpticalPhysics.hh>

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <string>
#include <iterator>


// visible = true
TEST_CASE("Hamamatsu blue", "[geometry][hamamatsu][blue]") {
  n4::sensitive_detector sensitive{"ignoreme", {}, {}};
  auto& whole    = *sipm_hamamatsu_blue(true, &sensitive);
  auto& active_p = *whole.GetDaughter(0);
  CHECK(active_p.GetName() == "fake_active");
  auto& active_l = *active_p.GetLogicalVolume();
  // Verify the number of sub-volumes: 1 active region, 1 infinitesimal volume
  // for attaching SD in front of skin surface (G4 10.7 bug?)
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  CHECK(whole   .GetVisAttributes()->GetColour() == G4Colour::Yellow());
  CHECK(active_l.GetVisAttributes()->GetColour() == G4Colour::Blue());

  auto number_of_sensitive_detectors =
    std::count_if(begin(whole), end(whole),
                  [](auto v) { return v->GetLogicalVolume()->GetSensitiveDetector() != nullptr;});
  CHECK(number_of_sensitive_detectors == 1);

}

// visible = false
TEST_CASE("Hamamatsu blue invisible", "[geometry][hamamatsu][blue]") {
  n4::sensitive_detector sensitive{"ignoreme", {}, {}};
  auto& whole  = *sipm_hamamatsu_blue(false, &sensitive);
  auto& active = *whole.GetDaughter(1)->GetLogicalVolume();
  // Verify the number of sub-volumes: 1 active region, 1 infinitesimal volume
  // for attaching SD in front of skin surface (G4 10.7 bug?)
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  // TODO How can you verify the G4VisAttributes::Invisible attribute?
  CHECK(whole .GetVisAttributes()->GetColour() == G4Colour::White());
  CHECK(active.GetVisAttributes()->GetColour() == G4Colour::White());

}

// ----- TODO Needs to find a home --------------------------------------------------
// Utility for connecting sensitive detector to hdf5 writer
class write_with {
public:
  write_with(hdf5_io& writer)
    : PROCESS_HITS{ [this](auto step){ return this -> process_hits(step); } }
    , END_OF_EVENT{ [this](auto hc)  {        this -> end_of_event(hc  ); } }
    , writer{writer}
  {}

  bool process_hits(G4Step* step) {
    hits.push_back(*step);
    auto pt = step -> GetPreStepPoint();
    auto p = pt -> GetPosition();
    auto t = pt -> GetGlobalTime();
    writer.write_hit_info(0, p[0], p[1], p[2], t);
    return true;
  }

  void end_of_event(G4HCofThisEvent*) {
    auto current_evt = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
    auto data = new n4::event_data{std::move(hits)};
    hits = {};
    current_evt->SetUserInformation(data);
  }

  // TODO: better docs: use for filling slots in n4::sensitive_detector
  n4::sensitive_detector::process_hits_fn const PROCESS_HITS;
  n4::sensitive_detector::end_of_event_fn const END_OF_EVENT;

private:
  std::vector<G4Step> hits{};
  hdf5_io& writer;

};


TEST_CASE("hamamatsu app", "[app]") {
  // ----- Prepare storage and retrieval of hits generated during test run ------
  std::string hdf5_test_file_name = std::tmpnam(nullptr) + std::string("_test.h5");
  std::vector<G4ThreeVector> detected{};
  auto writer = new hdf5_io{hdf5_test_file_name};
  auto fwd = write_with{*writer};
  // Intercept hits being processed by sensitive detector and store their
  // positions in `detected`
  auto process_hits = [&](auto* step) -> bool {
    detected.push_back(step -> GetPreStepPoint() -> GetPosition());
    return fwd.process_hits(step);
  };
  // An SD connected to `writer` and `detected`
  n4::sensitive_detector sensitive{"testing", process_hits, fwd.END_OF_EVENT};
  // ----- Key points in the test geometry and generator -----------------------
  std::vector<std::tuple<double, double>> xys;
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) { xys.emplace_back(x*mm, y*mm); }
  }
  // ----- Geometry ------------------------------------------------------------
  n4::geometry::construct_fn tiles = [&sensitive, &xys]() {
    auto air = nain4::material("G4_AIR");
    auto sipm = sipm_hamamatsu_blue(true, &sensitive);
    auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
    for (auto [x,y] : xys) { nain4::place(sipm).in(world).at(x*mm, y*mm, 30*mm).now(); }
    return nain4::place(world).now();
  };
  // ----- Generator -----------------------------------------------------------
  n4::generator::function shoot_at_each_tile = [&xys](auto* event) {
    G4ParticleGun gun {1};
    gun.SetParticleDefinition(nain4::find_particle("geantino"));
    gun.SetParticleMomentumDirection({0, 0, 1});
    for (auto [x,y] : xys) {
        gun.SetParticlePosition({x*mm, y*mm, 0*mm});
        gun.GeneratePrimaryVertex(event);
    }
  };
  // ----- Initialize and run Geant4 -------------------------------------------
  {
    nain4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    run_manager -> SetUserInitialization(new n4::geometry{tiles});
    auto physics_list = new FTFP_BERT{0};
    physics_list -> ReplacePhysics(new G4EmStandardPhysics_option4());
    physics_list -> RegisterPhysics(new G4OpticalPhysics{});
    run_manager  -> SetUserInitialization(physics_list);
    run_manager -> SetUserInitialization(new n4::actions{new n4::generator{shoot_at_each_tile}});
    run_manager -> Initialize();
    run_manager -> BeamOn(1);
  }
  // Close & flush writer: ensure a different one is used for reading back in
  delete writer;

  // ===== Verify ==============================================================
  // Verify the number of volumes that make up the geometry
  auto world = nain4::find_physical("world");

  // Check the number of volumes that make up the geometry
  size_t n_sipms = 10 * 10, volumes_per_sipm = 3, n_worlds = 1;
  std::ptrdiff_t number_of_volumes_in_geometry = n_sipms * volumes_per_sipm + n_worlds;
  CHECK(std::distance(begin(world), end(world)) == number_of_volumes_in_geometry);

  // Retrieve hits that were written out
  auto written_hit_structs = hdf5_io{hdf5_test_file_name}.read_hit_info();
  CHECK(written_hit_structs.size() == n_sipms);

  // Generate theoretically expected hit locations
  std::vector<G4ThreeVector> expected{};
  for (auto [x,y] : xys) { expected.emplace_back(x*mm, y*mm, 29.7*mm); }

  // Translate from HDF5 compatible structs to something we like (G4ThreeVector)
  std::vector<G4ThreeVector> written{};
  std::transform(begin(written_hit_structs), end(written_hit_structs),
                 std::back_inserter(written),
                 [](auto& hit) { return G4ThreeVector{hit.x, hit.y, hit.z}; } );

  // Ensure that implementation details which affect order of arrival of
  // particles, don't affect the test result
  std::sort(begin(expected) , end(expected));
  std::sort(begin(detected) , end(detected));
  std::sort(begin(written)  , end(written));

  CHECK(detected == expected);
  CHECK(written  == expected);

  // So far we have only checked spatial positions: need to do time and event number
  for (auto [evt_id, x,y,z,t] : written_hit_structs) {
    auto expected_t = z / (CLHEP::c_light / (mm/ns));
    CHECK(t == Approx(expected_t));
    // TODO: CHECK(row.event_id == ??);
  }
}
