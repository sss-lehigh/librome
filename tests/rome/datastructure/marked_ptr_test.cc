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

  delete data;
}

}  // namespace datastructure
}  // namespace rome
