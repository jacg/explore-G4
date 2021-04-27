#include <vector>
#include <tuple>
#include <numeric>

template<class F> auto map(F f, std::vector<double> const& xs) {
  std::vector<double> ys(xs.size());
  transform(begin(xs), end(xs), begin(ys), f);
  return ys;
}

template<class F> auto interpolate(F f, size_t N, double min, double max) {
  std::vector<double> xs(N);
  std::vector<double> ys(N);
  iota(begin(xs), end(xs), (max - min) / N);
  transform(begin(xs), end(xs), begin(ys), f);
  return make_tuple(xs, ys);
}
