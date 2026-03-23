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
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kRoundKeyBytes],
    unsigned char (&)[kMaskBytes],
    unsigned char (&)[kMaskBytes],
    unsigned int pLength) {
  CounterType counter;
  std::array<unsigned char, PASSWORD_EXPANDED_SIZE> seed_bytes{};
  std::copy(pSource, pSource + PASSWORD_EXPANDED_SIZE, seed_bytes.begin());
  counter.Seed(seed_bytes.data(), static_cast<int>(seed_bytes.size()));
  if (pWorkerA != nullptr) {
    counter.Get(pWorkerA, static_cast<int>(PASSWORD_EXPANDED_SIZE));
  }
  if (pWorkerB != nullptr) {
    counter.Get(pWorkerB, static_cast<int>(PASSWORD_EXPANDED_SIZE));
  }
  counter.Get(pDest, static_cast<int>(pLength));
}

void Baseline_AES256CTR(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],
    unsigned int pLength) {
  FillWithBaselineCounter<AESCounter>(
      pSource,
      pWorkerA,
      pWorkerB,
      pDest,
      pKeyStack,
      pMaskStackA,
      pMaskStackB,
      pNextRoundKeyBuffer,
      pNextRoundMaskBufferA,
      pNextRoundMaskBufferB,
      pLength);
}

void Baseline_ARIA256CTR(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],
    unsigned int pLength) {
  FillWithBaselineCounter<ARIA256Counter>(
      pSource,
      pWorkerA,
      pWorkerB,
      pDest,
      pKeyStack,
      pMaskStackA,
      pMaskStackB,
      pNextRoundKeyBuffer,
      pNextRoundMaskBufferA,
      pNextRoundMaskBufferB,
      pLength);
}

void Baseline_ChaCha20CTR(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],
    unsigned int pLength) {
  FillWithBaselineCounter<ChaCha20Counter>(
      pSource,
      pWorkerA,
      pWorkerB,
      pDest,
      pKeyStack,
      pMaskStackA,
      pMaskStackB,
      pNextRoundKeyBuffer,
      pNextRoundMaskBufferA,
      pNextRoundMaskBufferB,
      pLength);
}

void Baseline_MersenneTwister(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],
    unsigned int pLength) {
  FillWithBaselineCounter<MersenneCounter>(
      pSource,
      pWorkerA,
      pWorkerB,
      pDest,
      pKeyStack,
      pMaskStackA,
      pMaskStackB,
      pNextRoundKeyBuffer,
      pNextRoundMaskBufferA,
      pNextRoundMaskBufferB,
      pLength);
}

}  // namespace

void Baseline_NoKeySeed(
    unsigned char*,
    unsigned char (&)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int) {}

void Baseline_NoSaltSeed(
    unsigned char*,
    unsigned char (&)[kSaltBytes],
    unsigned int) {}

void Baseline_NoMaskSeedA(
    unsigned char*,
    unsigned char*,
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned int) {}

void Baseline_NoMaskSeedB(
    unsigned char*,
    unsigned char*,
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned int) {}

void Baseline_NoTwistBlock(
    unsigned char*,
    unsigned char*,
    unsigned char*,
    unsigned char*,
    unsigned int,
    const unsigned char (&)[kSaltBytes],
    unsigned char (&)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned int) {}

void Baseline_NoPushKeyRound(
    unsigned char*,
    const unsigned char (&)[kSaltBytes],
    unsigned char (&)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&)[kRoundKeyBytes],
    unsigned int) {}

void Baseline_NoPushMaskRoundA(
    unsigned char*,
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskBytes],
    unsigned int) {}

void Baseline_NoPushMaskRoundB(
    unsigned char*,
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskStackDepth][kMaskBytes],
    unsigned char (&)[kMaskBytes],
    unsigned int) {}

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
        &Baseline_NoKeySeed,
        &Baseline_NoSaltSeed,
        &Baseline_NoMaskSeedA,
        &Baseline_NoMaskSeedB,
        &Baseline_NoTwistBlock,
        &Baseline_NoPushKeyRound,
        &Baseline_NoPushMaskRoundA,
        &Baseline_NoPushMaskRoundB,
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
        &Baseline_NoKeySeed,
        &Baseline_NoSaltSeed,
        &Baseline_NoMaskSeedA,
        &Baseline_NoMaskSeedB,
        &Baseline_NoTwistBlock,
        &Baseline_NoPushKeyRound,
        &Baseline_NoPushMaskRoundA,
        &Baseline_NoPushMaskRoundB,
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
        &Baseline_NoKeySeed,
        &Baseline_NoSaltSeed,
        &Baseline_NoMaskSeedA,
        &Baseline_NoMaskSeedB,
        &Baseline_NoTwistBlock,
        &Baseline_NoPushKeyRound,
        &Baseline_NoPushMaskRoundA,
        &Baseline_NoPushMaskRoundB,
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
        &Baseline_NoKeySeed,
        &Baseline_NoSaltSeed,
        &Baseline_NoMaskSeedA,
        &Baseline_NoMaskSeedB,
        &Baseline_NoTwistBlock,
        &Baseline_NoPushKeyRound,
        &Baseline_NoPushMaskRoundA,
        &Baseline_NoPushMaskRoundB,
    },
};

const std::size_t kBaselineCandidateCount =
    sizeof(kBaselineCandidates) / sizeof(kBaselineCandidates[0]);

}  // namespace twist
