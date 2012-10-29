#include <string>
#include <set>
#include <gtest/gtest.h>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/intersection_array.hpp>

namespace {
using jellyfish::mer_dna;
typedef jellyfish::intersection_array<mer_dna> inter_array;
typedef std::set<mer_dna> mer_set;

void fill_sets(const int nb_mers, mer_set& all, mer_set& mult, mer_set& uniq) {
  mer_dna m;
  for(int i = 0; i < nb_mers; ++i) {
    m.randomize();
    all.insert(m);
    m.randomize();
    mult.insert(m);
    m.randomize();
    uniq.insert(m);
  }
}

void insert_mers(inter_array& ary, const int nb_mers, const mer_set& all, const mer_set& mult, const mer_set& uniq) {
  // First file. Contains the first third of uniq mers and the first
  // two thirds of the multiple mers.
  mer_set::const_iterator uit = uniq.begin();
  mer_set::const_iterator mit = mult.begin();
  for(mer_set::const_iterator it = all.begin(); it != all.end(); ++it)
    ary.add(*it);
  for(int i = 0; i < nb_mers / 3; ++i, ++uit, ++mit) {
    ary.add(*uit);
    ary.add(*mit);
  }
  mer_set::const_iterator mit1third = mit;
  for(int i = 0; i < nb_mers / 3; ++i, ++mit)
    ary.add(*mit);
  ary.postprocess(0, true);

  // Second file. Contains the second third of uniq mers and the last
  // two thirds of the multiple mers.
  for(mer_set::const_iterator it = all.begin(); it != all.end(); ++it)
    ary.add(*it);
  for(int i = 0; i < nb_mers / 3; ++i, ++uit)
    ary.add(*uit);
  for(mer_set::const_iterator it = mit1third; it != mult.end(); ++it)
    ary.add(*it);
  ary.postprocess(0, false);

  // Third file. Contains the last third of uniq mers, and the first
  // and last thirds of the multiple mers.
  for(mer_set::const_iterator it = all.begin(); it != all.end(); ++it)
    ary.add(*it);
  for( ; uit != uniq.end(); ++uit)
    ary.add(*uit);
  mer_set::const_iterator nmit = mult.begin();
  for(int i = 0; i < nb_mers / 3; ++i, ++nmit)
    ary.add(*nmit);
  for( ; mit != mult.end(); ++mit)
    ary.add(*mit);
  ary.postprocess(0, false);
}

TEST(IntersectionArray, Mers) {
  mer_dna::k(20);
  inter_array ary(1000, mer_dna::k() * 2, 1);
  static const int nb_mers = 100;
  mer_set all, mult, uniq;

  fill_sets(nb_mers, all, mult, uniq);
  insert_mers(ary, nb_mers, all, mult, uniq);
  ary.done();

  for(mer_set::const_iterator it = all.begin(); it != all.end(); ++it) {
    inter_array::mer_info info = ary[*it];
    EXPECT_TRUE(info.info.all);
    EXPECT_TRUE(info.info.mult);
    EXPECT_TRUE(info.info.uniq);
  }

  for(mer_set::const_iterator it = mult.begin(); it != mult.end(); ++it) {
    inter_array::mer_info info = ary[*it];
    EXPECT_TRUE(info.info.uniq);
    EXPECT_TRUE(info.info.mult);
    EXPECT_FALSE(info.info.all);
  }

  for(mer_set::const_iterator it = uniq.begin(); it != uniq.end(); ++it) {
    inter_array::mer_info info = ary[*it];
    EXPECT_TRUE(info.info.uniq);
    EXPECT_FALSE(info.info.mult);
    EXPECT_FALSE(info.info.all);
  }
}

TEST(IntersectionArray, Doubling) {
  mer_dna::k(20);
  inter_array ary_large(1000, mer_dna::k() * 2, 1);
  inter_array ary_small(100, mer_dna::k() * 2, 1);

  static const int nb_mers = 100;
  mer_set all, mult, uniq;

  fill_sets(nb_mers, all, mult, uniq);
  insert_mers(ary_large, nb_mers, all, mult, uniq);
  ary_large.done();
  insert_mers(ary_small, nb_mers, all, mult, uniq);
  ary_small.done();
}

} // namespace {
