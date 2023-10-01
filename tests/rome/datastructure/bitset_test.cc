#include "rome/datastructure/bitset.h"

#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace rome {
namespace datastructure {

TEST(BitsetTest, MatchesReference) {

  std::vector<bool> reference(4, false);
  uint8_bitset<4> bits;

  for(int j = 0; j < 4; ++j) {
    EXPECT_TRUE(reference[j] == bits.get(j));
  }
  for(int i = 0; i < 100000; ++i) {
    int idx = rand() % 4;
    bool val = rand() % 2;
    reference[idx] = val;
    bits.set(idx, val);

    for(int j = 0; j < 4; ++j) {
      EXPECT_TRUE(reference[j] == bits.get(j));
    }

  }
}

}  // namespace datastructure
}  // namespace rome
