#include "nain4.hh"

#include "geometries/sipm.hh"
#include "io/hdf5.hh"
#include "utils/enumerate.hh"

#include <G4Box.hh>
#include <G4ParticleGun.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserDetectorConstruction.hh>

#include <G4VUserPrimaryGeneratorAction.hh>
#include <QBBC.hh>

#include <catch2/catch.hpp>

#include <algorithm>
#include <string>


// visible = true
TEST_CASE("Hamamatsu blue", "[geometry][hamamatsu][blue]") {
  auto& whole  = *sipm_hamamatsu_blue(true, {});
  auto& active = *whole.GetLogicalVolume()->GetDaughter(0);
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  CHECK(whole .GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::Yellow());
  CHECK(active.GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::Blue());

  auto number_of_sensitive_detectors =
    std::count_if(begin(whole), end(whole),
                  [](auto v) { return v->GetLogicalVolume()->GetSensitiveDetector() != nullptr;});
  CHECK(number_of_sensitive_detectors == 1);

}

// visible = false
TEST_CASE("Hamamatsu blue invisible", "[geometry][hamamatsu][blue]") {
  auto& whole  = *sipm_hamamatsu_blue(false, {});
  auto& active = *whole.GetLogicalVolume()->GetDaughter(0);
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  // TODO How can you verify the G4VisAttributes::Invisible attribute?
  CHECK(whole .GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::White());
  CHECK(active.GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::White());

}


TEST_CASE("hamamatsu app", "[app]") {

  // ----- Geometry ------------------------------------------------------------
  struct geometry : public G4VUserDetectorConstruction {
    geometry(std::string const& f) : filename{f} {}

    G4VPhysicalVolume* Construct() {
      auto air = nain4::material("G4_AIR");
      auto sipm = sipm_hamamatsu_blue(true, filename)->GetLogicalVolume();
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
    void Build         () const override { SetUserAction(new primary_generator); }
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

  // Retrieve hits stored in the sensitive detector
  auto sd = dynamic_cast<sipm_sensitive*>(nain4::find_logical("PHOTODIODES") -> GetSensitiveDetector());
  auto& detected_hits = sd -> hits;
  CHECK(detected_hits.size() == n_sipms);

  // Retrieve hits that were written out
  std::vector<hit_t> written_hits;
  hdf5_io h5io{hdf5_test_file_name};
  h5io.read_hit_info(written_hits);

  CHECK(written_hits.size() == detected_hits.size());

  for (auto [i, hit] : enumerate(detected_hits)) {
    auto row =  written_hits[i];
    auto pos = hit.GetPreStepPoint() -> GetPosition();
    // TODO: CHECK(row.event_id == ??);
    CHECK(row.x == pos.getX());
    CHECK(row.y == pos.getY());
    CHECK(row.z == pos.getZ());

    // TODO: Stupid checks, just to get something going. Replace with something
    // more intelligent

    // The z-plane in which the nearest part of the sensitive detectors is positioned.
    CHECK(pos.getZ() == Approx(29.7));
    // The particles were fired at x and y positions that were multiples of 7,
    // and the gun direction had no z-component, so the xs and ys should still
    // be multilpes of 7.
    auto x_over_7 = pos.getX(); CHECK(x_over_7 == (int)x_over_7);
    auto y_over_7 = pos.getY(); CHECK(y_over_7 == (int)y_over_7);
  }
}
