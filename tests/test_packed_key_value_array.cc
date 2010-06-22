#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <storage.hpp>
#include <packed_key_value_array.hpp>
#include <hash.hpp>

#include <atomic_gcc.hpp>
#include <allocators_malloc.hpp>
#include <math.h>

using namespace jellyfish;

/* Helper functions.
 */
template<typename T>
class Log2Test : public ::testing::Test
{
public:
  T x;

  Log2Test() : x(1) { }
};

typedef ::testing::Types<uint64_t> IntegerTypes;

TYPED_TEST_CASE(Log2Test, IntegerTypes);

TYPED_TEST(Log2Test, Logs) {
  for(uint_t i = 0; i < bsizeof(TypeParam); i++) {
    ASSERT_EQ(i, floorLog2(this->x));
    ASSERT_EQ(i+1, bitsize(this->x));
    this->x = (this->x << 1) | (rand() > 0.5 ? 1 : 0);
  }
}

typedef struct {
  uint_t        key_len, val_len;
} offset_test_param;

class ComputeOffsetsTest : public ::testing::TestWithParam<offset_test_param>
{
public:
  packed_key_value::Offsets<uint64_t> offsets;

  ComputeOffsetsTest() :
    offsets(GetParam().key_len, GetParam().val_len, 15)
  { }

  ~ComputeOffsetsTest() { }
};

