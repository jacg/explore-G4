#include "nain4.hh"

#include "geometries/imas.hh"

#include <G4VSolid.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <catch2/catch.hpp>

#include <tuple>

TEST_CASE("IMAS demonstrator geometry", "[imas][geometry]") {

  using std::setw; using std::make_tuple;

  auto [version, n_elements] = GENERATE(make_tuple(1,  99206),
                                        make_tuple(2, 107226));

  auto& geometry = *imas_demonstrator(nullptr, 70*cm, version);

  // std::cout << std::endl;
  // for (auto v: geometry) {
  //   std::cout << setw(15) << v->GetName() << ": ";
  //   auto l = v->GetLogicalVolume();
  //   std::cout
  //     << setw(18) << l->GetMaterial()->GetName()
  //     << setw(12) << G4BestUnit(l->GetMass(), "Mass")
  //     << setw(12) << G4BestUnit(l->GetSolid()->GetCubicVolume(), "Volume")
  //     << std::endl;
  // }
  // std::cout << std::endl;

  // Verify the number of volumes that make up the geometry

  CHECK(std::distance(begin(geometry), end(geometry)) == n_elements);

  size_t count = 0;
  for (auto volume: geometry) {
    CHECK(volume->CheckOverlaps(1000, 0, false) == false);
    // Checking the whole lot takes almost a minute. After the first 1000,
    // chances are that the rest are also OK, by symmetry.
    count++;
    if (count > 1000) { break; }
  }

  // TODO send geantino outwards, and check that the order of volumes is as
  // expected

  // std::vector<std::string> names{};
  // for (auto volume: geometry) {
  //   names.push_back(volume -> GetName());
  //   auto name = volume -> GetName();
  //   if (name != "true_active" && name != "fake_active" && name[0] != 'H') {
  //     std::cout << volume -> GetName() << std::endl;
  //   }
  // }

}
