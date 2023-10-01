#include "rome/datastructure/marked_ptr.h"

#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace rome {
namespace datastructure {

TEST(MarkedPtrTest, MatchesReference) {

  long long* data = new long long(0);
  marked_ptr<long long, 2> ptr(data);
  EXPECT_TRUE(ptr.is_valid());
  uint8_bitset<2> bits;

  // TODO write reference code

  for (int i = 0; i < 10000; ++i) {
    bool val = rand() % 1;
    int idx = rand() % 2;
    bits.set(idx, val);
    if (val) {
      ptr = ptr.set_marked(idx);
    } else {
      ptr = ptr.set_unmarked(idx);
    }
    EXPECT_TRUE(static_cast<long long*>(ptr) == data);
    EXPECT_TRUE(ptr.marks() == bits);

  }

  delete data;
}

}  // namespace datastructure
}  // namespace rome