TEST_P(ComputeOffsetsTest, CheckCoherency) {
  packed_key_value::Offsets<uint64_t>::offset_t *it = NULL, *pit = NULL, 
    *lit = NULL, *lpit = NULL;
  uint_t k_len = GetParam().key_len;
  uint_t v_len = GetParam().val_len;
  uint_t kv_len = k_len + v_len;
  uint_t lk_len = 4;
  uint_t lv_len = kv_len - lk_len;
  uint_t i = 0, *w, *pw;
  
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
        EXPECT_LT(64u, pit->key.boff + k_len) <<
          i << ": Key not between words, should not have mask2";
      } else {
        EXPECT_GE(64u, pit->key.boff - 1 + k_len + 1) <<
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
      EXPECT_LT(64u, it->val.boff + v_len) <<
        i << ": val not between words, should not have mask2";
      EXPECT_EQ(64 - it->val.boff, it->val.shift) <<
        i << ": invalid val shift";
      EXPECT_EQ(UINT64_C(1) << (64 - it->val.boff), 1 + (it->val.mask1 >> it->val.boff)) <<
        i << ": invalid val mask1";
      EXPECT_EQ(UINT64_C(1) << (it->val.boff + v_len - 64), 1 + it->val.mask2) <<
        i << ": invalid val mask2";
    } else {
      EXPECT_GE(64u, it->val.boff + v_len) <<
        i << ": val between words, should have mask2";
      EXPECT_EQ(v_len, it->val.shift) <<
        i << ": invalid val shift";
      EXPECT_EQ(UINT64_C(1) << v_len, 1 + (it->val.mask1 >> it->val.boff)) <<
        i << ": invalid val mask1";
    }
    EXPECT_EQ(v_len, it->val.shift + it->val.cshift);
    if(lit->val.mask2) {
      EXPECT_LT(64u, lit->val.boff + lv_len) <<
        i << ": val not between words, should not have mask2 large field";
      EXPECT_EQ(64 - lit->val.boff, lit->val.shift) <<
        i << ": invalid val shift large field";
      EXPECT_EQ(UINT64_C(1) << (64 - lit->val.boff), 1 + (lit->val.mask1 >> lit->val.boff)) <<
        i << ": invalid val mask1 large field";
      EXPECT_EQ(UINT64_C(1) << (lit->val.boff + lv_len - 64), 1 + lit->val.mask2) <<
        i << ": invalid val mask2 large field";
    } else {
      EXPECT_GE(64u, lit->val.boff + lv_len) <<
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

typedef struct {
  size_t size, entries, iterations;
  uint_t key_len, val_len, reprobe_limit;
} packed_counting_param;

class PackedCountingTest : public ::testing::TestWithParam<packed_counting_param>
{
public:
  typedef packed_key_value::array<uint64_t,atomic::gcc<uint64_t>,allocators::malloc> pary_t;
  typedef hash< uint64_t,uint64_t,pary_t,atomic::gcc<uint64_t> > pch_t;
  typedef std::pair<uint64_t,uint64_t> kv_t;
  pary_t			 ary;
  pch_t				 pch;
  kv_t			*kv_pairs;
  std::map<uint64_t,size_t>	 key_entry_map;
  

  PackedCountingTest() : 
    ary(GetParam().size, GetParam().key_len, GetParam().val_len,
	GetParam().reprobe_limit, quadratic_reprobes),
    pch(&ary)
  { }

  void random_keys_fill() {
    uint64_t nkey;
    uint64_t key_mask = (1 << GetParam().key_len) - 1;
    kv_pairs = new kv_t[GetParam().entries];

    for(size_t i = 0; i < GetParam().entries; i++) {
      do {
        nkey = rand() & key_mask;
      } while(nkey == 0 || key_entry_map.find(nkey) != key_entry_map.end());
      kv_pairs[i] = kv_t(nkey, 0);
      key_entry_map[kv_pairs[i].first] = i;
    }
    
    for(size_t i = 0; i < GetParam().iterations; i++) {
      size_t k = exp_rand(0.5, GetParam().entries);
      if(ary.add(kv_pairs[k].first % GetParam().size, kv_pairs[k].first, (uint64_t)1)) {
	kv_pairs[k].second++;
      }
    }
  }

  void large_fields_fill() {
    std::vector<uint64_t> keys;
    uint64_t key_mask = (1 << GetParam().key_len) - 1;
    uint64_t fval = (((uint64_t)1) << GetParam().val_len) - 1;
    uint64_t nkey, nval;
    kv_pairs = new kv_t[GetParam().entries];

    keys.reserve(GetParam().entries);

    for(size_t i = 0; i < GetParam().entries; i++) {
      do {
        nkey = rand() & key_mask;
      } while(nkey == 0 || key_entry_map.find(nkey) != key_entry_map.end());
      keys.push_back(nkey);
      key_entry_map[nkey] = i;
    }

    std::vector<uint64_t> shuff_keys = keys;
    random_shuffle(shuff_keys.begin(), shuff_keys.end());
    for(std::vector<uint64_t>::const_iterator it = shuff_keys.begin();
	it != shuff_keys.end();
	it++) {
      ary.add(*it % GetParam().size, *it, fval);
      ASSERT_TRUE(ary.get_val(*it % GetParam().size,
			      *it, nval)) << std::hex <<
	"Key not set " << (it - shuff_keys.begin()) << " key " << *it;
      ASSERT_EQ(fval, nval) <<
	"Val not set properly " << (it - shuff_keys.begin()) << " key " << *it;
		  
    }

    for(std::vector<uint64_t>::const_iterator it = keys.begin();
	it != keys.end();
	it++) {
      uint64_t aval = rand() & 7;
      bool res = ary.add(*it % GetParam().size, *it, aval);
      kv_pairs[it - keys.begin()] = 
	kv_t(*it, fval + (res ? aval : 0));
      ASSERT_TRUE(ary.get_val(*it % GetParam().size,
			      *it, nval, true)) << std::hex <<
	"Key not set " << (it - keys.begin()) << " key " << *it;
      ASSERT_EQ(fval + (res ? aval : 0), nval) << std::hex <<
	"Val not set properly " << (it - keys.begin()) << " key " << *it;
    }    
  }

  ~PackedCountingTest() {}

  // Exponantial distribution with parameter p, truncated into [0, max-1]
  size_t exp_rand(double p, size_t max) {
    size_t k;
    do {
      k = (size_t)ceil(log(1 - (((double)rand()) / RAND_MAX)) / log(p)) - 1;
    } while(k >= max);
    return k;
  }
};

TEST_P(PackedCountingTest, RandomIncrementsIterator) {
  this->random_keys_fill();
  ASSERT_EQ(GetParam().size, this->ary.get_size());
  pch_t::iterator                      it = this->pch.iterator_all();
  std::map<uint64_t, size_t>::iterator  key_id;
  kv_t                           kv_pair;

  while(it.next()) {
    key_id = this->key_entry_map.find(it.key);
    ASSERT_FALSE(key_entry_map.end() == key_id) <<
      "Unknown key in hash: " << std::hex << it.key << " id " << it.id;
    kv_pair = kv_pairs[key_id->second];
    ASSERT_EQ(kv_pair.first, it.key) <<
      "Bad initialization";
    ASSERT_EQ(kv_pairs[key_id->second].second, it.val) <<
      "Error in count for key: " << it.key;
  }

//   for(size_t i = 0; i < GetParam().entries; i++) {
//     ASSERT_EQ((uint64_t)0, kv_pairs[i].second) <<
//       "Error in count for key: " << i << " " << std::hex << kv_pairs[i].first;
//   }
}

// Not neede anymore. Only one type of iterator already tested in previous TEST_P
// TEST_P(PackedCountingTest, RandomIncrementsFullIterator) {
//   this->random_keys_fill();
//   ASSERT_EQ(GetParam().size, this->pch.size());
//   pch_t::full_iterator                  it = this->pch.full_all();
//   std::map<uint64_t, size_t>::iterator  key_id;
//   kv_t                           kv_pair;

//   while(it.next()) {
//     key_id = this->key_entry_map.find(it.key);
//     ASSERT_FALSE(key_entry_map.end() == key_id) <<
//       "Unknown key in hash: " << std::hex << it.key << " id " << it.id;
//     kv_pair = kv_pairs[key_id->second];
//     ASSERT_EQ(kv_pair.first, it.key) <<
//       "Bad initialization";
//     ASSERT_EQ(kv_pairs[key_id->second].second, it.val) <<
//       "Error in count for key: " << std::hex << it.key;
//   }
// }

TEST_P(PackedCountingTest, FullKeyIncrements) {
  uint64_t fkey = (((uint64_t)1) << GetParam().key_len) - 1;
  uint64_t key, val;
  //  uint_t   overflows;

  for(size_t i = 0; i < GetParam().size; i++) {
    this->ary.add(i, fkey, (uint64_t)1);
    ASSERT_TRUE(this->ary.get_key_val(i, key, val));
    ASSERT_EQ(fkey, key);
    //    ASSERT_EQ(0u, overflows);
    ASSERT_EQ(1u, val);
  }

  for(size_t i = 0; i < GetParam().size; i++) {
    this->ary.add(i, fkey, (uint64_t)1);
    ASSERT_TRUE(this->ary.get_key_val(i, key, val));
    ASSERT_EQ(fkey, key);
    //    ASSERT_EQ(0u, overflows);
    ASSERT_EQ(2u, val);
  }
}

TEST_P(PackedCountingTest, LargeFields) {
  this->large_fields_fill();
  ASSERT_EQ(GetParam().size, this->pch.get_size());

  pch_t::iterator it = this->pch.iterator_all();
  std::map<uint64_t, size_t>::iterator  key_id;
  kv_t                           kv_pair;  

  while(it.next()) {
    key_id = this->key_entry_map.find(it.key);
    ASSERT_FALSE(key_entry_map.end() == key_id) <<
      "Unknown key in hash: " << std::hex << it.key << " id " << it.id;
    kv_pair = kv_pairs[key_id->second];
    ASSERT_EQ(kv_pair.first, it.key) <<
      "Bad initialization";
    ASSERT_EQ(kv_pairs[key_id->second].second, it.val) <<
      "Error in count for key: " << std::hex << it.key;
  }
}

packed_counting_param pc_params[] = {
  { 32, 16, 32, 20, 3, 15}, // size, entries, iterations, key_len, val_len, reprobe-limit
  { 32, 16, 1024, 20, 3, 15},
  { 64, 40, 2048, 22, 3, 7}
};
INSTANTIATE_TEST_CASE_P(SingleThreadTest, PackedCountingTest,
                        ::testing::ValuesIn(pc_params));
