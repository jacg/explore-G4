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
  auto& whole  = *sipm_hamamatsu_blue(true);
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
  auto& whole  = *sipm_hamamatsu_blue(false);
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
    geometry(std::string const& f) : filename{f} {}

    G4VPhysicalVolume* Construct() {
      auto air = nain4::material("G4_AIR");
      auto sipm = sipm_hamamatsu_blue(true, filename);
      auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
      for (int x=-35; x<35; x+=7) {
        for (int y=-35; y<35; y+=7) {
          nain4::place(sipm).in(world).at(x*mm, y*mm, 30*mm).now();
        }
      }
      return nain4::place(world).now();
    }

    std::string filename;
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

  // ----- Initialize and run Geant4 ------------------------------------------

  std::string hdf5_test_file_name = std::tmpnam(nullptr) + std::string("_test.h5");
  {
    nain4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    run_manager -> SetUserInitialization(new geometry{hdf5_test_file_name});
    run_manager -> SetUserInitialization(new QBBC{0});
    run_manager -> SetUserInitialization(new actions);
    run_manager -> Initialize();
    run_manager -> BeamOn(1);
  }

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
  std::vector<hit_t> written_hit_structs;
  hdf5_io h5io{hdf5_test_file_name};
  h5io.read_hit_info(written_hit_structs);

  CHECK(written_hit_structs.size() == n_sipms);

  std::vector<G4ThreeVector> positions{};
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
	  positions.push_back({x*mm, y*mm, 29.7*mm});
    }
  }

  std::vector<G4ThreeVector> written_hit_vecs{};
  std::transform(begin(written_hit_structs), end(written_hit_structs),
		  std::back_inserter(written_hit_vecs),
		  [](auto& hit) { return G4ThreeVector{hit.x, hit.y, hit.z}; } );

  std::sort(begin(positions)       , end(positions));
  std::sort(begin(written_hit_vecs), end(written_hit_vecs));


  for (auto [i, row] : enumerate(written_hit_vecs)) {
    auto pos  = positions[i];
    auto time = pos.getZ() / (CLHEP::c_light / (mm/ns));
    // TODO: CHECK(row.event_id == ??);
    CHECK(row.getX() == pos.getX());
    CHECK(row.getY() == pos.getY());
    CHECK(row.getZ() == pos.getZ());
    // CHECK(row.time == time); // TODO
    // CHECK(row.time == Approx(pos.getZ() / (CLHEP::c_light / (mm/ns))));
  }
}
