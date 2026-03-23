#pragma once

// Command reference:
//   generate:
//     ./generate_twists.sh <candidate_count> <seed>
//   measure:
//     LENGTH_FACTOR=<windows> SAMPLE_WINDOWS=<count> SIGNATURE_BYTES=<bytes> \
//     AVALANCHE_BLOCKS=<count> AVALANCHE_TRIALS=<count> BIC_SAMPLE_BITS=<count> \
//     SECOND_ORDER_TRIALS=<count> CROSS_INPUT_SIGNATURE_BYTES=<bytes> LONG_REPEAT_TOP=<count> \
//     TOP_N=<count> LIMIT=<count> CANDIDATE_ID=<id> ./run_measure_twists.sh <seed>
//   measure sharded:
//     SHARD_SIZE=<count> RESUME_MODE=<0|1> FAST_SHARD_MODE=<0|1> LENGTH_FACTOR=<windows> \
//     SAMPLE_WINDOWS=<count> SIGNATURE_BYTES=<bytes> AVALANCHE_BLOCKS=<count> \
//     AVALANCHE_TRIALS=<count> BIC_SAMPLE_BITS=<count> SECOND_ORDER_TRIALS=<count> \
//     CROSS_INPUT_SIGNATURE_BYTES=<bytes> LONG_REPEAT_TOP=<count> TOP_N=<count> \
//     ./run_measure_twists_sharded.sh <seed> [shard_size]
//   extract winners:
//     TOP_COUNT=<count> INPUT_DIR=<path> OUTPUT_DIR=<path> \
//     ./run_extract_winners.sh
//   extract byte twister:
//     SELECTION_SEED=<u64> BYTE_TWISTER_COUNT=<count> OUTPUT_PATH=<path> \
//     ./run_extract_byte_twister.sh

// ./generate_twists.sh 100 7777

// ./run_measure_twists_sharded.sh 7777 50



#include <cstddef>
#include <cstdint>

namespace twist::knobs {

// Generator defaults.
inline constexpr bool kRandomizeSeedByDefault = true;
inline constexpr std::uint64_t kRandomSeed = 0;  // 0 means "choose a fresh runtime seed"
inline constexpr std::size_t kCandidateCount = 10;
inline constexpr std::size_t kTopCandidateCount = 25;
inline constexpr bool kEnableThreeMatrix = true;
inline constexpr int kThreeMatrixRatioPercent = 40;

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
inline constexpr std::size_t kLengthFactor = 4096;
inline constexpr std::size_t kTrialCountAES = 6;
inline constexpr std::size_t kTrialCountChaCha = 6;
inline constexpr std::size_t kTrialCountZeros = 2;
inline constexpr std::size_t kTrialCountOnes = 2;
inline constexpr std::size_t kTrialCountPredictableA = 2;
inline constexpr std::size_t kTrialCountPredictableB = 2;
inline constexpr std::size_t kTrialCountPredictableC = 2;
inline constexpr std::size_t kDefaultSampleWindows = 138240;
inline constexpr std::size_t kDefaultSignatureBytes = 4096;
inline constexpr std::size_t kDefaultAvalancheBlocks = 24;
inline constexpr std::size_t kDefaultAvalancheTrials = 2;
inline constexpr std::size_t kDefaultBicSampleBits = 64;
inline constexpr std::size_t kDefaultSecondOrderTrials = 2;
inline constexpr std::size_t kDefaultCrossInputSignatureBytes = 2048;
inline constexpr std::size_t kLongRepeatScanBytes = 31457280;  // 4096 * 7680-byte windows
inline constexpr std::size_t kLongRepeatTopCandidateCount = 1;
inline constexpr std::size_t kLongRepeatMinMatchBytes = 96;

}  // namespace twist::knobs
