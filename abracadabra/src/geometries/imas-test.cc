#include "nain4.hh"

#include "geometries/imas.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

TEST_CASE("IMAS demonstrator geometry", "[imas][geometry]") {

  using std::setw;

  auto& geometry = *imas_demonstrator();

  std::cout << std::endl;
  for (auto v: geometry) {
    std::cout << setw(15) << v->GetName() << ": ";
    auto l = v->GetLogicalVolume();
    std::cout
      << setw(18) << l->GetMaterial()->GetName()
      << setw(12) << G4BestUnit(l->GetMass(), "Mass")
      << setw(12) << G4BestUnit(l->GetSolid()->GetCubicVolume(), "Volume")
      << std::endl;
  }
  std::cout << std::endl;

  // Verify the number of volumes that make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 10);

  for (auto volume: geometry) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
  }
}
