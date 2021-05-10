#include <iostream>

// ----- The class we want to use ----------------------------------------------
class ready {
  ready(int f, int b) : f{f}, b{b} {}
  int f;
  int b;
public:
  void use() { std::cout << f << ' ' << b << std::endl; }
  friend class needs_bar;
  friend class needs_foo;
};

// ----- helpers: not fully-configured states ----------------------------------
struct needs_foo {
  needs_foo(int b) : b{b} {}
  ready foo(int f) { return {f, b}; }
private:
  int b;
};

struct needs_bar {
  needs_bar(int f) : f{f} {}
  ready bar(int b) { return {f, b}; }
private:
  int f;
};

// ----- interface entry point --------------------------------------------------
struct needs_foo_and_bar {
  needs_bar foo(int f) { return {f}; }
  needs_foo bar(int b) { return {b}; }
};

// ----- Client code that must specify foo and bar exactly once each ------------
int main() {
  needs_foo_and_bar()
    .foo(1)
    .bar(2)
    .use();
}
