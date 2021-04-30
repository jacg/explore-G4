
#include <G4Types.hh>

#include <optional>
#include <vector>


class waveform {
  template<class T> using opt = std::optional<T>;
public:
  waveform(G4double dt, opt<size_t> max_bins = {}, opt<size_t> size_hint = {});
  void add(G4double t);

  struct data {
    G4double const& dt;
    opt<G4double> const& start_t;
    std::vector<size_t> const& bins;
  };

  data get() { return data{dt, start_t, bins}; }

private:
  G4double dt;
  opt<G4double> start_t;
  G4double end_of_last_bin;
  std::optional<size_t> max_bins;
  std::vector<size_t> bins;
  bool reached_limit = false;

};
