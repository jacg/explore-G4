#include "nain4.hh"

#include "action_initialization.hh"
#include "detector_construction.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisManager.hh>

#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

#include <QBBC.hh>
#include <Randomize.hh>

#include <catch2/catch.hpp>

#include <algorithm>
#include <iterator>

TEST_CASE("abracadabra", "[abra]") {

  using std::setw;

  auto geometry = detector_construction{}.Construct();

  std::cout << std::endl;
  for (const auto& v: geometry) {
    std::cout << setw(15) << v.GetName() << ": ";
    auto & l = *v.GetLogicalVolume();
    std::cout
      << setw(18) << l.GetMaterial()->GetName()
      << setw(12) << G4BestUnit(l.GetMass(), "Mass")
      << setw(12) << G4BestUnit(l.GetSolid()->GetCubicVolume(), "Volume")
      << std::endl;
  }
  std::cout << std::endl;

  // 4 volumes make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 8);


}
