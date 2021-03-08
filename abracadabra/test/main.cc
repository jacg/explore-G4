#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("pure dummy functions", "[fart]") {
  REQUIRE(1+1 == 2);
}
