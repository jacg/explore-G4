#include "trivial.hh"

#include <catch2/catch.hpp>

TEST_CASE("trivial dummy functions", "[two]") {
  REQUIRE(trivial::two  () == 2);
  REQUIRE(trivial::three() == 3);
}
