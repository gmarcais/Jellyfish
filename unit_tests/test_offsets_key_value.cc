#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <jellyfish/offsets_key_value.hpp>

#ifndef UINT64_C
#define UINT64_C(x) ((uint64_t)x)
#endif

using namespace jellyfish;

typedef struct {
  uint_t        key_len, val_len;
} offset_test_param;

class ComputeOffsetsTest : public ::testing::TestWithParam<offset_test_param>
{
public:
  Offsets<uint64_t> offsets;

  ComputeOffsetsTest() :
    offsets(GetParam().key_len, GetParam().val_len, 15)
  { }

  ~ComputeOffsetsTest() { }
};

TEST_P(ComputeOffsetsTest, CheckCoherency) {
  const Offsets<uint64_t>::offset_t *it     = NULL, *pit = NULL;
  const Offsets<uint64_t>::offset_t *lit    = NULL, *lpit = NULL;
  uint_t                             k_len  = GetParam().key_len;
  uint_t                             v_len  = GetParam().val_len;
  uint_t                             kv_len = k_len + v_len;
  uint_t                             lk_len = 4;
  uint_t                             lv_len = kv_len - lk_len;
  uint_t                             i      = 0;
  uint64_t                          *w, *pw;
  
  //   /* Still missing:
  //    * - testing of value masks
  //    */
  EXPECT_EQ(lk_len, this->offsets.get_reprobe_len());
  EXPECT_EQ(lv_len, this->offsets.get_lval_len());
  
  for(i = 0; true; i++) {
    w = this->offsets.get_word_offset(i, &it, &lit, NULL);
    if(i > 0) {
      pw = this->offsets.get_word_offset(i-1, &pit, &lpit, NULL);
      
      uint_t noff = pit->key.boff + kv_len;
      if(noff > 64) {
        EXPECT_EQ(pit->key.woff + 1, it->key.woff) <<
          i << ": Word offset should go up";
      } else {
        EXPECT_EQ(pit->key.woff, it->key.woff) <<
          i << ": Word offset should NOT go up";
      }

      if(pit->key.mask2) { // key between two words
        EXPECT_LT((uint_t)64, pit->key.boff + k_len) <<
          i << ": Key not between words, should not have mask2";
      } else {
        EXPECT_GE((uint_t)64, pit->key.boff - 1 + k_len + 1) <<
          i << ": Key between words, should have mask2";
      } 
      EXPECT_EQ((pit->key.boff + kv_len + 1 + (pit->key.mask2 ? 2 : 0)) % 64,
                it->key.boff) <<
        i << ": Invalid jump of bit offset";
      EXPECT_EQ((pit->key.boff + kv_len + 1 + (pit->key.mask2 ? 2 : 0)) % 64,
                lit->key.boff) <<
        i << ": Invalid jump of bit offset large field";
    } // if(i > 0)
    
    EXPECT_EQ((it->key.boff + k_len + (it->key.mask2 ? 2 : 0)) % 64, it->val.boff) <<
      i << ": Invalid value offset";
    EXPECT_EQ((lit->key.boff + lk_len + (lit->key.mask2 ? 2 : 0)) % 64, lit->val.boff) <<
      i << ": Invalid value offset large field";
    EXPECT_EQ(UINT64_C(1) << (it->key.boff - 1), it->key.lb_mask) <<
      i << ": Invalid lb_mask";
    EXPECT_EQ(UINT64_C(1) << (lit->key.boff - 1), lit->key.lb_mask) <<
      i << ": Invalid lb_mask";
    if(it->key.mask2) {
      EXPECT_EQ(64 - it->key.boff - 1, it->key.shift) <<
        i << ": invalid key shift";
      EXPECT_EQ(UINT64_C(1) << (64 - it->key.boff + 1), 1 + (it->key.mask1 >> (it->key.boff - 1))) <<
        i << ": invalid key mask";
      EXPECT_EQ(UINT64_C(1) << (it->key.boff + k_len - 64 + 2), 1 + it->key.mask2) <<
        i << ": invalid key mask2";
      EXPECT_EQ(k_len, it->key.shift + it->key.cshift);
      EXPECT_EQ(UINT64_C(1) << 63, it->key.sb_mask1) <<
        i << ": invalid key key sb_mask1";
      EXPECT_EQ(UINT64_C(1) << it->key.cshift, it->key.sb_mask2) <<
        i << ": invalid key sb_mask2";
    } else {
      EXPECT_EQ(0u, it->key.shift) <<
        i << ": key shift should be zero, no mask";
      EXPECT_EQ(UINT64_C(1) << (k_len + 1), 1 + (it->key.mask1 >> (it->key.boff - 1))) <<
        i << ": invalid key mask " << it->key.boff;
      EXPECT_EQ(0u, it->key.cshift);
      EXPECT_EQ(0u, it->key.sb_mask1 | it->key.sb_mask2) <<
        i << ": set bit masks should be 0";
    }
    if(lit->key.mask2) {
      EXPECT_EQ(64 - lit->key.boff - 1, lit->key.shift) <<
        i << ": invalid key shift large field";
      EXPECT_EQ(UINT64_C(1) << (64 - lit->key.boff + 1), 1 + (lit->key.mask1 >> (lit->key.boff - 1))) <<
        i << ": invalid key mask large field";
      EXPECT_EQ(UINT64_C(1) << (lit->key.boff + lk_len - 64 + 2), 1 + lit->key.mask2) <<
        i << ": invalid key mask2 large field";
      EXPECT_EQ(lk_len, lit->key.shift + lit->key.cshift);
      EXPECT_EQ(UINT64_C(1) << 63, lit->key.sb_mask1) <<
        i << ": invalid key key sb_mask1";
      EXPECT_EQ(UINT64_C(1) << lit->key.cshift, lit->key.sb_mask2) <<
        i << ": invalid key sb_mask2";
    } else {
      EXPECT_EQ(0u, lit->key.shift) <<
        i << ": key shift should be zero, no mask, large field";
      EXPECT_EQ(0u, lit->key.cshift);
      EXPECT_EQ(UINT64_C(1) << (lk_len + 1), 1 + (lit->key.mask1 >> (lit->key.boff - 1))) <<
        i << ": invalid key mask large field " << lit->key.boff;
    }

    if(it->val.mask2) {
      EXPECT_LT((uint_t)64, it->val.boff + v_len) <<
        i << ": val not between words, should not have mask2";
      EXPECT_EQ(64 - it->val.boff, it->val.shift) <<
        i << ": invalid val shift";
      EXPECT_EQ(UINT64_C(1) << (64 - it->val.boff), 1 + (it->val.mask1 >> it->val.boff)) <<
        i << ": invalid val mask1";
      EXPECT_EQ(UINT64_C(1) << (it->val.boff + v_len - 64), 1 + it->val.mask2) <<
        i << ": invalid val mask2";
    } else {
      EXPECT_GE((uint_t)64, it->val.boff + v_len) <<
        i << ": val between words, should have mask2";
      EXPECT_EQ(v_len, it->val.shift) <<
        i << ": invalid val shift";
      EXPECT_EQ(UINT64_C(1) << v_len, 1 + (it->val.mask1 >> it->val.boff)) <<
        i << ": invalid val mask1";
    }
    EXPECT_EQ(v_len, it->val.shift + it->val.cshift);
    if(lit->val.mask2) {
      EXPECT_LT((uint_t)64, lit->val.boff + lv_len) <<
        i << ": val not between words, should not have mask2 large field";
      EXPECT_EQ(64 - lit->val.boff, lit->val.shift) <<
        i << ": invalid val shift large field";
      EXPECT_EQ(UINT64_C(1) << (64 - lit->val.boff), 1 + (lit->val.mask1 >> lit->val.boff)) <<
        i << ": invalid val mask1 large field";
      EXPECT_EQ(UINT64_C(1) << (lit->val.boff + lv_len - 64), 1 + lit->val.mask2) <<
        i << ": invalid val mask2 large field";
    } else {
      EXPECT_GE((uint_t)64, lit->val.boff + lv_len) <<
        i << ": val between words, should have mask2 large field " << lit->val.boff << " " << lv_len;
      EXPECT_EQ(lv_len, lit->val.shift) <<
        i << ": invalid val shift large field";
      EXPECT_EQ(UINT64_C(1) << lv_len, 1 + (lit->val.mask1 >> lit->val.boff)) <<
        i << ": invalid val mask1 large field";
    }
    EXPECT_EQ(lv_len, lit->val.shift + lit->val.cshift);
      
    uint_t noff = it->key.boff + kv_len;
    if(noff == 62 || noff == 63 || noff == 64)
      break;
  }
}

offset_test_param params[] = { {10, 10}, { 22, 5 }, {20, 5} };
INSTANTIATE_TEST_CASE_P(OffsetsTest, ComputeOffsetsTest, ::testing::ValuesIn(params));
