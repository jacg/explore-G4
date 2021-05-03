#include <vector>
#include <tuple>
#include <numeric>

template<class F> auto map(F f, std::vector<double> const& xs) {
  std::vector<double> ys(xs.size());
  transform(begin(xs), end(xs), begin(ys), f);
  return ys;
}

// TODO needs testing
template<class F> auto interpolate(F f, size_t N, double min, double max) {
  std::vector<double> xs(N);
  std::vector<double> ys(N);
  auto step = (max - min) / N;
  size_t n = 0;
  generate (begin(xs), end(xs), [min, step, &n](){ return  min + (n++ * step); });
  transform(begin(xs), end(xs), begin(ys), f);
  return make_tuple(xs, ys);
}
