#include "utils/waveform.hh"

#include <catch2/catch.hpp>

TEST_CASE("waveform", "[utils][waveform]") {

  double dt = 1.23;
  double first = 10.3;
  waveform wf{dt};

  // Empty waveform
  auto data = wf.get();
  CHECK(data.dt == dt);
  CHECK(data.start_t.has_value() == false); // long-winded style for clarity
  CHECK(data.bins.size() == 0);

  // Create first bin by adding one datum
  wf.add(first);
  CHECK(data.start_t == first);
  CHECK(data.bins.size() == 1);
  CHECK(data.bins[0] == 1);

  // Add another datum to the first bin
  wf.add(first + dt / 3);
  CHECK(data.bins.size() == 1);
  CHECK(data.bins[0] == 2);

  // Lots more into the first bin
  for (int n=0; n<100; ++n) {
    wf.add(first + dt/3 + n*dt/200);
  }
  CHECK(data.bins.size() == 1);
  CHECK(data.bins[0] == 102);

  // Add to the next bin
  wf.add(first + dt);
  CHECK(data.bins.size() == 2);
  CHECK(data.bins[0] == 102);
  CHECK(data.bins[1] == 1);

  // Lots more into the second bin
  for (int n=0; n<100; ++n) {
    wf.add(first + dt + n*dt/200);
  }
  CHECK(data.bins.size() == 2);
  CHECK(data.bins[0] == 102);
  CHECK(data.bins[1] == 101);

  // Skip some bins
  size_t skip_to = 7;
  wf.add(first + dt*skip_to + dt/3);
  CHECK(data.bins.size() == skip_to+1);
  CHECK(data.bins[0] == 102);
  CHECK(data.bins[1] == 101);
  for (size_t n=2; n<skip_to; n++) {
    // The bins that have been skipped should be empty
    CHECK(data.bins[n] == 0);
  }
  CHECK(data.bins[skip_to] == 1);

  // Add more to first bin after skip
  wf.add(first + dt*skip_to + dt/2);
  CHECK(data.bins[skip_to] == 2);

  // Next bin after skipping should work too
  wf.add(first + dt*(skip_to+1) + dt/2);
  CHECK(data.bins[skip_to+1] == 1);

  // Check that the basic data still make sense at the end
  CHECK(data.dt == dt);
  CHECK(data.start_t == first);

  // TODO should `waveform.add` assert that the received times are monotonically
  // increasing? Or at least don't belong in an earlier bin?

}

TEST_CASE("waveform limit noskip", "[utils][waveform][limit]") {

  double dt = 4.56;
  double first = 9.71;
  size_t max_bins = 15;

  waveform wf{dt, max_bins};
  auto data = wf.get();
  for (size_t n=0; n<max_bins*100; n++) {
    wf.add(first + n*dt/3);
  }
  CHECK(data.bins.size() == max_bins);
}

TEST_CASE("waveform limit skip", "[utils][waveform][limit]") {

  double dt = 7.89;
  double first = 123.45;
  size_t max_bins = 15;

  waveform wf{dt, max_bins};
  auto data = wf.get();
  for (size_t n=max_bins/2; n<max_bins*100; n += max_bins) {
    wf.add(first + n*dt/3);
  }
  CHECK(data.bins.size() == max_bins);

}
