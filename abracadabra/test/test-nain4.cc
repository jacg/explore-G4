#include "nain4.hh"

#include <G4NistManager.hh>
#include <G4RunManager.hh>

#include <catch2/catch.hpp>

TEST_CASE("nain4", "[nain]") {

  auto run_manager = G4RunManager::GetRunManager();

  SECTION("NIST materials") {
    auto material_name = GENERATE("G4_AIR", "G4_WATER", "G4_H", "G4_A-150_TISSUE");
    auto nain_material = nain4::material(material_name);
    auto nist_material = G4NistManager::Instance()->FindOrBuildMaterial(material_name);
    REQUIRE(nain_material == nist_material);
    REQUIRE(nain_material != nullptr);
  }

}
