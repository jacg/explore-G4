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

  // nain4::material finds the same materials as the verbose G4 style
  SECTION("material NIST") {
    auto material_name = GENERATE("G4_AIR", "G4_WATER", "G4_H", "G4_A-150_TISSUE");
    auto nain_material = nain4::material(material_name);
    auto nist_material = G4NistManager::Instance()->FindOrBuildMaterial(material_name);
    REQUIRE(nain_material == nist_material);
    REQUIRE(nain_material != nullptr);
  }

  // Basic material properties make sense (except for solid water at RTP!)
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

  // nain4::volume produces objects with sensible sizes, masses, etc.
  SECTION("volume") {
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

  // nain4::place is a good replacement for G4PVPlacement
  SECTION("place") {
    auto air = nain4::material("G4_AIR");
    auto outer = nain4::volume<G4Box>("outer", air, 1*m, 2*m, 3*m);

    // Default values are sensible
    SECTION("defaults") {
      auto world = nain4::place(outer).now();

      auto trans = world->GetObjectTranslation();
      CHECK(trans == G4ThreeVector{});
      CHECK(world -> GetName()          == "outer");
      CHECK(world -> GetCopyNo()        == 0);
      CHECK(world -> GetLogicalVolume() == outer);
      CHECK(world -> GetMotherLogical() == nullptr);
    }

    // Multiple optional values can be set at once.
    SECTION("multiple options") {
      G4ThreeVector translation = {1,2,3};
      auto world = nain4::place(outer)
        .at(translation) // 1-arg version of at()
        .name("not outer")
        .id(382)
        .now();

      CHECK(world -> GetObjectTranslation() == translation);
      CHECK(world -> GetName()   == "not outer");
      CHECK(world -> GetCopyNo() == 382);
    }

    // The at() option accepts vector components (as well as a whole vector)
    SECTION("at 3-args") {
      auto world = nain4::place(outer).at(4,5,6).now(); // 3-arg version of at()
      CHECK(world->GetObjectTranslation() == G4ThreeVector{4,5,6});
    }

    // The in() option creates correct mother/daughter relationship
    SECTION("in") {
      auto water = nain4::material("G4_WATER");
      auto inner = nain4::volume<G4Box>("inner", water, 0.3*m, 0.2*m, 0.1*m);

      auto inner_placed = nain4::place(inner)
        .in(outer)
        .at(0.1*m, 0.2*m, 0.3*m)
        .now();

      auto outer_placed = nain4::place(outer).now();

      CHECK(inner_placed -> GetMotherLogical() == outer);
      CHECK(outer_placed -> GetLogicalVolume() == outer);
      CHECK(outer -> GetNoDaughters() == 1);
      CHECK(outer -> GetDaughter(0) -> GetLogicalVolume() == inner);

      // Quick check that geometry_iterator works TODO expand
      for (const auto& v: outer_placed) {
        std::cout << v.GetName() << std::endl;
      }

    }

  }
}
