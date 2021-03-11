#include "nain4.hh"

#include "action_initialization.hh"
#include "detector_construction.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisManager.hh>

#include <G4SystemOfUnits.hh>

#include <QBBC.hh>
#include <Randomize.hh>

#include <catch2/catch.hpp>

#include <algorithm>
#include <iterator>

TEST_CASE("abracadabra", "[abra]") {

  auto geometry = detector_construction{}.Construct();

  for (const auto& v: geometry) {
    std::cout << v.GetName() << ": ";
    auto & l = *v.GetLogicalVolume();
    std::cout
      << l.GetMaterial()->GetName() << "  "
      << l.GetMass() / kg << " kg  "
      << l.GetSolid()->GetCubicVolume() / m3 << " m3  "
      << std::endl;
  }

  // 4 volumes make up the geometry
  CHECK(std::distance(begin(geometry), end(geometry)) == 4);


}
