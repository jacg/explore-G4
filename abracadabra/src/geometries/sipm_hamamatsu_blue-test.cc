#include "nain4.hh"

#include "geometries/sipm_hamamatsu_blue.hh"

#include <catch2/catch.hpp>

// visible = true
TEST_CASE("Hamamatsu blue", "[geometry][hamamatsu][blue]") {
  auto& whole  = *sipm_hamamatsu_blue(true);
  auto& active = *whole.GetLogicalVolume()->GetDaughter(0);
  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(whole), end(whole)) == 2);

  CHECK(whole .GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::Yellow());
  CHECK(active.GetLogicalVolume()->GetVisAttributes()->GetColour() == G4Colour::Blue());

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
