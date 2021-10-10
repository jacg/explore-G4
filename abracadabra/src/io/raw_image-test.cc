#include "io/raw_image.hh"

#include <catch2/catch.hpp>

TEST_CASE("raw_image roundtrip", "[io][raw_image]") {
  std::vector<float> pixels{1,2,3,4,5,6};
  raw_image original{{1,2,3}, {10,20,30}, pixels};
  std::string test_file_name = std::tmpnam(nullptr) + std::string("-test.raw");
  original.write(test_file_name);
  raw_image retrieved{test_file_name};

  auto const [nxa, nya, nza] = original.n_pixels();
  auto const [nxb, nyb, nzb] = retrieved.n_pixels();
  CHECK(nxa == nxb);
  CHECK(nya == nyb);
  CHECK(nza == nzb);

  auto const [dxa, dya, dza] = original .full_widths();
  auto const [dxb, dyb, dzb] = retrieved.full_widths();
  CHECK(dxa == dxb);
  CHECK(dya == dyb);
  CHECK(dza == dzb);

  CHECK(pixels == retrieved.data());
}
