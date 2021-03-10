#include "nain4.hh"

#include <G4Box.hh>
#include <G4Cons.hh>
#include <G4Trd.hh>

#include <G4NistManager.hh>
#include <G4RunManager.hh>

#include <G4SystemOfUnits.hh>

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

  SECTION("material properties") {
    SECTION("water") {
      auto water = nain4::material("G4_WATER");
      CHECK(water->GetName()                  == "G4_WATER");
      CHECK(water->GetChemicalFormula()       == "H_2O");
      CHECK(water->GetTemperature() /  kelvin == Approx(293.15));
      CHECK(water->GetPressure() / atmosphere == Approx(1));
      CHECK(water->GetDensity() /     (kg/m3) == Approx(1000));
      CHECK(water->GetState()                 == G4State::kStateSolid); // WTF!?
    }
  }
}
