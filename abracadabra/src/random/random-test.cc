#include "random/random.hh"

#include <catch2/catch.hpp>


TEST_CASE("biased choice", "[random][biased][choice]") {

  // TODO Ideally this should be done with a proptesting generator/shrinker
  std::vector<G4double> weights{9.1, 1.2, 3.4, 100, 0.3, 12.4, 6.7};
  auto pick = biased_choice(weights);
  std::vector<unsigned> hits(weights.size(), 0);
  for (unsigned i=0; i<1000000; ++i) {
    hits[pick()] += 1;
  }

  // Verify that ratio of weights matches ratio of generated choices
  auto check = [&](auto l, auto r) {
    CHECK(static_cast<G4double>(hits[l]) /    hits[r] ==
                      Approx(weights[l]  / weights[r]).epsilon(0.01));
  };

  for (unsigned i=1; i<weights.size(); ++i) { check(0, i); }
}
