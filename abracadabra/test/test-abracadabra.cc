#include "nain4.hh"

#include "geometries/sipm_hamamatsu_blue.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

TEST_CASE("Hamamatsu blue", "[geometry][hamamatsu][blue]") {
  auto& geometry = *sipm_hamamatsu_blue();
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 2);

}
