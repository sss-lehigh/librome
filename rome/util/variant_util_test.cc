#include "variant_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"

namespace rome {
namespace {

struct A {
  std::string s = "A";
};

struct B {
  std::string s = "B";
};

struct C {
  std::string s = "C";
};

TEST(VariantUtilTest, Test) {
  ROME_INIT_LOG();
  // Test plan:
  std::variant<A, B, C> var;
  var = A();
  auto o = overload{
      [](A& a) {
        ROME_INFO("{}", a.s);
        return true;
      },
      [](B& b) {
        ROME_INFO("{}", b.s);
        return true;
      },
      [](C& c) {
        ROME_INFO("{}", c.s);
        return true;
      },
  };
  EXPECT_TRUE(visit(var, o));

  var = B();
  EXPECT_TRUE(visit(var, o));
  var = C();
  EXPECT_TRUE(visit(var, o));
  EXPECT_TRUE(true);
}

}  // namespace
}  // namespace rome