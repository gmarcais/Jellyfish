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
      if(ary.add_rec(kv_pairs[k].first % GetParam().size,
                     kv_pairs[k].first, (uint64_t)1, false)) {
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
      ary.add_rec(*it % GetParam().size, *it, fval, false);
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
      bool res = ary.add_rec(*it % GetParam().size, *it, aval, false);
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
    this->ary.add_rec(i, fkey, (uint64_t)1, false);
    ASSERT_TRUE(this->ary.get_key_val(i, key, val));
    ASSERT_EQ(fkey, key);
    //    ASSERT_EQ(0u, overflows);
    ASSERT_EQ(1u, val);
  }

  for(size_t i = 0; i < GetParam().size; i++) {
    this->ary.add_rec(i, fkey, (uint64_t)1, false);
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
