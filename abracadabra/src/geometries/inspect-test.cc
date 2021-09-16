#include <materials/LXe.hh>

#include <geometries/inspect.hh>

#include <nain4.hh>
#include <g4-mandatory.hh>

#include <G4Tubs.hh>
#include <G4Box.hh>
#include <G4SystemOfUnits.hh>

#include <catch2/catch.hpp>

using n4::material;
using n4::volume;
using n4::place;

TEST_CASE("geometry inspect", "[geometry][inspect]") {

  // A geometry with a variety of materials
  n4::geometry::construct_fn geometry = []() {
    auto air     = material("G4_AIR");
    auto steel   = material("G4_STAINLESS-STEEL");
    auto vacuum  = material("G4_Galactic");
    auto quartz  = quartz_with_properties();
    auto lxe     = LXe_with_properties();

    auto l = 10 * mm;
    auto v_air    = volume<G4Tubs>("Air"   , air   , 0.0, 10.0*mm, l, 0.0, 360*deg);
    auto v_steel  = volume<G4Tubs>("Steel" , steel , 0.0, 20.0*mm, l, 0.0, 360*deg);
    auto v_vacuum = volume<G4Tubs>("Vacuum", vacuum, 0.0, 30.0*mm, l, 0.0, 360*deg);
    auto v_quartz = volume<G4Tubs>("Quartz", quartz, 0.0, 40.0*mm, l, 0.0, 360*deg);
    auto v_lxe    = volume<G4Tubs>("LXe"   , lxe   , 0.0, 50.0*mm, l, 0.0, 360*deg);
    auto world    = volume<G4Box> ("World" , air   , 60*mm, 60*mm, 60*mm);

    place(v_air)   .in(v_steel) .now();
    place(v_steel) .in(v_vacuum).now();
    place(v_vacuum).in(v_quartz).now();
    place(v_quartz).in(v_lxe)   .now();
    place(v_lxe)   .in(world)   .now();
    return place(world)         .now();
  };

  auto run_manager = G4RunManager::GetRunManager();
  run_manager -> SetUserInitialization(new n4::geometry{geometry});

  // Inspector constructor will initialize the run (to close the geometry), and
  // attach itself to the run manager's world volume.
  world_geometry_inspector inspect{run_manager};

  // A couple of helpers, for writing the tests
  auto density_at = [&inspect](auto x) {
    return inspect.material_at({x*mm, 0, 0}) -> GetDensity() / (kg / m3);
  };

  auto name_at = [&inspect](auto x) {
    return inspect.volume_at({x*mm, 0, 0}) -> GetName();
  };

  // Verify that names and densities are as expected at various points in the geometry
  G4double x;
  x =  5; CHECK(name_at(x) == "Air")   ; CHECK(density_at(x) == Approx(   1.20479));
  x = 15; CHECK(name_at(x) == "Steel") ; CHECK(density_at(x) == Approx(8000.0));
  x = 25; CHECK(name_at(x) == "Vacuum"); CHECK(density_at(x) == Approx(1e-22));
  x = 35; CHECK(name_at(x) == "Quartz"); CHECK(density_at(x) == Approx(2320.0));
  x = 45; CHECK(name_at(x) == "LXe")   ; CHECK(density_at(x) == Approx(2980.0));

}
