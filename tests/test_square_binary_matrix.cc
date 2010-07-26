#include <gtest/gtest.h>
#include <square_binary_matrix.hpp>
#include <fstream>

class RandomEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {
    std::ifstream dev_rand("/dev/urandom");
    unsigned int seed;
    dev_rand.read((char *)&seed, sizeof(seed));
    std::cout << "Seed: " << seed << std::endl;
    dev_rand.close();
    srandom(seed);
  }
};
::testing::Environment* const foo_env = ::testing::AddGlobalTestEnvironment(new RandomEnvironment);

uint64_t random_vector(int length) {
  uint64_t _mask = (((uint64_t)1) << length) - 1;
  uint64_t res = 0;
  int lrm = floorLog2((unsigned long)RAND_MAX);
  int i;
  for(i = 0; i < length; i += lrm) 
    res ^= (uint64_t)random() << i;
  return res & _mask;
}


#define VECLEN 44
TEST(SquareBinaryMatrix, Initialization) {
  SquareBinaryMatrix small_id_m(8);

  small_id_m.init_identity();
  ASSERT_STREQ("8x8\n10000000\n01000000\n00100000\n00010000\n00001000\n00000100\n00000010\n00000001\n",
               small_id_m.str().c_str());
  SquareBinaryMatrix id_m(VECLEN);
  SquareBinaryMatrix rand_m(VECLEN);

  id_m.init_identity();
  ASSERT_TRUE(id_m.is_identity());
  ASSERT_EQ(VECLEN, id_m.get_size());

  rand_m.init_random();
  ASSERT_EQ(VECLEN, rand_m.get_size());

  SquareBinaryMatrix rand2_m(5);
  rand2_m = rand_m;
  ASSERT_TRUE(rand_m == rand2_m);
  ASSERT_FALSE(rand_m != rand2_m);

  SquareBinaryMatrix rand3_m = rand_m;
  ASSERT_TRUE(rand_m == rand3_m);
  ASSERT_FALSE(rand_m != rand3_m);

  uint64_t v = random_vector(VECLEN);
  ASSERT_EQ(v, id_m.times(v));

  ASSERT_TRUE(rand_m == rand_m.transpose().transpose());
  ASSERT_TRUE(id_m == id_m.transpose());

  SquareBinaryMatrix sym_m = rand_m.transpose() * rand_m;
  ASSERT_TRUE(sym_m == sym_m.transpose());

  ASSERT_FALSE(rand_m[0] == v); // Could fail with probability 2**-64!!
  rand_m[0] = v;
  ASSERT_TRUE(rand_m[0] == v);

  ASSERT_TRUE(rand_m == (id_m * rand_m));

  int i, regular = 0, singular = 0;
  for(i = 0; i < 10; i++) {
    SquareBinaryMatrix tmp_m(32);
    tmp_m.init_random();
    try {
      SquareBinaryMatrix inv_m = tmp_m.inverse();
      SquareBinaryMatrix sbi_m = inv_m * tmp_m;
      regular++;
      ASSERT_TRUE(sbi_m.is_identity())
	<< "Not identity\n" << sbi_m.str() << std::endl;
    } catch(SquareBinaryMatrix::SingularMatrix e) {
      singular++;
    }
  }
  ASSERT_EQ(10, regular + singular);

  for(i = 0; i < 100; i++) {
    v = random_vector(VECLEN);
    ASSERT_EQ(rand_m.times_loop(v), rand_m.times_unrolled(v));
#ifdef SSE
    ASSERT_EQ(rand_m.times_loop(v), rand_m.times_sse(v));
#endif
  }
}
