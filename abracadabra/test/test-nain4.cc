#include "nain4.hh"

#include <G4Box.hh>
#include <G4Cons.hh>
#include <G4Trd.hh>

#include <G4NistManager.hh>
#include <G4RunManager.hh>

#include <G4SystemOfUnits.hh>

#include <catch2/catch.hpp>

// Many of the tests below check physical quantities. Dividing physical
// quantities by their units gives raw numbers which are easily understandable
// by a human reader, which is important test failures are reported. Sometimes
// this gives rise to the apparently superfluous division by the same unit on
// both sides of an equation, in the source code.


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

  SECTION("logical volumes") {
      auto water = nain4::material("G4_WATER");
      auto lx = 1 * m;
      auto ly = 2 * m;
      auto lz = 3 * m;
      auto box = nain4::volume<G4Box>("test_box", water, lx, ly, lz);
      auto density = water->GetDensity();
      CHECK(box->TotalVolumeEntities() == 1);
      CHECK(box->GetMass() / kg        == Approx(8 * lx * ly * lz * density / kg));
      CHECK(box->GetMaterial()         == water);
      CHECK(box->GetName()             == "test_box");

      auto solid = box->GetSolid();
      CHECK(solid->GetCubicVolume() / m3 == Approx(8 *  lx    * ly    * lz     / m3));
      CHECK(solid->GetSurfaceArea() / m2 == Approx(8 * (lx*ly + ly*lz + lz*lx) / m2));
      CHECK(solid->GetName()             == "test_box");
  }
}
