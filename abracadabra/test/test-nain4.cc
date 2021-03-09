#include "nain4.hh"

#include <G4NistManager.hh>
#include <G4RunManager.hh>

#include <catch2/catch.hpp>

TEST_CASE("nain4", "[nain]") {

  auto run_manager = G4RunManager::GetRunManager();

  SECTION("NIST materials") {
    auto name = "G4_AIR";
    auto nain_air = nain4::material(name);
    auto nist_air = G4NistManager::Instance()->FindOrBuildMaterial(name);
    REQUIRE(nain_air == nist_air);
  }

}
