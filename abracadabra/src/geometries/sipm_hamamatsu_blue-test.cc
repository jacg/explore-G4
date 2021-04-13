#include "nain4.hh"

#include "geometries/sipm_hamamatsu_blue.hh"
#include "writer/persistency_manager.hh"

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
  auto& whole  = *sipm_hamamatsu_blue(true);
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
  auto& whole  = *sipm_hamamatsu_blue(false);
  auto& active = *whole.GetLogicalVolume()->GetDaughter(0);
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  // TODO How can you verify the G4VisAttributes::Invisible attribute?
  CHECK(whole .GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::White());
  CHECK(active.GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::White());

}


TEST_CASE("hamamatsu app", "[app]") {

  // ----- Geometry ------------------------------------------------------------
  class geometry : public G4VUserDetectorConstruction {
    G4VPhysicalVolume* Construct() {
      auto air = nain4::material("G4_AIR");
      auto sipm = sipm_hamamatsu_blue(true)->GetLogicalVolume();
      auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
      for (int x=-35; x<35; x+=7) {
        for (int y=-35; y<35; y+=7) {
          nain4::place(sipm).in(world).at(x*mm, y*mm, 30*mm).now();
        }
      }
      return nain4::place(world).now();

    }
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

  // Add persistency manager
  auto persistency = new persistency_manager{};

  // ----- Initialize and run Geant4 ------------------------------------------
  {
    nain4::silence _{G4cout};
    auto run_manager = G4RunManager::GetRunManager();
    run_manager -> SetUserInitialization(new geometry);
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


#define DBG(stuff) std::cout << stuff << std::endl;

  DBG("before dynamic_caste");
  auto sd = dynamic_cast<hamamatsu_sensitive*>(nain4::find_logical("PHOTODIODES") -> GetSensitiveDetector());
  DBG("got sd");
  auto& hits_collection = *(sd -> hits);
  //hits_collection.PrintAllHits();
  DBG("got hits_collection");
  auto hits = hits_collection . GetVector();
  DBG("got hits");
  DBG(hits << " size: " << hits->size());
  auto one_hit = hits->operator[](0);
  DBG("got one hit");
  auto pos = one_hit -> get_position();
  DBG("got position");
  //DBG(pos);

  // TODO: check total number of hits
  for (auto hit : *hits) {
    DBG("loop: got one hit");
    // TODO: Stupid checks, just to get something going. Replace with something
    // more intelligent
    DBG(&hit);
    auto pos = hit -> get_position();

    // The z-plane in which the nearest part of the sensitive detectors is positioned.
    CHECK(pos.getZ() == Approx(30.2));
    // The particles were fired at x and y positions that were multiples of 7,
    // and the gun direction had no z-component, so the xs and ys should still
    // be multilpes of 7.
    auto x_over_7 = pos.getX(); CHECK(x_over_7 == (int)x_over_7);
    auto y_over_7 = pos.getY(); CHECK(y_over_7 == (int)y_over_7);
  }
  DBG("ZZZZZZZZZZZZZZZZZ");
}
