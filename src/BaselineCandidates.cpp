#include "TwistTypes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "../references/AESCounter.hpp"
#include "../references/ARIA256Counter.hpp"
#include "../references/ChaCha20Counter.hpp"
#include "../references/MersenneCounter.hpp"

namespace twist {
namespace {

template <typename CounterType>
void FillWithBaselineCounter(
    const std::uint8_t source[PASSWORD_EXPANDED_SIZE],
    std::uint8_t worker[PASSWORD_EXPANDED_SIZE],
    std::uint8_t dest[PASSWORD_EXPANDED_SIZE]) {
  CounterType counter;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> seed_bytes{};
  std::copy(source, source + PASSWORD_EXPANDED_SIZE, seed_bytes.begin());
  counter.Seed(seed_bytes.data(), static_cast<int>(seed_bytes.size()));
  counter.Get(worker, static_cast<int>(PASSWORD_EXPANDED_SIZE));
  counter.Get(dest, static_cast<int>(PASSWORD_EXPANDED_SIZE));
}

void Baseline_AES256CTR(
    const std::uint8_t source[PASSWORD_EXPANDED_SIZE],
    std::uint8_t worker[PASSWORD_EXPANDED_SIZE],
    std::uint8_t dest[PASSWORD_EXPANDED_SIZE]) {
  FillWithBaselineCounter<AESCounter>(source, worker, dest);
}

void Baseline_ARIA256CTR(
    const std::uint8_t source[PASSWORD_EXPANDED_SIZE],
    std::uint8_t worker[PASSWORD_EXPANDED_SIZE],
    std::uint8_t dest[PASSWORD_EXPANDED_SIZE]) {
  FillWithBaselineCounter<ARIA256Counter>(source, worker, dest);
}

void Baseline_ChaCha20CTR(
    const std::uint8_t source[PASSWORD_EXPANDED_SIZE],
    std::uint8_t worker[PASSWORD_EXPANDED_SIZE],
    std::uint8_t dest[PASSWORD_EXPANDED_SIZE]) {
  FillWithBaselineCounter<ChaCha20Counter>(source, worker, dest);
}

void Baseline_MersenneTwister(
    const std::uint8_t source[PASSWORD_EXPANDED_SIZE],
    std::uint8_t worker[PASSWORD_EXPANDED_SIZE],
    std::uint8_t dest[PASSWORD_EXPANDED_SIZE]) {
  FillWithBaselineCounter<MersenneCounter>(source, worker, dest);
}

}  // namespace

const RegisteredCandidate kBaselineCandidates[] = {
    {
        -101,
        "Baseline_AES256CTR",
        0,
        0,
        0,
        {{0, 0, 0}, "ctr", "encrypt", "stream", "source", "source", "none", 0, "none", 0},
        {{0, 0, 0}, "ctr", "encrypt", "stream", "worker", "worker", "none", 0, "none", 0},
        "baseline[aes256-ctr seeded from source; worker/dest filled from consecutive keystream]",
        &Baseline_AES256CTR,
    },
    {
        -102,
        "Baseline_ARIA256CTR",
        0,
        0,
        0,
        {{0, 0, 0}, "ctr", "hash", "stream", "source", "source", "none", 0, "none", 0},
        {{0, 0, 0}, "ctr", "hash", "stream", "worker", "worker", "none", 0, "none", 0},
        "baseline[aria256-ctr-style seeded from source; worker/dest filled from consecutive keystream]",
        &Baseline_ARIA256CTR,
    },
    {
        -103,
        "Baseline_ChaCha20CTR",
        0,
        0,
        0,
        {{0, 0, 0}, "ctr", "quarterround", "stream", "source", "source", "none", 0, "none", 0},
        {{0, 0, 0}, "ctr", "quarterround", "stream", "worker", "worker", "none", 0, "none", 0},
        "baseline[chacha20-ctr seeded from source; worker/dest filled from consecutive keystream]",
        &Baseline_ChaCha20CTR,
    },
    {
        -104,
        "Baseline_MersenneTwister",
        0,
        0,
        0,
        {{0, 0, 0}, "seed", "twist", "stream", "source", "source", "none", 0, "none", 0},
        {{0, 0, 0}, "seed", "temper", "stream", "worker", "worker", "none", 0, "none", 0},
        "baseline[mersenne-twister seeded from source; worker/dest filled from consecutive output]",
        &Baseline_MersenneTwister,
    },
};

const std::size_t kBaselineCandidateCount =
    sizeof(kBaselineCandidates) / sizeof(kBaselineCandidates[0]);

}  // namespace twist
