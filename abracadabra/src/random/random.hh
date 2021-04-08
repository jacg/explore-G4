#ifndef random_random_hh
#define random_random_hh

#include <G4Types.hh>

#include <vector>

class biased_choice {
public:

  biased_choice(std::vector<G4double> weights);
  unsigned operator()();

private:
  std::vector<G4double> prob;
  std::vector<unsigned> topup;
};

#endif
