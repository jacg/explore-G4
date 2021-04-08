#include "utils/enumerate.hh"

#include <catch2/catch.hpp>

TEST_CASE("enumerate", "[utils][enumerate]") {
  // NB, const is ignored and by-reference is implicit!
  SECTION("primitives") {
    std::vector<unsigned> stuff {5, 4, 3, 2, 1};
    for (const auto [n, el] : enumerate(stuff)) {
      CHECK(el == 5 - n);
      el++;
    }
    CHECK(stuff[0] == 6);
  }
  SECTION("objects") {
    struct Foo {
      Foo(unsigned n): n{n} {}
      unsigned n;
    };
    std::vector<Foo> stuff {2, 8};
    for (auto [n, el] : enumerate(stuff)) {
      if (n == 0) { CHECK(el.n == 2); }
      else        { CHECK(el.n == 8); }
      el.n += 1000;
    }
    CHECK(stuff[0].n == 1002);
    CHECK(stuff[1].n == 1008);
  }
}
