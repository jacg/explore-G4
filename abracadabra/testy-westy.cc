#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

unsigned factorial(unsigned n) { return n <= 1 ? n : factorial(n - 1) * n; }


TEST_CASE("Factorials are computed", "[factorial]") {
  REQUIRE(factorial(0) ==   1);
  REQUIRE(factorial(1) ==   1);
  REQUIRE(factorial(2) ==   2);
  REQUIRE(factorial(3) ==   6);
  REQUIRE(factorial(4) ==  24);
  REQUIRE(factorial(5) == 120);
  REQUIRE(factorial(6) == 720);
}
