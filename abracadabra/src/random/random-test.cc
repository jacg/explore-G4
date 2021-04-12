#include "random/random.hh"

#include <CLHEP/Units/SystemOfUnits.h>
#include <catch2/catch.hpp>


TEST_CASE("biased choice", "[random][biased][choice]") {

  // TODO Ideally this should be done with a proptesting generator/shrinker
  std::vector<G4double> weights{9.1, 1.2, 3.4, 100, 0.3, 12.4, 6.7};
  auto pick = biased_choice(weights);
  std::vector<size_t> hits(weights.size(), 0);
  for (size_t i=0; i<1000000; ++i) {
    hits[pick()] += 1;
  }

  // Verify that ratio of weights matches ratio of generated choices
  auto check = [&](auto l, auto r) {
    CHECK(static_cast<G4double>(hits[l]) /    hits[r] ==
                      Approx(weights[l]  / weights[r]).epsilon(0.01));
  };

  for (size_t i=1; i<weights.size(); ++i) { check(0, i); }
}



// Ratio of volumes of two spherical shells of equal thickness, the inner-radius
// of the larger being equal to the outer radius of the smaller
//
// Shpere volume         proportional to  n^3           in units of shell thickness
// Smaller sphere volume proportional to        (n-1)^3
// Shell volume          proportional to  n^3 - (n-1)^3
// Ratio of touching shells = (n^3 - (n-1)^3) / ((n-1)^3 - (n-2)^3)
// A bit of algebra simplifies this to the following
double shell_ratio(double n) {
  return
    (3 * n * (n-1) + 1) /
    (3 * n * (n-3) + 7);
}


TEST_CASE("random point in sphere", "[random][sphere]") {
  using CLHEP::twopi;
  G4double const r_max = 3.456;
  size_t   const N_bins = 10;
  size_t   const N_per_bin = 1e5;
  std::vector<double> r_hits(N_bins, 0); // Concentric shells
  std::vector<size_t> x_hits(N_bins, 0); // Equal-angle wedge-bins around x-axis
  std::vector<size_t> y_hits(N_bins, 0); //                               y
  std::vector<size_t> z_hits(N_bins, 0); //                               z

  // ----- Helpers to identify in which bin to place the points ---------
  auto radius_bin = [=](auto r)         { return floor(N_bins *         r   / r_max       ); };
  auto  angle_bin = [=](auto x, auto y) { return floor(N_bins * (atan2(x,y) / twopi + 0.5)); };

  // ----- Collect samples ----------------------------------------------
  size_t const N_SAMPLES = N_per_bin * N_bins;
  for (size_t i=0; i<N_SAMPLES; ++i) {
    auto pt = random_in_sphere(r_max);
    auto [x, y, z] = std::make_tuple(pt.x(), pt.y(), pt.z());
    auto r = pt.mag();
    r_hits[radius_bin( r  )]++;
    x_hits[ angle_bin(y, z)]++;
    y_hits[ angle_bin(z, x)]++;
    z_hits[ angle_bin(x, y)]++;
  }

  // ----- Check distribution in concentric shells ----------------------
  for (size_t n=1; n<N_bins; ++n) {
    CHECK(r_hits[n] / r_hits[n-1] == Approx(shell_ratio(n+1)).epsilon(0.02));
  }

  // ----- Check angular distribution around each axis ------------------
  auto check_around_axis = [=](auto const& bin) {
    for (size_t n = 0; n < N_bins; ++n) {
      CHECK(bin[n] == Approx(N_per_bin).epsilon(0.01));
    }
  };

  check_around_axis(x_hits);
  check_around_axis(y_hits);
  check_around_axis(z_hits);

}
