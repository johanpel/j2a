#include <arrow/api.h>
#include <gtest/gtest.h>

#include <functional>

template <typename T>
struct Foo {
  std::function<bool(T)> Set;
};

struct Bar {
  static auto Set(Bar* foo, bool val) -> bool {
    foo->bar = val;
    return true;
  }

  bool bar = false;
};

struct Qux {
  auto Set(bool val) -> bool {
    bar = val;
    return true;
  }

  bool bar = false;
};

TEST(Foo, Bar) {
  Foo<bool> foo;
  Bar bar;
  foo.Set = std::bind(Bar::Set, &bar, std::placeholders::_1);
  foo.Set(true);
  std::cout << bar.bar << std::endl;

  Qux qux;
  auto q = std::mem_fn(&Qux::Set);
  q(qux, true);
  std::cout << qux.bar << std::endl;

  auto schema = arrow::schema({arrow::field(
      "voltage", arrow::list(arrow::field("item", arrow::uint64(), false)), false)});
}