#ifndef __RANDOM_SEED_HPP__
#define __RANDOM_SEED_HPP__

#include <random>
#include <fstream>
#include <algorithm>
#include <array>

// Seed a random generator
template <typename EngineT, std::size_t StateSize = EngineT::state_size>
void
seed_prg(EngineT& engine, const char* save = nullptr, const char* load = nullptr)
{
  using          engine_type    = typename EngineT::result_type;
  using          device_type    = std::random_device::result_type;
  using          seedseq_type   = std::seed_seq::result_type;
  constexpr auto bytes_needed   = StateSize * sizeof(engine_type);
  constexpr auto numbers_needed = (sizeof(device_type) < sizeof(seedseq_type))
    ? (bytes_needed / sizeof(device_type))
    : (bytes_needed / sizeof(seedseq_type));
  std::array<device_type, numbers_needed> numbers {};

  if(load) {
    std::ifstream is(load);
    size_t i = 0;
    for( ; is && i < numbers_needed; ++i)
      is >> numbers[i];
    if(i != numbers_needed)
      std::runtime_error(std::string("Failed loading seed from '") + load + "'");
  } else {
    std::random_device rnddev {};
    std::generate(numbers.begin(), numbers.end(), std::ref(rnddev));
  }
  std::seed_seq seedseq(numbers.cbegin(), numbers.cend());
  engine.seed(seedseq);

  if(save) {
    std::ofstream os(save);
    for(size_t i = 0; i < numbers_needed; ++i)
      os << numbers[i] << '\n';
    if(!os.good())
      throw std::runtime_error(std::string("Failed writing seed to '") + save + "'");
  }
}

template<typename EngineT>
EngineT seeded_prg(const char* save = nullptr, const char* load = nullptr)
{
  EngineT res;
  seed_prg(res, save, load);
  return res;
}

#endif // __RANDOM_SEED_HPP__
