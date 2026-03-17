#pragma once

#include <cstddef>
#include <cstdint>

namespace twist::knobs {

// Generator defaults.
inline constexpr bool kRandomizeSeedByDefault = true;
inline constexpr std::uint64_t kRandomSeed = 0;  // 0 means "choose a fresh runtime seed"
inline constexpr std::size_t kCandidateCount = 10;
inline constexpr std::size_t kTopCandidateCount = 25;

// Counted ops are "3 core binary ops + transform count" per phase.
inline constexpr std::size_t kPhase1MinOps = 3;
inline constexpr std::size_t kPhase1MaxOps = 4;
inline constexpr std::size_t kPhase2MinOps = 4;
inline constexpr std::size_t kPhase2MaxOps = 5;
inline constexpr int kMaxTransformsTotal = 3;  // -1 means unlimited

// Core binary-op limits across both phases.
inline constexpr int kMaxAddOps = -1;
inline constexpr int kMaxSubOps = -1;
inline constexpr int kMaxMulOps = 2;
inline constexpr int kMaxXorOps = -1;
inline constexpr int kMaxAndOps = 0;
inline constexpr int kMaxOrOps = 0;

// Transform limits across both phases.
inline constexpr int kMaxAddConstTransforms = -1;
inline constexpr int kMaxShiftLeftTransforms = -1;
inline constexpr int kMaxShiftRightTransforms = -1;
inline constexpr int kMaxNotTransforms = -1;
inline constexpr int kMaxSwapNibblesTransforms = 1;
inline constexpr int kMaxByteLR8LeftTransforms = -1;
inline constexpr int kMaxByteLR8RightTransforms = -1;

// Harness defaults.
inline constexpr std::size_t kLengthFactor = 200;
inline constexpr std::size_t kTrialCountAES = 20;
inline constexpr std::size_t kTrialCountChaCha = 20;
inline constexpr std::size_t kTrialCountZeros = 5;
inline constexpr std::size_t kTrialCountOnes = 5;
inline constexpr std::size_t kTrialCountPredictableA = 5;
inline constexpr std::size_t kTrialCountPredictableB = 5;
inline constexpr std::size_t kTrialCountPredictableC = 5;
inline constexpr std::size_t kDefaultCycleBlockCount = 256;
inline constexpr std::size_t kDefaultSampleWindows = 131072;
inline constexpr std::size_t kDefaultSignatureBytes = 4096;
inline constexpr std::size_t kDefaultAvalancheBlocks = 8;
inline constexpr std::size_t kDefaultAvalancheTrials = 3;
inline constexpr std::size_t kLongRepeatScanBytes = 50000000;
inline constexpr std::size_t kLongRepeatTopCandidateCount = 1;
inline constexpr std::size_t kLongRepeatWindowBytesA = 64;
inline constexpr std::size_t kLongRepeatWindowBytesB = 128;

}  // namespace twist::knobs
