// clang-format off

#include "nain4.hh"

#include "geometries/sipm.hh"
#include "io/hdf5.hh"
#include "utils/enumerate.hh"
#include "g4-mandatory/event_action.hh"
#include "g4-mandatory/stepping_action.hh"
#include "g4-mandatory/run_action.hh"

#include <G4Box.hh>
#include <G4ParticleGun.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserDetectorConstruction.hh>

#include <G4VUserPrimaryGeneratorAction.hh>
#include <QBBC.hh>

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <string>
#include <iterator>


// visible = true
TEST_CASE("Hamamatsu blue", "[geometry][hamamatsu][blue]") {
  n4::sensitive_detector sensitive{"ignoreme", {}, {}};
  auto& whole  = *sipm_hamamatsu_blue(true, &sensitive);
  auto& active = *whole.GetDaughter(0)->GetLogicalVolume();
  // Verify the number of sub-volumes: 1 active region
  CHECK(std::distance(begin(whole), end(whole)) == 1);

  CHECK(whole .GetVisAttributes()->GetColour() == G4Colour::Yellow());
  CHECK(active.GetVisAttributes()->GetColour() == G4Colour::Blue());

  auto number_of_sensitive_detectors =
    std::count_if(begin(whole), end(whole),
                  [](auto v) { return v->GetLogicalVolume()->GetSensitiveDetector() != nullptr;});
  CHECK(number_of_sensitive_detectors == 1);

}

// visible = false
TEST_CASE("Hamamatsu blue invisible", "[geometry][hamamatsu][blue]") {
  n4::sensitive_detector sensitive{"ignoreme", {}, {}};
  auto& whole  = *sipm_hamamatsu_blue(false, &sensitive);
  auto& active = *whole.GetDaughter(0)->GetLogicalVolume();
  // Verify the number of sub-volumes: 1 active region
  CHECK(std::distance(begin(whole), end(whole)) == 1);

  // TODO How can you verify the G4VisAttributes::Invisible attribute?
  CHECK(whole .GetVisAttributes()->GetColour() == G4Colour::White());
  CHECK(active.GetVisAttributes()->GetColour() == G4Colour::White());

}


TEST_CASE("hamamatsu app", "[app]") {

  // ----- Geometry ------------------------------------------------------------
  struct geometry : public G4VUserDetectorConstruction {
    geometry(n4::sensitive_detector* sd) : sensitive{sd} {}

    G4VPhysicalVolume* Construct() {
      auto air = nain4::material("G4_AIR");
      auto sipm = sipm_hamamatsu_blue(true, sensitive);
      auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
      for (int x=-35; x<35; x+=7) {
        for (int y=-35; y<35; y+=7) {
          nain4::place(sipm).in(world).at(x*mm, y*mm, 30*mm).now();
        }
      }
      return nain4::place(world).now();
    }

    n4::sensitive_detector* sensitive;
  };

  // ----- Generator ------------------------------------------------------------
  class primary_generator : public G4VUserPrimaryGeneratorAction {
  public:
    primary_generator() {
      gun.SetParticleDefinition(nain4::find_particle("geantino"));
      gun.SetParticleMomentumDirection({0, 0, 1});
    }

    void GeneratePrimaries(G4Event* event) override {
      for (int x=-35; x<35; x+=7) {
        for (int y=-35; y<35; y+=7) {
          gun.SetParticlePosition({x*mm, y*mm, 0*mm});
          gun.GeneratePrimaryVertex(event);
        }
      }
    }

  private:
    G4ParticleGun gun {1};
  };

  // ----- Actions ------------------------------------------------------------
  class actions : public G4VUserActionInitialization {
    void BuildForMaster() const override {}
    void Build         () const override {
      auto set_and_return = [this](auto action) {
        SetUserAction(action);
        return action;
      };
      SetUserAction(new primary_generator);

      set_and_return(new stepping_action {
          set_and_return(new event_action {
              set_and_return(new run_action)})});
    }
  };

  // ----- Needs to find a home --------------------------------------------------
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
      auto data = new event_data{std::move(hits)};
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


  n4::sensitive_detector sensitive{"testing", process_hits, fwd.END_OF_EVENT};

  // ----- Initialize and run Geant4 ------------------------------------------
  {
    nain4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    run_manager -> SetUserInitialization(new geometry{&sensitive});
    run_manager -> SetUserInitialization(new QBBC{0});
    run_manager -> SetUserInitialization(new actions);
    run_manager -> Initialize();
    run_manager -> BeamOn(1);
  }

  delete writer;

  // ----- Verify -------------------------------------------------------------

  // Verify the number of volumes that make up the geometry
  auto world = nain4::find_physical("world");

  // std::cout << std::endl;
  // for (const auto v: world) {
  //   std::cout << std::setw(15) << v->GetName() << ": ";
  //   auto l = v->GetLogicalVolume();
  //   std::cout
  //     << std::setw(12) << l->GetMaterial()->GetName()
  //     << std::setw(12) << G4BestUnit(l->GetMass(), "Mass")
  //     << std::setw(12) << G4BestUnit(l->GetSolid()->GetCubicVolume(), "Volume")
  //     << std::endl;
  // }
  // std::cout << std::endl;

  size_t n_sipms = 10 * 10, volumes_per_sipm = 2, n_worlds = 1;
  std::ptrdiff_t number_of_volumes_in_geometry = n_sipms * volumes_per_sipm + n_worlds;

  CHECK(std::distance(begin(world), end(world)) == number_of_volumes_in_geometry);

  // Retrieve hits that were written out
  auto written_hit_structs = hdf5_io{hdf5_test_file_name}.read_hit_info();

  CHECK(written_hit_structs.size() == n_sipms);

  std::vector<G4ThreeVector> expected{};
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
      expected.push_back({x*mm, y*mm, 29.7*mm});
    }
  }

  std::vector<G4ThreeVector> written{};
  std::transform(begin(written_hit_structs), end(written_hit_structs),
                 std::back_inserter(written),
                 [](auto& hit) { return G4ThreeVector{hit.x, hit.y, hit.z}; } );

  std::sort(begin(expected) , end(expected));
  std::sort(begin(detected) , end(detected));
  std::sort(begin(written)  , end(written));

  CHECK(detected == expected);
  CHECK(written  == expected);

  for (auto [evt_id, x,y,z,t] : written_hit_structs) {
    auto expected_t = z / (CLHEP::c_light / (mm/ns));
    CHECK(t == Approx(expected_t));
    // TODO: CHECK(row.event_id == ??);
  }
}
