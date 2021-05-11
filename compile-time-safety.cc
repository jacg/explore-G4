#include <iostream>

// ----- A class with two mandatory configurables ------------------------------
class ready {
  ready(int f, int b) : f{f}, b{b} {}
  int f;
  int b;
public:
  void use() { std::cout << f << ' ' << b << std::endl; }

  // ----- incompletely-specified states ---------------------------------------
private:
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
  // ----- entry point to the construction interface ----------------------------
public:
  struct make {
    needs_bar foo(int f) { return {f}; }
    needs_foo bar(int b) { return {b}; }
  };
};


// ----- Client code that must specify foo and bar exactly once each ------------
int main() {
  auto not_set = ready::make{};

  auto foo_set = not_set.foo(1);
  auto bar_set = not_set.bar(2);

  // compile-time errors:
  // foo_set.foo(666) ; // setting foo more than once
  // bar_set.bar(666) ; // setting bar more than once
  // not_set.use();     // neither foo nor bar set before use
  // foo_set.use();     // bar not set before use
  // bar_set.use();     // foo not set before use

  auto all_set_a = foo_set.bar(3); // foo already set, now setting bar
  auto all_set_b = bar_set.foo(4); // bar already set, now setting foo

  all_set_a.use(); // Can use because both foo and bar have been set
  all_set_b.use(); // Same again, though setting done in reverse order

  // ----- Single line versions ------------------------------

  // Compile-time errors

  // ready::make().use();   // foo and bar missing
  // ready::make().foo(1).use();  // bar missing
  // ready::make().bar(2).use();  // foo missing
  // ready::make().foo(1).foo(2); // foo conflict
  // ready::make().bar(1).bar(2); // bar conflict

  // Successes

  ready::make().foo(1).bar(2).use();
  ready::make().bar(1).foo(2).use();
}
