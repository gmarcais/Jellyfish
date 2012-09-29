#include <fstream>
#include <gtest/gtest.h>
#include <unit_test/test_main.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/time.hpp>

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
//::testing::Environment* const foo_env = ::testing::AddGlobalTestEnvironment(new RandomEnvironment);



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

  // speed tests
  uint64_t v1 = random_vector(VECLEN);
  uint64_t v2 = random_vector(VECLEN);
  uint64_t v3 = random_vector(VECLEN);
  uint64_t v4 = random_vector(VECLEN);
  uint64_t v5 = random_vector(VECLEN);
  uint64_t v6 = random_vector(VECLEN);
  uint64_t v7 = random_vector(VECLEN);
  uint64_t v8 = random_vector(VECLEN);
  uint64_t res_unrolled = 0, res_sse = 0;
  Time time1;
  const int nb_loops = 2560000;
  for(i = 0; i < nb_loops; i++) {
    res_unrolled ^= rand_m.times_unrolled(v1);
    res_unrolled ^= rand_m.times_unrolled(v2);
    res_unrolled ^= rand_m.times_unrolled(v3);
    res_unrolled ^= rand_m.times_unrolled(v4);
    res_unrolled ^= rand_m.times_unrolled(v5);
    res_unrolled ^= rand_m.times_unrolled(v6);
    res_unrolled ^= rand_m.times_unrolled(v7);
    res_unrolled ^= rand_m.times_unrolled(v8);
  }
  Time time2;
  for(i = 0; i < nb_loops; i++) {
    res_sse ^= rand_m.times_sse(v1);
    res_sse ^= rand_m.times_sse(v2);
    res_sse ^= rand_m.times_sse(v3);
    res_sse ^= rand_m.times_sse(v4);
    res_sse ^= rand_m.times_sse(v5);
    res_sse ^= rand_m.times_sse(v6);
    res_sse ^= rand_m.times_sse(v7);
    res_sse ^= rand_m.times_sse(v8);
  }
  Time time3;
  ASSERT_LT(time3 - time2, time2 - time1);
  // std::cout << "unrolled timing " << (time2 - time1).str() <<
  //   " sse timing " << (time3 - time2).str() << std::endl;
  ASSERT_EQ(res_unrolled, res_sse);
}
