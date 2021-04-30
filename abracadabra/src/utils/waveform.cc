#include <utils/waveform.hh>

waveform::waveform(G4double dt, opt<size_t> max_bins, opt<size_t> size_hint)
  : dt{dt}
  , start_t{}
  , max_bins{max_bins}
{
  bins.reserve(max_bins.value_or(size_hint.value_or(0)));
  // end_of_last_bin is initialized the first time `add` is called
}

void waveform::add(G4double t) {
  if (reached_limit) { return; }
  if (!start_t) {
    start_t = t;
    bins.push_back(0);
    end_of_last_bin = t + dt;
  }
  while (t >= end_of_last_bin) {
    if (max_bins && max_bins.value() == bins.size()) {
      reached_limit = true;
      return;
    }
    bins.push_back(0);
    end_of_last_bin += dt;
  }
  bins.back()++;
}
