#ifndef io_raw_hh
#define io_raw_hh

#include <string>
#include <tuple>
#include <vector>

#define g(i) std::get<i>(n)

class raw_image {
  using u = unsigned short;
  using f = float;
public:
  template<class P>
  raw_image(std::tuple<u,u,u> n, std::tuple<f,f,f> extent, P&& pixels) : n{n}, d{extent}, pixels{std::forward<P>(pixels)} {}
  raw_image(std::tuple<u,u,u> n, std::tuple<f,f,f> extent) : raw_image{n, extent, std::vector<f>(g(0) * g(1) * g(2), 0)} {}
  raw_image(std::string filename);
  void write(std::string filename);
  std::tuple<u,u,u> n_pixels   () const { return n; }
  std::tuple<f,f,f> full_widths() const { return d; }
  std::vector<f> const & data() const { return pixels; }
private:
  std::tuple<u,u,u> n;
  std::tuple<f,f,f> d;
  std::vector<f> pixels;
};

#undef g

#endif
