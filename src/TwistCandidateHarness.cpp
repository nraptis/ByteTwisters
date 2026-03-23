#include "Knobs.hpp"
#include "PasswordExpander.hpp"
#include "TwistTypes.hpp"

#include "../references/AESCounter.hpp"
#include "../references/ChaCha20Counter.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace twist {
namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;

enum class TrialCategory {
  kAES,
  kChaCha,
  kZeros,
  kOnes,
  kPredictableA,
  kPredictableB,
  kPredictableC,
};

struct Options {
  std::uint64_t seed = 1337;
  bool seed_supplied = false;
  bool stream_bytes_supplied = false;
  std::string legacy_input_suite = "mixed";
  std::size_t length_factor = knobs::kLengthFactor;
  std::size_t stream_bytes = PASSWORD_EXPANDED_SIZE * knobs::kLengthFactor;
  std::size_t sample_windows = knobs::kDefaultSampleWindows;
  std::size_t signature_bytes = knobs::kDefaultSignatureBytes;
  std::size_t avalanche_blocks = knobs::kDefaultAvalancheBlocks;
  std::size_t avalanche_trials = knobs::kDefaultAvalancheTrials;
  std::size_t bic_sample_bits = knobs::kDefaultBicSampleBits;
  std::size_t second_order_trials = knobs::kDefaultSecondOrderTrials;
  std::size_t cross_input_signature_bytes = knobs::kDefaultCrossInputSignatureBytes;
  std::size_t long_repeat_scan_bytes = knobs::kLongRepeatScanBytes;
  std::size_t long_repeat_top_count = knobs::kLongRepeatTopCandidateCount;
  std::size_t long_repeat_min_match_bytes = knobs::kLongRepeatMinMatchBytes;
  std::size_t top_n = knobs::kTopCandidateCount;
  std::size_t trial_cap_per_category = 0;
  int candidate_id = -1;
  bool candidate_id_supplied = false;
  std::size_t limit = 0;
  std::string output_dir = "generated";
};

struct SplitMix64 {
  explicit SplitMix64(std::uint64_t seed_value) : state(seed_value) {}

  std::uint64_t Next() {
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31U);
  }

  std::uint64_t state;
};

struct Window128 {
  std::uint64_t lo = 0;
  std::uint64_t hi = 0;

  bool operator==(const Window128& other) const noexcept {
    return lo == other.lo && hi == other.hi;
  }
};

struct Window128Hash {
  std::size_t operator()(const Window128& window) const noexcept {
    std::uint64_t mixed = window.lo ^ (window.hi + 0x9E3779B97F4A7C15ULL +
                                       (window.lo << 6U) + (window.lo >> 2U));
    mixed ^= mixed >> 33U;
    mixed *= 0xFF51AFD7ED558CCDULL;
    mixed ^= mixed >> 33U;
    return static_cast<std::size_t>(mixed);
  }
};

struct ExactRepeatMatch {
  bool found = false;
  std::size_t first_position = std::numeric_limits<std::size_t>::max();
  std::size_t repeat_position = std::numeric_limits<std::size_t>::max();
  std::size_t match_length = 0;
};

struct GradeInfo {
  const char* label;
  int rank;
};

struct TrialSpec {
  TrialCategory category;
  std::size_t ordinal_within_category;
  std::size_t global_index;
};

struct CategoryConfig {
  TrialCategory category;
  std::size_t trial_count;
};

struct CandidateResult {
  int candidate_id = 0;
  std::string function_name;
  std::string recipe_summary;
  double repeat_score = 0.0;
  double cycle_score = 0.0;
  double uniformity_score = 0.0;
  double predictability_score = 0.0;
  double avalanche_score = 0.0;
  double completeness_score = 0.0;
  double bic_score = 0.0;
  double bit_inclusion_score = 0.0;
  double nonlinearity_score = 0.0;
  double cross_input_collision_score = 0.0;
  double distinctness_score = 0.0;
  double composite_score = 0.0;
  double entropy = 0.0;
  double reduced_chi_squared = 0.0;
  double max_deviation = 0.0;
  double byte_count_spread_ratio = 0.0;
  std::uint64_t most_common_byte_count = 0;
  std::uint64_t least_common_byte_count = 0;
  double equal_rate_drift = 0.0;
  double correlation = 0.0;
  double conditional_entropy = 0.0;
  double avalanche_byte_ratio = 0.0;
  double avalanche_bit_ratio = 0.0;
  double avalanche_min_byte_ratio = 1.0;
  double avalanche_max_byte_ratio = 0.0;
  double avalanche_min_bit_ratio = 1.0;
  double avalanche_max_bit_ratio = 0.0;
  double bit_inclusion_byte_ratio = 0.0;
  double bit_inclusion_bit_ratio = 0.0;
  double bic_bit_bias = 0.5;
  double bic_pair_bias = 0.5;
  double second_order_byte_ratio = 0.0;
  double second_order_bit_ratio = 0.0;
  int cross_input_nearest_hamming = 0;
  bool cross_input_collision_found = false;
  std::size_t first_repeat_window = std::numeric_limits<std::size_t>::max();
  std::size_t first_cycle_block = std::numeric_limits<std::size_t>::max();
  std::size_t cycle_length = 0;
  std::size_t long_repeat_match_position = std::numeric_limits<std::size_t>::max();
  std::size_t long_repeat_match_trial = std::numeric_limits<std::size_t>::max();
  std::size_t long_repeat_match_length = 0;
  bool repeat_found = false;
  bool cycle_found = false;
  bool long_repeat_match_found = false;
  bool long_repeat_verified = false;
  bool rejected = false;
  std::string failure_reason;
  std::string grade;
  int grade_rank = 0;
  std::uint64_t first_block_hash = 0;
  std::uint64_t signature_lo = 0;
  std::uint64_t signature_hi = 0;
  std::array<double, 7> category_scores{};
  std::array<int, 7> category_grade_ranks{};
  std::array<bool, 7> category_present{};
  std::array<bool, 7> category_rejected{};
  std::array<std::string, 7> category_grade_labels{};
};

std::uint64_t Mix64(std::uint64_t value);
const char* TrialCategoryName(TrialCategory category);
const char* TrialCategoryShortName(TrialCategory category);
const char* InputLabel(TrialCategory category, std::size_t trial_index);
const char* LabelForGlobalTrialIndex(std::size_t global_trial_index);
void FinalizeScores(std::vector<CandidateResult>& results, const Options& options);

double Clamp(double value, double lower, double upper) {
  return std::max(lower, std::min(value, upper));
}

std::size_t RoundUpToWindowMultiple(std::size_t value) {
  if (value == 0U) {
    return PASSWORD_EXPANDED_SIZE;
  }
  const std::size_t remainder = value % PASSWORD_EXPANDED_SIZE;
  if (remainder == 0U) {
    return value;
  }
  return value + (PASSWORD_EXPANDED_SIZE - remainder);
}

std::size_t WindowCountForBytes(std::size_t value) {
  return RoundUpToWindowMultiple(value) / PASSWORD_EXPANDED_SIZE;
}

std::vector<std::size_t> BuildSampleBlockPositions(
    std::size_t total_blocks,
    std::size_t sample_count) {
  std::vector<std::size_t> positions;
  if (total_blocks == 0U) {
    return positions;
  }

  const std::size_t desired = std::min(total_blocks, std::max<std::size_t>(1U, sample_count));
  positions.reserve(desired);
  if (desired == 1U) {
    positions.push_back(0U);
    return positions;
  }

  std::size_t previous = std::numeric_limits<std::size_t>::max();
  for (std::size_t index = 0; index < desired; ++index) {
    std::size_t position =
        (index * (total_blocks - 1U) + ((desired - 1U) / 2U)) / (desired - 1U);
    if (previous != std::numeric_limits<std::size_t>::max() && position <= previous) {
      position = std::min(total_blocks - 1U, previous + 1U);
    }
    positions.push_back(position);
    previous = position;
  }
  return positions;
}

std::vector<std::size_t> BuildEvenlySpacedPositions(
    std::size_t total_positions,
    std::size_t sample_count) {
  std::vector<std::size_t> positions;
  if (total_positions == 0U || sample_count == 0U) {
    return positions;
  }
  const std::size_t desired = std::min(total_positions, sample_count);
  positions.reserve(desired);
  if (desired == 1U) {
    positions.push_back(0U);
    return positions;
  }
  std::size_t previous = std::numeric_limits<std::size_t>::max();
  for (std::size_t index = 0; index < desired; ++index) {
    std::size_t position =
        (index * (total_positions - 1U) + ((desired - 1U) / 2U)) / (desired - 1U);
    if (previous != std::numeric_limits<std::size_t>::max() && position <= previous) {
      position = std::min(total_positions - 1U, previous + 1U);
    }
    positions.push_back(position);
    previous = position;
  }
  return positions;
}

Window128 BuildSignatureFromBytes(const std::vector<std::uint8_t>& bytes, std::size_t limit_bytes) {
  Window128 signature;
  const std::size_t size = std::min(limit_bytes, bytes.size());
  std::uint64_t lo = 1469598103934665603ULL;
  std::uint64_t hi = 1099511628211ULL ^ 0x9E3779B97F4A7C15ULL;
  for (std::size_t index = 0; index < size; ++index) {
    lo ^= static_cast<std::uint64_t>(bytes[index]) +
          (static_cast<std::uint64_t>(index) << 8U);
    lo *= 1099511628211ULL;
    const std::size_t reverse_index = size - 1U - index;
    hi ^= static_cast<std::uint64_t>(bytes[reverse_index]) +
          (static_cast<std::uint64_t>(index) << 16U);
    hi *= 1469598103934665603ULL;
  }
  signature.lo = Mix64(lo ^ static_cast<std::uint64_t>(size));
  signature.hi = Mix64(hi ^ (static_cast<std::uint64_t>(size) << 32U));
  return signature;
}

std::uint64_t RuntimeSeed() {
  const std::uint64_t time_seed = static_cast<std::uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::random_device rd;
  const std::uint64_t random_seed =
      (static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd());
  return Mix64(time_seed ^ random_seed ^ 0xA5A5A5A5D3C4B2E1ULL);
}

std::uint64_t ResolveDefaultSeed() {
  if (knobs::kRandomSeed != 0U) {
    return knobs::kRandomSeed;
  }
  if (knobs::kRandomizeSeedByDefault) {
    return RuntimeSeed();
  }
  return 1337ULL;
}

std::uint64_t Mix64(std::uint64_t value) {
  value += 0x9E3779B97F4A7C15ULL;
  value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31U);
}

std::uint64_t HashBytes64(const std::uint8_t* data, std::size_t size) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= static_cast<std::uint64_t>(data[i]);
    hash *= 1099511628211ULL;
  }
  return hash;
}

Window128 BuildWindow(const std::array<std::uint8_t, 16>& ring, std::size_t next_index) {
  Window128 window;
  for (std::size_t i = 0; i < 8; ++i) {
    const std::uint8_t byte = ring[(next_index + i) % 16U];
    window.lo |= static_cast<std::uint64_t>(byte) << (8U * i);
  }
  for (std::size_t i = 0; i < 8; ++i) {
    const std::uint8_t byte = ring[(next_index + 8U + i) % 16U];
    window.hi |= static_cast<std::uint64_t>(byte) << (8U * i);
  }
  return window;
}

GradeInfo ComputeGrade(double composite_score) {
  if (composite_score >= 97.0) {
    return {"A+", 12};
  }
  if (composite_score >= 93.0) {
    return {"A", 11};
  }
  if (composite_score >= 90.0) {
    return {"A-", 10};
  }
  if (composite_score >= 87.0) {
    return {"B+", 9};
  }
  if (composite_score >= 83.0) {
    return {"B", 8};
  }
  if (composite_score >= 80.0) {
    return {"B-", 7};
  }
  if (composite_score >= 77.0) {
    return {"C+", 6};
  }
  if (composite_score >= 73.0) {
    return {"C", 5};
  }
  if (composite_score >= 70.0) {
    return {"C-", 4};
  }
  if (composite_score >= 65.0) {
    return {"D+", 3};
  }
  if (composite_score >= 60.0) {
    return {"D", 2};
  }
  if (composite_score >= 50.0) {
    return {"D-", 1};
  }
  return {"F", 0};
}

std::string FormatDouble(double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(3) << value;
  return stream.str();
}

std::string EscapeHtml(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char ch : value) {
    switch (ch) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

std::string FormatLimit(int value) {
  if (value < 0) {
    return "unlimited";
  }
  if (value == 0) {
    return "disabled";
  }
  return std::to_string(value);
}

constexpr std::size_t kCategoryCount = 7;

std::size_t CategoryIndex(TrialCategory category) {
  return static_cast<std::size_t>(category);
}

const std::array<CategoryConfig, kCategoryCount>& CategoryConfigs() {
  static const std::array<CategoryConfig, kCategoryCount> configs = {{
      {TrialCategory::kAES, knobs::kTrialCountAES},
      {TrialCategory::kChaCha, knobs::kTrialCountChaCha},
      {TrialCategory::kZeros, knobs::kTrialCountZeros},
      {TrialCategory::kOnes, knobs::kTrialCountOnes},
      {TrialCategory::kPredictableA, knobs::kTrialCountPredictableA},
      {TrialCategory::kPredictableB, knobs::kTrialCountPredictableB},
      {TrialCategory::kPredictableC, knobs::kTrialCountPredictableC},
  }};
  return configs;
}

std::size_t TotalConfiguredTrials() {
  std::size_t total = 0;
  for (const CategoryConfig& config : CategoryConfigs()) {
    total += config.trial_count;
  }
  return total;
}

std::size_t EffectiveTrialCount(const Options& options, const CategoryConfig& config) {
  if (options.trial_cap_per_category == 0U) {
    return config.trial_count;
  }
  return std::min(config.trial_count, options.trial_cap_per_category);
}

std::vector<TrialSpec> BuildTrialPlan(const Options& options) {
  std::vector<TrialSpec> plan;
  std::size_t reserved = 0U;
  for (const CategoryConfig& config : CategoryConfigs()) {
    reserved += EffectiveTrialCount(options, config);
  }
  plan.reserve(reserved);
  std::size_t global_index = 0;
  for (const CategoryConfig& config : CategoryConfigs()) {
    for (std::size_t ordinal = 0; ordinal < EffectiveTrialCount(options, config); ++ordinal) {
      plan.push_back({config.category, ordinal, global_index});
      ++global_index;
    }
  }
  return plan;
}

const char* TrialCategoryName(TrialCategory category) {
  switch (category) {
    case TrialCategory::kAES:
      return "aes";
    case TrialCategory::kChaCha:
      return "chacha";
    case TrialCategory::kZeros:
      return "zeros";
    case TrialCategory::kOnes:
      return "ones";
    case TrialCategory::kPredictableA:
      return "predictable_a";
    case TrialCategory::kPredictableB:
      return "predictable_b";
    case TrialCategory::kPredictableC:
      return "predictable_c";
  }
  return "unknown";
}

const char* TrialCategoryShortName(TrialCategory category) {
  switch (category) {
    case TrialCategory::kAES:
      return "AES";
    case TrialCategory::kChaCha:
      return "ChaCha";
    case TrialCategory::kZeros:
      return "Zeros";
    case TrialCategory::kOnes:
      return "Ones";
    case TrialCategory::kPredictableA:
      return "PredictableA";
    case TrialCategory::kPredictableB:
      return "PredictableB";
    case TrialCategory::kPredictableC:
      return "PredictableC";
  }
  return "Unknown";
}

std::string SourcePatternList(const std::vector<TrialSpec>& plan) {
  std::ostringstream stream;
  for (std::size_t i = 0; i < plan.size(); ++i) {
    if (i > 0U) {
      stream << ", ";
    }
    stream << InputLabel(plan[i].category, plan[i].ordinal_within_category);
  }
  return stream.str();
}

const char* InputLabel(TrialCategory category, std::size_t trial_index) {
  static_cast<void>(trial_index);
  switch (category) {
    case TrialCategory::kAES:
      return "aes_counter";
    case TrialCategory::kChaCha:
      return "chacha_counter";
    case TrialCategory::kZeros:
      return "all_00";
    case TrialCategory::kOnes:
      return "all_ff";
    case TrialCategory::kPredictableA:
      return "predictable_ramp";
    case TrialCategory::kPredictableB:
      return "predictable_checker";
    case TrialCategory::kPredictableC:
      return "predictable_blockstep";
  }
  return "unknown";
}

const char* LabelForGlobalTrialIndex(std::size_t global_trial_index) {
  static const Options default_options;
  static const std::vector<TrialSpec> plan = BuildTrialPlan(default_options);
  if (global_trial_index >= plan.size()) {
    return "unknown";
  }
  return InputLabel(plan[global_trial_index].category, plan[global_trial_index].ordinal_within_category);
}

bool IncludeTrialInCrossInputSignatureSet(TrialCategory category, std::size_t trial_index) {
  switch (category) {
    case TrialCategory::kZeros:
    case TrialCategory::kOnes:
      return trial_index == 0U;
    case TrialCategory::kAES:
    case TrialCategory::kChaCha:
    case TrialCategory::kPredictableA:
    case TrialCategory::kPredictableB:
    case TrialCategory::kPredictableC:
      return true;
  }
  return true;
}

std::uint64_t ScenarioTrialSeed(
    std::uint64_t base_seed,
    TrialCategory category,
    std::size_t trial_index) {
  const std::uint64_t tag =
      0x1111111111111111ULL +
      (static_cast<std::uint64_t>(CategoryIndex(category)) * 0x0101010101010101ULL);
  return Mix64(base_seed ^ tag ^
               (static_cast<std::uint64_t>(trial_index) * 0x9E3779B97F4A7C15ULL));
}

void FillCounterSeedMaterial(
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE>& seed_material,
    std::uint8_t seed_byte) {
  seed_material.fill(seed_byte);
}

void FillSourceForTrial(
    std::vector<std::uint8_t>& buffer,
    TrialCategory category,
    std::uint64_t base_seed,
    std::size_t trial_index) {
  const std::uint8_t seed_byte = static_cast<std::uint8_t>(base_seed & 0xFFU);
  if (category == TrialCategory::kAES) {
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> seed_material{};
    FillCounterSeedMaterial(seed_material, seed_byte);
    AESCounter counter;
    counter.Seed(seed_material.data(), static_cast<int>(seed_material.size()));
    const std::size_t bytes_to_skip = trial_index * buffer.size();
    for (std::size_t i = 0; i < bytes_to_skip; ++i) {
      static_cast<void>(counter.Get());
    }
    counter.Get(buffer.data(), static_cast<int>(buffer.size()));
    return;
  }
  if (category == TrialCategory::kChaCha) {
    std::array<unsigned char, PASSWORD_EXPANDED_SIZE> seed_material{};
    FillCounterSeedMaterial(seed_material, seed_byte);
    ChaCha20Counter counter;
    counter.Seed(seed_material.data(), static_cast<int>(seed_material.size()));
    const std::size_t bytes_to_skip = trial_index * buffer.size();
    for (std::size_t i = 0; i < bytes_to_skip; ++i) {
      static_cast<void>(counter.Get());
    }
    counter.Get(buffer.data(), static_cast<int>(buffer.size()));
    return;
  }

  for (std::size_t i = 0; i < buffer.size(); ++i) {
    switch (category) {
      case TrialCategory::kZeros:
        buffer[i] = 0x00U;
        break;
      case TrialCategory::kOnes:
        buffer[i] = 0xFFU;
        break;
      case TrialCategory::kPredictableA:
        buffer[i] = static_cast<std::uint8_t>((i + trial_index + seed_byte) & 0xFFU);
        break;
      case TrialCategory::kPredictableB:
        buffer[i] = static_cast<std::uint8_t>(((i + trial_index) & 1U) == 0U ? seed_byte : (seed_byte ^ 0xFFU));
        break;
      case TrialCategory::kPredictableC:
        buffer[i] = static_cast<std::uint8_t>((((i / 32U) + trial_index) & 1U) == 0U ? seed_byte : (seed_byte + 0x55U));
        break;
      default:
        buffer[i] = 0U;
        break;
    }
  }
}

std::vector<std::uint8_t> GenerateExpandedStream(
    TwistFunction function,
    const std::vector<std::uint8_t>& source,
    std::size_t stream_bytes) {
  std::vector<std::uint8_t> expanded_source = source;
  std::vector<std::uint8_t> worker_a(PASSWORD_EXPANDED_SIZE);
  std::vector<std::uint8_t> worker_b(PASSWORD_EXPANDED_SIZE);
  unsigned char key_stack[kRoundKeyStackDepth][kRoundKeyBytes]{};
  unsigned char mask_stack_a[kMaskStackDepth][kMaskBytes]{};
  unsigned char mask_stack_b[kMaskStackDepth][kMaskBytes]{};
  unsigned char next_round_key_buffer[kRoundKeyBytes]{};
  unsigned char next_round_mask_buffer_a[kMaskBytes]{};
  unsigned char next_round_mask_buffer_b[kMaskBytes]{};
  std::vector<std::uint8_t> stream(stream_bytes);
  if (stream.empty()) {
    return stream;
  }

  PasswordExpander::ExpandPassword(
      function,
      expanded_source.data(),
      worker_a.data(),
      worker_b.data(),
      stream.data(),
      key_stack,
      mask_stack_a,
      mask_stack_b,
      next_round_key_buffer,
      next_round_mask_buffer_a,
      next_round_mask_buffer_b,
      static_cast<unsigned int>(stream.size()));
  return stream;
}

const std::uint8_t* BlockData(
    const std::vector<std::uint8_t>& stream,
    std::size_t block_index) {
  return stream.data() + (block_index * PASSWORD_EXPANDED_SIZE);
}

ExactRepeatMatch FindBestRepeatedMatchLZ(
    const std::vector<std::uint8_t>& stream,
    std::size_t min_match_bytes) {
  ExactRepeatMatch result;
  constexpr std::size_t kAnchorBytes = 4U;
  constexpr std::size_t kHashBits = 18U;
  constexpr std::size_t kBucketCount = 1U << kHashBits;
  constexpr std::size_t kBucketMask = kBucketCount - 1U;
  constexpr std::size_t kBucketDepth = 8U;

  if (min_match_bytes < kAnchorBytes || stream.size() < min_match_bytes) {
    return result;
  }

  std::vector<std::array<std::uint32_t, kBucketDepth>> buckets(kBucketCount);
  std::vector<std::uint8_t> bucket_counts(kBucketCount, 0U);
  std::vector<std::uint8_t> bucket_next(kBucketCount, 0U);
  for (auto& bucket : buckets) {
    bucket.fill(std::numeric_limits<std::uint32_t>::max());
  }

  const std::size_t limit = stream.size() - kAnchorBytes + 1U;
  for (std::size_t position = 0; position < limit; ++position) {
    std::uint32_t anchor = 0U;
    std::memcpy(&anchor, stream.data() + position, sizeof(anchor));
    const std::size_t bucket_index =
        static_cast<std::size_t>(Mix64(static_cast<std::uint64_t>(anchor) * 0x9E3779B185EBCA87ULL)) &
        kBucketMask;
    const std::uint8_t count = bucket_counts[bucket_index];
    const std::uint8_t next_slot = bucket_next[bucket_index];
    const auto& bucket = buckets[bucket_index];

    for (std::uint8_t probe = 0; probe < count; ++probe) {
      const std::uint32_t prior_u32 = bucket[(next_slot + kBucketDepth - 1U - probe) % kBucketDepth];
      if (prior_u32 == std::numeric_limits<std::uint32_t>::max()) {
        continue;
      }
      const std::size_t prior = static_cast<std::size_t>(prior_u32);
      if (prior >= position) {
        continue;
      }

      std::size_t match_length = kAnchorBytes;
      while (position + match_length < stream.size() &&
             prior + match_length < stream.size() &&
             stream[position + match_length] == stream[prior + match_length]) {
        ++match_length;
      }

      if (match_length < min_match_bytes) {
        continue;
      }

      if (!result.found ||
          match_length > result.match_length ||
          (match_length == result.match_length && prior < result.first_position) ||
          (match_length == result.match_length && prior == result.first_position &&
           position < result.repeat_position)) {
        result.found = true;
        result.first_position = prior;
        result.repeat_position = position;
        result.match_length = match_length;
      }
    }

    buckets[bucket_index][next_slot] = static_cast<std::uint32_t>(position);
    bucket_next[bucket_index] = static_cast<std::uint8_t>((next_slot + 1U) % kBucketDepth);
    if (bucket_counts[bucket_index] < kBucketDepth) {
      bucket_counts[bucket_index] += 1U;
    }
  }

  return result;
}

CandidateResult EvaluateCandidateForPlan(
    const RegisteredCandidate& candidate,
    const Options& options,
    const std::vector<TrialSpec>& trial_plan) {
  CandidateResult result;
  result.candidate_id = candidate.candidate_id;
  result.function_name = candidate.function_name;
  result.recipe_summary = candidate.recipe_summary;

  std::array<std::uint64_t, 256> histogram{};
  std::vector<std::uint64_t> transitions(256U * 256U, 0U);
  std::array<int, 128> simhash_accumulator{};
  std::size_t processed_bytes = 0;
  std::size_t signature_bytes_seen = 0;
  std::uint64_t equal_adjacent = 0;
  long double sum_x = 0.0L;
  long double sum_y = 0.0L;
  long double sum_x2 = 0.0L;
  long double sum_y2 = 0.0L;
  long double sum_xy = 0.0L;
  std::uint64_t differing_bytes = 0U;
  std::uint64_t differing_bits = 0U;
  std::uint64_t compared_bytes = 0U;
  std::uint64_t second_order_differing_bytes = 0U;
  std::uint64_t second_order_differing_bits = 0U;
  std::uint64_t second_order_compared_bytes = 0U;
  std::uint64_t included_byte_slots = 0U;
  std::uint64_t included_bit_slots = 0U;
  std::uint64_t total_inclusion_byte_slots = 0U;
  std::uint64_t total_inclusion_bit_slots = 0U;
  std::vector<Window128> cross_input_signatures;
  cross_input_signatures.reserve(trial_plan.size());

  std::vector<std::uint64_t> bic_one_counts(options.bic_sample_bits, 0U);
  std::vector<std::uint64_t> bic_pair_counts(options.bic_sample_bits * options.bic_sample_bits, 0U);
  std::uint64_t bic_observation_count = 0U;

  for (const TrialSpec& trial : trial_plan) {
    const std::uint64_t trial_seed =
        ScenarioTrialSeed(options.seed, trial.category, trial.ordinal_within_category);

    std::vector<std::uint8_t> source(PASSWORD_EXPANDED_SIZE);
    FillSourceForTrial(source, trial.category, options.seed, trial.ordinal_within_category);
    const std::vector<std::uint8_t> initial_source = source;
    const std::size_t required_blocks = WindowCountForBytes(options.stream_bytes);
    const std::vector<std::size_t> sampled_block_positions =
        BuildSampleBlockPositions(required_blocks, options.avalanche_blocks);
    std::vector<std::vector<std::uint8_t>> sampled_baseline_blocks(sampled_block_positions.size());
    std::size_t sampled_block_cursor = 0U;
    const std::vector<std::uint8_t> baseline_stream =
        GenerateExpandedStream(candidate.function, initial_source, options.stream_bytes);

    const std::size_t window_stride = std::max<std::size_t>(
        1U, options.stream_bytes / std::max<std::size_t>(1U, options.sample_windows));
    std::unordered_map<Window128, std::size_t, Window128Hash> sampled_windows;
    sampled_windows.reserve(options.sample_windows * 2U);

    std::array<std::uint8_t, 16> rolling_window{};
    std::size_t rolling_position = 0;
    std::size_t trial_processed_bytes = 0;
    std::optional<std::uint8_t> previous_byte;
    std::vector<std::uint8_t> cross_input_signature_bytes;
    cross_input_signature_bytes.reserve(options.cross_input_signature_bytes);

    const std::size_t total_blocks = required_blocks;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> block_hashes;
    block_hashes.reserve(total_blocks * 2U);
    std::vector<std::vector<std::uint8_t>> seen_blocks;
    seen_blocks.reserve(total_blocks);

    for (std::size_t block_index = 0; block_index < total_blocks; ++block_index) {
      const std::uint8_t* block = BlockData(baseline_stream, block_index);
      const std::vector<std::uint8_t> current_block(
          block, block + PASSWORD_EXPANDED_SIZE);

      const std::uint64_t block_hash = HashBytes64(block, PASSWORD_EXPANDED_SIZE);
      if (trial.global_index == 0U && block_index == 0U) {
        result.first_block_hash = block_hash;
      }
      const auto block_match = block_hashes.find(block_hash);
      if (block_match != block_hashes.end() && !result.cycle_found) {
        for (const std::size_t prior_index : block_match->second) {
          if (seen_blocks[prior_index] == current_block) {
            result.cycle_found = true;
            result.first_cycle_block = std::min(result.first_cycle_block, prior_index);
            result.cycle_length = result.cycle_length == 0U
                                      ? (block_index - prior_index)
                                      : std::min(result.cycle_length, block_index - prior_index);
            break;
          }
        }
      }
      seen_blocks.push_back(current_block);
      block_hashes[block_hash].push_back(block_index);

      if (sampled_block_cursor < sampled_block_positions.size() &&
          block_index == sampled_block_positions[sampled_block_cursor]) {
        sampled_baseline_blocks[sampled_block_cursor].assign(
            block, block + PASSWORD_EXPANDED_SIZE);
        ++sampled_block_cursor;
      }

      const std::size_t bytes_to_scan =
          trial_processed_bytes < options.stream_bytes
              ? std::min<std::size_t>(PASSWORD_EXPANDED_SIZE, options.stream_bytes - trial_processed_bytes)
              : 0U;

      for (std::size_t i = 0; i < bytes_to_scan; ++i) {
        const std::uint8_t byte = block[i];
        histogram[byte] += 1U;

        if (previous_byte.has_value()) {
          const std::uint8_t prev = *previous_byte;
          transitions[static_cast<std::size_t>(prev) * 256U + static_cast<std::size_t>(byte)] += 1U;
          equal_adjacent += static_cast<std::uint64_t>(prev == byte);
          const long double x = static_cast<long double>(prev);
          const long double y = static_cast<long double>(byte);
          sum_x += x;
          sum_y += y;
          sum_x2 += x * x;
          sum_y2 += y * y;
          sum_xy += x * y;
        }

        previous_byte = byte;
        rolling_window[rolling_position] = byte;
        rolling_position = (rolling_position + 1U) % rolling_window.size();

        if (trial_processed_bytes + 1U >= rolling_window.size()) {
          const std::size_t window_index = trial_processed_bytes + 1U - rolling_window.size();
          if ((window_index % window_stride) == 0U) {
            const Window128 window = BuildWindow(rolling_window, rolling_position);
            const auto insert = sampled_windows.emplace(window, window_index);
            if (!insert.second) {
              result.repeat_found = true;
              result.first_repeat_window =
                  std::min(result.first_repeat_window, insert.first->second);
            }
          }
        }

        if (signature_bytes_seen < options.signature_bytes) {
          const std::uint64_t token = Mix64(
              (static_cast<std::uint64_t>(signature_bytes_seen) << 8U) ^
              static_cast<std::uint64_t>(byte) ^ 0xD6E8FEB86659FD93ULL);
          const std::uint64_t token_hi = Mix64(token ^ 0xA0761D6478BD642FULL);
          for (int bit = 0; bit < 64; ++bit) {
            simhash_accumulator[bit] += ((token >> bit) & 1ULL) != 0ULL ? 1 : -1;
            simhash_accumulator[64 + bit] += ((token_hi >> bit) & 1ULL) != 0ULL ? 1 : -1;
          }
          signature_bytes_seen += 1U;
        }

        if (cross_input_signature_bytes.size() < options.cross_input_signature_bytes) {
          cross_input_signature_bytes.push_back(byte);
        }

        processed_bytes += 1U;
        trial_processed_bytes += 1U;
      }
    }

    if (IncludeTrialInCrossInputSignatureSet(trial.category, trial.ordinal_within_category)) {
      cross_input_signatures.push_back(
          BuildSignatureFromBytes(cross_input_signature_bytes, options.cross_input_signature_bytes));
    }

    if (!sampled_baseline_blocks.empty()) {
      std::vector<std::uint8_t> byte_inclusion_flags(
          sampled_baseline_blocks.size() * PASSWORD_EXPANDED_SIZE, 0U);
      std::vector<std::uint8_t> bit_inclusion_flags(
          sampled_baseline_blocks.size() * PASSWORD_EXPANDED_SIZE * 8U, 0U);
      std::vector<std::uint8_t> flattened_diff(
          sampled_baseline_blocks.size() * PASSWORD_EXPANDED_SIZE, 0U);
      std::uint64_t trial_included_byte_slots = 0U;
      std::uint64_t trial_included_bit_slots = 0U;
      const std::vector<std::size_t> bic_sample_positions = BuildEvenlySpacedPositions(
          flattened_diff.size() * 8U, options.bic_sample_bits);

      for (std::size_t avalanche_trial = 0; avalanche_trial < options.avalanche_trials; ++avalanche_trial) {
        std::vector<std::uint8_t> alt_source = initial_source;
        std::fill(flattened_diff.begin(), flattened_diff.end(), 0U);
        const std::uint64_t flip_token =
            Mix64(trial_seed ^ (static_cast<std::uint64_t>(avalanche_trial) * 0xBF58476D1CE4E5B9ULL));
        const std::size_t flip_index =
            static_cast<std::size_t>(flip_token % PASSWORD_EXPANDED_SIZE);
        const std::uint8_t bit_mask =
            static_cast<std::uint8_t>(1U << ((flip_token >> 8U) & 7U));
        alt_source[flip_index] ^= bit_mask;
        const std::vector<std::uint8_t> alt_stream =
            GenerateExpandedStream(candidate.function, alt_source, options.stream_bytes);

        std::uint64_t local_differing_bytes = 0U;
        std::uint64_t local_differing_bits = 0U;
        std::uint64_t local_compared_bytes = 0U;
        for (std::size_t sample_index = 0; sample_index < sampled_block_positions.size(); ++sample_index) {
          const std::vector<std::uint8_t>& baseline = sampled_baseline_blocks[sample_index];
          const std::uint8_t* alt_block =
              BlockData(alt_stream, sampled_block_positions[sample_index]);
          for (std::size_t i = 0; i < baseline.size(); ++i) {
            const std::uint8_t diff = static_cast<std::uint8_t>(baseline[i] ^ alt_block[i]);
            differing_bytes += static_cast<std::uint64_t>(diff != 0U);
            differing_bits += static_cast<std::uint64_t>(
                std::popcount(static_cast<unsigned int>(diff)));
            local_differing_bytes += static_cast<std::uint64_t>(diff != 0U);
            local_differing_bits += static_cast<std::uint64_t>(
                std::popcount(static_cast<unsigned int>(diff)));
            flattened_diff[(sample_index * baseline.size()) + i] = diff;
            if (diff == 0U) {
              continue;
            }
            const std::size_t byte_slot = (sample_index * baseline.size()) + i;
            if (byte_inclusion_flags[byte_slot] == 0U) {
              byte_inclusion_flags[byte_slot] = 1U;
              ++trial_included_byte_slots;
            }
            const std::size_t bit_slot_base = byte_slot * 8U;
            for (std::size_t bit = 0; bit < 8U; ++bit) {
              if (((diff >> bit) & 1U) == 0U) {
                continue;
              }
              if (bit_inclusion_flags[bit_slot_base + bit] == 0U) {
                bit_inclusion_flags[bit_slot_base + bit] = 1U;
                ++trial_included_bit_slots;
              }
            }
          }
          compared_bytes += baseline.size();
          local_compared_bytes += baseline.size();
        }

        if (local_compared_bytes > 0U) {
          const double local_byte_ratio =
              static_cast<double>(local_differing_bytes) /
              static_cast<double>(local_compared_bytes);
          const double local_bit_ratio =
              static_cast<double>(local_differing_bits) /
              static_cast<double>(local_compared_bytes * 8U);
          result.avalanche_min_byte_ratio =
              std::min(result.avalanche_min_byte_ratio, local_byte_ratio);
          result.avalanche_max_byte_ratio =
              std::max(result.avalanche_max_byte_ratio, local_byte_ratio);
          result.avalanche_min_bit_ratio =
              std::min(result.avalanche_min_bit_ratio, local_bit_ratio);
          result.avalanche_max_bit_ratio =
              std::max(result.avalanche_max_bit_ratio, local_bit_ratio);

          if (!bic_sample_positions.empty()) {
            std::vector<std::uint8_t> observation_bits(bic_sample_positions.size(), 0U);
            for (std::size_t sample_index = 0; sample_index < bic_sample_positions.size(); ++sample_index) {
              const std::size_t bit_position = bic_sample_positions[sample_index];
              const std::size_t byte_position = bit_position / 8U;
              const std::size_t bit_offset = bit_position % 8U;
              observation_bits[sample_index] =
                  static_cast<std::uint8_t>((flattened_diff[byte_position] >> bit_offset) & 1U);
            }
            for (std::size_t left = 0; left < observation_bits.size(); ++left) {
              bic_one_counts[left] += observation_bits[left];
              const std::size_t row_offset = left * observation_bits.size();
              for (std::size_t right = 0; right < observation_bits.size(); ++right) {
                bic_pair_counts[row_offset + right] +=
                    static_cast<std::uint64_t>(observation_bits[left] & observation_bits[right]);
              }
            }
            bic_observation_count += 1U;
          }
        }
      }

      included_byte_slots += trial_included_byte_slots;
      included_bit_slots += trial_included_bit_slots;
      total_inclusion_byte_slots +=
          static_cast<std::uint64_t>(sampled_baseline_blocks.size()) * PASSWORD_EXPANDED_SIZE;
      total_inclusion_bit_slots +=
          static_cast<std::uint64_t>(sampled_baseline_blocks.size()) * PASSWORD_EXPANDED_SIZE * 8U;

      for (std::size_t second_trial = 0; second_trial < options.second_order_trials; ++second_trial) {
        std::vector<std::uint8_t> alt_source = initial_source;
        std::vector<std::uint8_t> alt_source_b = initial_source;
        std::vector<std::uint8_t> alt_source_ab = initial_source;
        const std::uint64_t token_a =
            Mix64(trial_seed ^ (static_cast<std::uint64_t>(second_trial) * 0x94D049BB133111EBULL) ^
                  0xA0761D6478BD642FULL);
        std::uint64_t token_b =
            Mix64(trial_seed ^ (static_cast<std::uint64_t>(second_trial) * 0xBF58476D1CE4E5B9ULL) ^
                  0xE7037ED1A0B428DBULL);
        const std::size_t flip_index_a =
            static_cast<std::size_t>(token_a % PASSWORD_EXPANDED_SIZE);
        const std::uint8_t bit_mask_a =
            static_cast<std::uint8_t>(1U << ((token_a >> 8U) & 7U));
        std::size_t flip_index_b =
            static_cast<std::size_t>(token_b % PASSWORD_EXPANDED_SIZE);
        std::uint8_t bit_mask_b =
            static_cast<std::uint8_t>(1U << ((token_b >> 8U) & 7U));
        if (flip_index_a == flip_index_b && bit_mask_a == bit_mask_b) {
          token_b = Mix64(token_b ^ 0x6A09E667F3BCC909ULL);
          flip_index_b = static_cast<std::size_t>(token_b % PASSWORD_EXPANDED_SIZE);
          bit_mask_b = static_cast<std::uint8_t>(1U << ((token_b >> 8U) & 7U));
        }
        alt_source[flip_index_a] ^= bit_mask_a;
        alt_source_b[flip_index_b] ^= bit_mask_b;
        alt_source_ab[flip_index_a] ^= bit_mask_a;
        alt_source_ab[flip_index_b] ^= bit_mask_b;

        const std::vector<std::uint8_t> alt_stream =
            GenerateExpandedStream(candidate.function, alt_source, options.stream_bytes);
        const std::vector<std::uint8_t> alt_stream_b =
            GenerateExpandedStream(candidate.function, alt_source_b, options.stream_bytes);
        const std::vector<std::uint8_t> alt_stream_ab =
            GenerateExpandedStream(candidate.function, alt_source_ab, options.stream_bytes);

        for (std::size_t sample_index = 0; sample_index < sampled_block_positions.size(); ++sample_index) {
          const std::vector<std::uint8_t>& baseline = sampled_baseline_blocks[sample_index];
          const std::uint8_t* block_a =
              BlockData(alt_stream, sampled_block_positions[sample_index]);
          const std::uint8_t* block_b =
              BlockData(alt_stream_b, sampled_block_positions[sample_index]);
          const std::uint8_t* block_ab =
              BlockData(alt_stream_ab, sampled_block_positions[sample_index]);
          for (std::size_t i = 0; i < baseline.size(); ++i) {
            const std::uint8_t second_diff = static_cast<std::uint8_t>(
                baseline[i] ^ block_a[i] ^ block_b[i] ^ block_ab[i]);
            second_order_differing_bytes += static_cast<std::uint64_t>(second_diff != 0U);
            second_order_differing_bits += static_cast<std::uint64_t>(
                std::popcount(static_cast<unsigned int>(second_diff)));
          }
          second_order_compared_bytes += baseline.size();
        }
      }
    }
  }

  double entropy = 0.0;
  double chi_squared = 0.0;
  double max_deviation = 0.0;
  std::uint64_t most_common_byte_count = 0U;
  std::uint64_t least_common_byte_count = std::numeric_limits<std::uint64_t>::max();
  if (processed_bytes > 0U) {
    const double total = static_cast<double>(processed_bytes);
    const double expected = total / 256.0;
    for (std::size_t i = 0; i < histogram.size(); ++i) {
      const double count = static_cast<double>(histogram[i]);
      most_common_byte_count = std::max(most_common_byte_count, histogram[i]);
      least_common_byte_count = std::min(least_common_byte_count, histogram[i]);
      if (count > 0.0) {
        const double probability = count / total;
        entropy -= probability * std::log2(probability);
      }
      const double diff = count - expected;
      chi_squared += (diff * diff) / expected;
      max_deviation = std::max(max_deviation, std::fabs(diff) / expected);
    }
  }
  result.entropy = entropy;
  result.reduced_chi_squared = chi_squared / 255.0;
  result.max_deviation = max_deviation;
  if (processed_bytes == 0U) {
    least_common_byte_count = 0U;
  }
  result.most_common_byte_count = most_common_byte_count;
  result.least_common_byte_count = least_common_byte_count;
  const double expected = processed_bytes > 0U
                              ? static_cast<double>(processed_bytes) / 256.0
                              : 1.0;
  result.byte_count_spread_ratio =
      expected > 0.0
          ? static_cast<double>(most_common_byte_count - least_common_byte_count) / expected
          : 0.0;

  const double score_entropy = Clamp((entropy - 7.0) * 100.0, 0.0, 100.0);
  const double score_chi =
      Clamp(100.0 - (std::fabs(result.reduced_chi_squared - 1.0) * 40.0), 0.0, 100.0);
  const double score_deviation = Clamp(100.0 - (max_deviation * 900.0), 0.0, 100.0);
  const double score_spread =
      Clamp(100.0 - (result.byte_count_spread_ratio * 12.5), 0.0, 100.0);
  // Byte-count skew is the primary uniformity signal for this project, so make it dominant.
  result.uniformity_score =
      (score_spread * 0.55) + (score_entropy * 0.20) + (score_chi * 0.15) +
      (score_deviation * 0.10);

  const std::size_t adjacent_count = processed_bytes > 0U ? processed_bytes - 1U : 0U;
  double equal_rate_drift = 1.0;
  double correlation = 0.0;
  double conditional_entropy = 0.0;
  if (adjacent_count > 0U) {
    const double adjacent_total = static_cast<double>(adjacent_count);
    const double equal_rate = static_cast<double>(equal_adjacent) / adjacent_total;
    equal_rate_drift = std::fabs(equal_rate - (1.0 / 256.0));
    const long double count_ld = static_cast<long double>(adjacent_count);
    const long double mean_x = sum_x / count_ld;
    const long double mean_y = sum_y / count_ld;
    const long double variance_x = (sum_x2 / count_ld) - (mean_x * mean_x);
    const long double variance_y = (sum_y2 / count_ld) - (mean_y * mean_y);
    const long double covariance = (sum_xy / count_ld) - (mean_x * mean_y);
    if (variance_x > 0.0L && variance_y > 0.0L) {
      correlation = static_cast<double>(covariance / std::sqrt(variance_x * variance_y));
    }

    for (std::size_t prev = 0; prev < 256U; ++prev) {
      std::uint64_t row_total = 0U;
      const std::size_t row_offset = prev * 256U;
      for (std::size_t curr = 0; curr < 256U; ++curr) {
        row_total += transitions[row_offset + curr];
      }
      if (row_total == 0U) {
        continue;
      }
      double row_entropy = 0.0;
      const double row_total_double = static_cast<double>(row_total);
      for (std::size_t curr = 0; curr < 256U; ++curr) {
        const double cell = static_cast<double>(transitions[row_offset + curr]);
        if (cell == 0.0) {
          continue;
        }
        const double probability = cell / row_total_double;
        row_entropy -= probability * std::log2(probability);
      }
      conditional_entropy += (row_total_double / adjacent_total) * row_entropy;
    }
  }
  result.equal_rate_drift = equal_rate_drift;
  result.correlation = correlation;
  result.conditional_entropy = conditional_entropy;

  const double score_drift = Clamp(100.0 - (equal_rate_drift * 20000.0), 0.0, 100.0);
  const double score_correlation = Clamp(100.0 - (std::fabs(correlation) * 400.0), 0.0, 100.0);
  const double score_conditional = Clamp((conditional_entropy - 6.5) * 66.6666667, 0.0, 100.0);
  result.predictability_score =
      (score_drift * 0.30) + (score_correlation * 0.30) + (score_conditional * 0.40);

  if (!result.repeat_found) {
    result.repeat_score = 100.0;
  } else {
    const double repeat_ratio =
        static_cast<double>(result.first_repeat_window + 16U) /
        std::max<double>(1.0, static_cast<double>(options.stream_bytes));
    result.repeat_score = Clamp(100.0 * std::sqrt(repeat_ratio), 0.0, 99.0);
  }

  if (!result.cycle_found) {
    result.cycle_score = 100.0;
  } else {
    const double first_block_factor =
        std::min(1.0, static_cast<double>(result.first_cycle_block + 1U) / 32.0);
    const double cycle_length_factor =
        std::min(1.0, static_cast<double>(std::max<std::size_t>(1U, result.cycle_length)) / 32.0);
    result.cycle_score = Clamp(100.0 * first_block_factor * cycle_length_factor, 0.0, 99.0);
  }

  if (compared_bytes > 0U) {
    result.avalanche_byte_ratio =
        static_cast<double>(differing_bytes) / static_cast<double>(compared_bytes);
    result.avalanche_bit_ratio =
        static_cast<double>(differing_bits) / static_cast<double>(compared_bytes * 8U);
  } else {
    result.avalanche_min_byte_ratio = 0.0;
    result.avalanche_max_byte_ratio = 0.0;
    result.avalanche_min_bit_ratio = 0.0;
    result.avalanche_max_bit_ratio = 0.0;
  }
  if (total_inclusion_byte_slots > 0U) {
    result.bit_inclusion_byte_ratio =
        static_cast<double>(included_byte_slots) /
        static_cast<double>(total_inclusion_byte_slots);
  }
  if (total_inclusion_bit_slots > 0U) {
    result.bit_inclusion_bit_ratio =
        static_cast<double>(included_bit_slots) /
        static_cast<double>(total_inclusion_bit_slots);
  }
  if (second_order_compared_bytes > 0U) {
    result.second_order_byte_ratio =
        static_cast<double>(second_order_differing_bytes) /
        static_cast<double>(second_order_compared_bytes);
    result.second_order_bit_ratio =
        static_cast<double>(second_order_differing_bits) /
        static_cast<double>(second_order_compared_bytes * 8U);
  }

  if (bic_observation_count > 0U && !bic_one_counts.empty()) {
    double bit_bias_sum = 0.0;
    double pair_bias_sum = 0.0;
    std::size_t pair_count = 0U;
    for (std::size_t left = 0; left < bic_one_counts.size(); ++left) {
      const double p_left =
          static_cast<double>(bic_one_counts[left]) / static_cast<double>(bic_observation_count);
      bit_bias_sum += std::fabs(p_left - 0.5);
      const std::size_t row_offset = left * bic_one_counts.size();
      for (std::size_t right = left + 1U; right < bic_one_counts.size(); ++right) {
        const double p_right =
            static_cast<double>(bic_one_counts[right]) / static_cast<double>(bic_observation_count);
        const double p_pair =
            static_cast<double>(bic_pair_counts[row_offset + right]) /
            static_cast<double>(bic_observation_count);
        pair_bias_sum += std::fabs(p_pair - (p_left * p_right));
        pair_count += 1U;
      }
    }
    result.bic_bit_bias = bit_bias_sum / static_cast<double>(bic_one_counts.size());
    result.bic_pair_bias =
        pair_count > 0U ? (pair_bias_sum / static_cast<double>(pair_count)) : 0.5;
  }

  result.cross_input_nearest_hamming = 128;
  for (std::size_t left = 0; left < cross_input_signatures.size(); ++left) {
    for (std::size_t right = left + 1U; right < cross_input_signatures.size(); ++right) {
      if (cross_input_signatures[left] == cross_input_signatures[right]) {
        result.cross_input_collision_found = true;
      }
      const int hamming =
          std::popcount(cross_input_signatures[left].lo ^ cross_input_signatures[right].lo) +
          std::popcount(cross_input_signatures[left].hi ^ cross_input_signatures[right].hi);
      result.cross_input_nearest_hamming =
          std::min(result.cross_input_nearest_hamming, hamming);
    }
  }
  if (cross_input_signatures.size() <= 1U) {
    result.cross_input_nearest_hamming = 128;
  }

  constexpr double kIdealAvalancheBitRatio = 0.5;
  constexpr double kIdealAvalancheByteRatio = 255.0 / 256.0;
  const double score_avalanche_bits_avg =
      Clamp(100.0 - (std::fabs(result.avalanche_bit_ratio - kIdealAvalancheBitRatio) / 0.06) * 100.0,
            0.0, 100.0);
  const double score_avalanche_bytes_avg =
      Clamp(100.0 - (std::fabs(result.avalanche_byte_ratio - kIdealAvalancheByteRatio) / 0.03) * 100.0,
            0.0, 100.0);
  const double score_avalanche_bits_floor =
      Clamp(((result.avalanche_min_bit_ratio - 0.42) / 0.08) * 100.0, 0.0, 100.0);
  const double score_avalanche_bytes_floor =
      Clamp(((result.avalanche_min_byte_ratio - 0.94) / 0.05) * 100.0, 0.0, 100.0);
  const double score_avalanche_bits_spread =
      Clamp(100.0 - ((result.avalanche_max_bit_ratio - result.avalanche_min_bit_ratio) / 0.06) * 100.0,
            0.0, 100.0);
  const double score_avalanche_bytes_spread =
      Clamp(100.0 - ((result.avalanche_max_byte_ratio - result.avalanche_min_byte_ratio) / 0.04) * 100.0,
            0.0, 100.0);
  result.avalanche_score =
      (score_avalanche_bits_avg * 0.40) + (score_avalanche_bits_floor * 0.25) +
      (score_avalanche_bits_spread * 0.20) + (score_avalanche_bytes_avg * 0.05) +
      (score_avalanche_bytes_floor * 0.05) + (score_avalanche_bytes_spread * 0.05);
  result.completeness_score =
      (Clamp(((result.avalanche_min_byte_ratio - 0.94) / 0.05) * 100.0, 0.0, 100.0) * 0.40) +
      (Clamp(((result.bit_inclusion_byte_ratio - 0.92) / 0.07) * 100.0, 0.0, 100.0) * 0.25) +
      (Clamp(((result.avalanche_min_bit_ratio - 0.43) / 0.07) * 100.0, 0.0, 100.0) * 0.20) +
      (Clamp(((result.bit_inclusion_bit_ratio - 0.72) / 0.20) * 100.0, 0.0, 100.0) * 0.15);

  const double ideal_bit_inclusion_ratio =
      1.0 - std::pow(0.5, static_cast<double>(options.avalanche_trials));
  const double ideal_byte_inclusion_ratio =
      1.0 - std::pow(1.0 - kIdealAvalancheByteRatio, static_cast<double>(options.avalanche_trials));
  const double score_inclusion_bits =
      Clamp(100.0 - (std::fabs(result.bit_inclusion_bit_ratio - ideal_bit_inclusion_ratio) / 0.08) * 100.0,
            0.0, 100.0);
  const double score_inclusion_bytes =
      Clamp(100.0 - (std::fabs(result.bit_inclusion_byte_ratio - ideal_byte_inclusion_ratio) / 0.03) * 100.0,
            0.0, 100.0);
  result.bit_inclusion_score =
      (score_inclusion_bits * 0.75) + (score_inclusion_bytes * 0.25);
  const double score_bic_bits =
      Clamp(100.0 - (result.bic_bit_bias / 0.10) * 100.0, 0.0, 100.0);
  const double score_bic_pairs =
      Clamp(100.0 - (result.bic_pair_bias / 0.06) * 100.0, 0.0, 100.0);
  result.bic_score = (score_bic_bits * 0.35) + (score_bic_pairs * 0.65);
  const double score_second_order_bits =
      Clamp(100.0 - (std::fabs(result.second_order_bit_ratio - 0.5) / 0.08) * 100.0, 0.0, 100.0);
  const double score_second_order_bytes =
      Clamp(100.0 - (std::fabs(result.second_order_byte_ratio - kIdealAvalancheByteRatio) / 0.05) * 100.0,
            0.0, 100.0);
  result.nonlinearity_score =
      (score_second_order_bits * 0.80) + (score_second_order_bytes * 0.20);
  double collision_score =
      Clamp((static_cast<double>(result.cross_input_nearest_hamming) / 96.0) * 100.0, 0.0, 100.0);
  if (result.cross_input_collision_found) {
    collision_score = Clamp(collision_score - 50.0, 0.0, 100.0);
  }
  result.cross_input_collision_score = collision_score;

  for (int bit = 0; bit < 64; ++bit) {
    if (simhash_accumulator[bit] >= 0) {
      result.signature_lo |= (1ULL << bit);
    }
    if (simhash_accumulator[64 + bit] >= 0) {
      result.signature_hi |= (1ULL << bit);
    }
  }

  return result;
}

CandidateResult EvaluateCandidate(
    const RegisteredCandidate& candidate,
    const Options& options) {
  const std::vector<TrialSpec> all_trials = BuildTrialPlan(options);
  CandidateResult overall = EvaluateCandidateForPlan(candidate, options, all_trials);

  for (const CategoryConfig& config : CategoryConfigs()) {
    const std::size_t effective_trials = EffectiveTrialCount(options, config);
    if (effective_trials == 0U) {
      continue;
    }
    std::vector<TrialSpec> category_trials;
    category_trials.reserve(effective_trials);
    for (std::size_t i = 0; i < effective_trials; ++i) {
      category_trials.push_back({config.category, i, i});
    }
    CandidateResult category_result =
        EvaluateCandidateForPlan(candidate, options, category_trials);
    category_result.distinctness_score = 100.0;
    std::vector<CandidateResult> single_result = {category_result};
    FinalizeScores(single_result, options);
    category_result = single_result.front();
    const std::size_t index = CategoryIndex(config.category);
    overall.category_present[index] = true;
    overall.category_scores[index] = category_result.composite_score;
    overall.category_rejected[index] = category_result.rejected;
    overall.category_grade_labels[index] = category_result.grade;
    overall.category_grade_ranks[index] = category_result.grade_rank;
  }
  return overall;
}

std::vector<std::uint8_t> GenerateCandidateStream(
    const RegisteredCandidate& candidate,
    const TrialSpec& trial,
    std::uint64_t base_seed,
    std::size_t stream_bytes) {
  std::vector<std::uint8_t> source(PASSWORD_EXPANDED_SIZE);
  std::vector<std::uint8_t> stream(stream_bytes);
  FillSourceForTrial(source, trial.category, base_seed, trial.ordinal_within_category);
  return GenerateExpandedStream(candidate.function, source, stream.size());
}

bool VerifyLongRepeats(
    std::vector<CandidateResult>& results,
    const std::vector<const RegisteredCandidate*>& candidates,
    const Options& options) {
  if (results.empty() || options.long_repeat_top_count == 0U || options.long_repeat_scan_bytes == 0U) {
    return false;
  }

  std::unordered_map<int, const RegisteredCandidate*> candidate_by_id;
  candidate_by_id.reserve(candidates.size() * 2U);
  for (const RegisteredCandidate* candidate : candidates) {
    candidate_by_id.emplace(candidate->candidate_id, candidate);
  }

  bool verified_any = false;
  const std::size_t verify_count =
      std::min(options.long_repeat_top_count, results.size());
  for (std::size_t index = 0; index < verify_count; ++index) {
    CandidateResult& result = results[index];
    if (result.long_repeat_verified) {
      continue;
    }
    const auto candidate_it = candidate_by_id.find(result.candidate_id);
    if (candidate_it == candidate_by_id.end()) {
      continue;
    }
    const RegisteredCandidate& candidate = *candidate_it->second;
    result.long_repeat_verified = true;
    verified_any = true;

    const std::vector<TrialSpec> plan = BuildTrialPlan(options);
    for (const TrialSpec& trial : plan) {
      const std::vector<std::uint8_t> stream = GenerateCandidateStream(
          candidate, trial, options.seed, options.long_repeat_scan_bytes);
      const ExactRepeatMatch repeat_match =
          FindBestRepeatedMatchLZ(stream, options.long_repeat_min_match_bytes);
      if (!repeat_match.found) {
        continue;
      }
      if (!result.long_repeat_match_found ||
          repeat_match.match_length > result.long_repeat_match_length ||
          (repeat_match.match_length == result.long_repeat_match_length &&
           repeat_match.first_position < result.long_repeat_match_position)) {
        result.long_repeat_match_found = true;
        result.long_repeat_match_position = repeat_match.first_position;
        result.long_repeat_match_trial = trial.global_index;
        result.long_repeat_match_length = repeat_match.match_length;
      }
    }
  }

  return verified_any;
}

void ComputeDistinctness(std::vector<CandidateResult>& results) {
  if (results.size() <= 1U) {
    for (CandidateResult& result : results) {
      result.distinctness_score = 100.0;
    }
    return;
  }

  for (std::size_t i = 0; i < results.size(); ++i) {
    int nearest_hamming = 128;
    bool exact_first_block_collision = false;
    for (std::size_t j = 0; j < results.size(); ++j) {
      if (i == j) {
        continue;
      }
      exact_first_block_collision |=
          results[i].first_block_hash == results[j].first_block_hash;
      const int hamming = std::popcount(results[i].signature_lo ^ results[j].signature_lo) +
                          std::popcount(results[i].signature_hi ^ results[j].signature_hi);
      nearest_hamming = std::min(nearest_hamming, hamming);
    }

    double score = Clamp((static_cast<double>(nearest_hamming) / 64.0) * 100.0, 0.0, 100.0);
    if (exact_first_block_collision) {
      score = Clamp(score - 50.0, 0.0, 100.0);
    }
    results[i].distinctness_score = score;
  }
}

void FinalizeScores(std::vector<CandidateResult>& results, const Options& options) {
  for (CandidateResult& result : results) {
    if (result.long_repeat_match_found) {
      const double repeat_ratio =
          static_cast<double>(result.long_repeat_match_position +
                              std::max<std::size_t>(1U, result.long_repeat_match_length)) /
          std::max<double>(1.0, static_cast<double>(options.long_repeat_scan_bytes));
      const double length_factor = Clamp(
          static_cast<double>(options.long_repeat_min_match_bytes) /
              static_cast<double>(std::max<std::size_t>(1U, result.long_repeat_match_length)),
          0.20, 1.00);
      const double long_repeat_score =
          Clamp(100.0 * std::sqrt(repeat_ratio) * length_factor, 0.0, 95.0);
      result.repeat_score = std::min(result.repeat_score, long_repeat_score);
    }

    const double structural_composite_score =
        (result.repeat_score * 0.12) + (result.cycle_score * 0.10) +
        (result.uniformity_score * 0.12) + (result.predictability_score * 0.10) +
        (result.avalanche_score * 0.16) + (result.completeness_score * 0.10) +
        (result.bic_score * 0.10) + (result.bit_inclusion_score * 0.08) +
        (result.nonlinearity_score * 0.08) +
        (result.cross_input_collision_score * 0.10) +
        (result.distinctness_score * 0.04);
    result.composite_score = structural_composite_score;

    double weighted_category_score = 0.0;
    std::size_t weighted_category_trials = 0;
    double weakest_category_score = std::numeric_limits<double>::max();
    std::vector<double> present_category_scores;
    for (const CategoryConfig& config : CategoryConfigs()) {
      const std::size_t index = CategoryIndex(config.category);
      if (!result.category_present[index] || config.trial_count == 0U) {
        continue;
      }
      weighted_category_score += result.category_scores[index] * static_cast<double>(config.trial_count);
      weighted_category_trials += config.trial_count;
      weakest_category_score = std::min(weakest_category_score, result.category_scores[index]);
      present_category_scores.push_back(result.category_scores[index]);
    }
    if (weighted_category_trials > 0U) {
      const double average_category_score =
          weighted_category_score / static_cast<double>(weighted_category_trials);
      std::sort(present_category_scores.begin(), present_category_scores.end());
      const std::size_t lower_tail_count = std::min<std::size_t>(2U, present_category_scores.size());
      double lower_tail_score = weakest_category_score;
      if (lower_tail_count > 0U) {
        lower_tail_score =
            std::accumulate(
                present_category_scores.begin(),
                present_category_scores.begin() + static_cast<std::ptrdiff_t>(lower_tail_count), 0.0) /
            static_cast<double>(lower_tail_count);
      }
      result.composite_score =
          (structural_composite_score * 0.20) +
          (average_category_score * 0.35) +
          (lower_tail_score * 0.25) +
          (weakest_category_score * 0.20);
      result.composite_score = Clamp(result.composite_score, 0.0, 100.0);
    }

    const std::array<std::pair<const char*, double>, 11> components = {{
        {"repeat resistance", result.repeat_score},
        {"cycle resistance", result.cycle_score},
        {"uniformity", result.uniformity_score},
        {"predictability", result.predictability_score},
        {"avalanche", result.avalanche_score},
        {"completeness", result.completeness_score},
        {"BIC", result.bic_score},
        {"bit inclusion", result.bit_inclusion_score},
        {"nonlinearity", result.nonlinearity_score},
        {"cross-input collision", result.cross_input_collision_score},
        {"distinctness", result.distinctness_score},
    }};

    auto weakest = std::min_element(
        components.begin(), components.end(),
        [](const auto& left, const auto& right) { return left.second < right.second; });
    result.failure_reason = weakest->first;
    if (weighted_category_trials > 0U) {
      double weakest_category_score_for_reason = std::numeric_limits<double>::max();
      const char* weakest_category_name = weakest->first;
      for (const CategoryConfig& config : CategoryConfigs()) {
        const std::size_t index = CategoryIndex(config.category);
        if (!result.category_present[index]) {
          continue;
        }
        if (result.category_scores[index] < weakest_category_score_for_reason) {
          weakest_category_score_for_reason = result.category_scores[index];
          weakest_category_name = TrialCategoryName(config.category);
        }
      }
      result.failure_reason = weakest_category_name;
    }

    if (result.long_repeat_match_found) {
      result.failure_reason = "long repeat match";
    }

    const bool catastrophic_repeat =
        (result.repeat_found && result.first_repeat_window < 65536U) ||
        result.long_repeat_match_found;
    const bool catastrophic_cycle =
        result.cycle_found && result.cycle_length > 0U && result.cycle_length < 4U;
    const bool catastrophic_uniformity =
        result.entropy < 7.0 || result.max_deviation > 0.35;
    const bool catastrophic_diffusion =
        result.avalanche_min_bit_ratio < 0.30 || result.completeness_score < 45.0 ||
        result.bit_inclusion_score < 45.0;
    const bool catastrophic_structure =
        result.bic_score < 15.0 || result.nonlinearity_score < 35.0;

    result.rejected = catastrophic_repeat || catastrophic_cycle || catastrophic_uniformity ||
                      catastrophic_diffusion || catastrophic_structure;

    if (result.rejected) {
      result.composite_score = Clamp(result.composite_score - 25.0, 0.0, 100.0);
    }

    const GradeInfo grade = ComputeGrade(result.composite_score);
    result.grade = grade.label;
    result.grade_rank = grade.rank;
  }
}

bool CompareResults(const CandidateResult& left, const CandidateResult& right) {
  if (left.grade_rank != right.grade_rank) {
    return left.grade_rank > right.grade_rank;
  }
  if (left.composite_score != right.composite_score) {
    return left.composite_score > right.composite_score;
  }
  return left.candidate_id < right.candidate_id;
}

void WriteCsv(
    const std::filesystem::path& path,
    const std::vector<CandidateResult>& results,
    const Options& options) {
  std::ofstream output(path);
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan(options);
  output
      << "input_mix,candidate_id,function_name,grade,grade_rank,composite_score,repeat_score,cycle_score,"
      << "uniformity_score,predictability_score,avalanche_score,completeness_score,bic_score,"
      << "bit_inclusion_score,nonlinearity_score,cross_input_collision_score,distinctness_score,"
      << "entropy,reduced_chi_squared,max_deviation,byte_count_spread_ratio,"
      << "most_common_byte_count,least_common_byte_count,equal_rate_drift,correlation,"
      << "conditional_entropy,avalanche_byte_ratio,avalanche_bit_ratio,"
      << "avalanche_min_byte_ratio,avalanche_max_byte_ratio,avalanche_min_bit_ratio,avalanche_max_bit_ratio,"
      << "bit_inclusion_byte_ratio,bit_inclusion_bit_ratio,bic_bit_bias,bic_pair_bias,"
      << "second_order_byte_ratio,second_order_bit_ratio,cross_input_nearest_hamming,cross_input_collision_found,repeat_found,"
      << "first_repeat_window,cycle_found,first_cycle_block,cycle_length,long_repeat_verified,"
      << "long_repeat_match_found,long_repeat_match_position,long_repeat_match_trial,long_repeat_match_length,"
      << "first_block_hash,signature_lo,signature_hi,rejected,"
      << "failure_reason,recipe_summary,"
      << "aes_score,aes_grade,chacha_score,chacha_grade,zeros_score,zeros_grade,ones_score,ones_grade,"
      << "predictable_a_score,predictable_a_grade,predictable_b_score,predictable_b_grade,predictable_c_score,predictable_c_grade\n";

  for (const CandidateResult& result : results) {
    output << "mixed" << ','
           << result.candidate_id << ','
           << result.function_name << ','
           << result.grade << ','
           << result.grade_rank << ','
           << FormatDouble(result.composite_score) << ','
           << FormatDouble(result.repeat_score) << ','
           << FormatDouble(result.cycle_score) << ','
           << FormatDouble(result.uniformity_score) << ','
           << FormatDouble(result.predictability_score) << ','
           << FormatDouble(result.avalanche_score) << ','
           << FormatDouble(result.completeness_score) << ','
           << FormatDouble(result.bic_score) << ','
           << FormatDouble(result.bit_inclusion_score) << ','
           << FormatDouble(result.nonlinearity_score) << ','
           << FormatDouble(result.cross_input_collision_score) << ','
           << FormatDouble(result.distinctness_score) << ','
           << FormatDouble(result.entropy) << ','
           << FormatDouble(result.reduced_chi_squared) << ','
           << FormatDouble(result.max_deviation) << ','
           << FormatDouble(result.byte_count_spread_ratio) << ','
           << result.most_common_byte_count << ','
           << result.least_common_byte_count << ','
           << FormatDouble(result.equal_rate_drift) << ','
           << FormatDouble(result.correlation) << ','
           << FormatDouble(result.conditional_entropy) << ','
           << FormatDouble(result.avalanche_byte_ratio) << ','
           << FormatDouble(result.avalanche_bit_ratio) << ','
           << FormatDouble(result.avalanche_min_byte_ratio) << ','
           << FormatDouble(result.avalanche_max_byte_ratio) << ','
           << FormatDouble(result.avalanche_min_bit_ratio) << ','
           << FormatDouble(result.avalanche_max_bit_ratio) << ','
           << FormatDouble(result.bit_inclusion_byte_ratio) << ','
           << FormatDouble(result.bit_inclusion_bit_ratio) << ','
           << FormatDouble(result.bic_bit_bias) << ','
           << FormatDouble(result.bic_pair_bias) << ','
           << FormatDouble(result.second_order_byte_ratio) << ','
           << FormatDouble(result.second_order_bit_ratio) << ','
           << result.cross_input_nearest_hamming << ','
           << (result.cross_input_collision_found ? "true" : "false") << ','
           << (result.repeat_found ? "true" : "false") << ','
           << (result.repeat_found ? std::to_string(result.first_repeat_window) : "") << ','
           << (result.cycle_found ? "true" : "false") << ','
           << (result.cycle_found ? std::to_string(result.first_cycle_block) : "") << ','
           << (result.cycle_found ? std::to_string(result.cycle_length) : "") << ','
           << (result.long_repeat_verified ? "true" : "false") << ','
           << (result.long_repeat_match_found ? "true" : "false") << ','
           << (result.long_repeat_match_found ? std::to_string(result.long_repeat_match_position) : "") << ','
           << (result.long_repeat_match_found ? std::to_string(result.long_repeat_match_trial) : "") << ','
           << (result.long_repeat_match_found ? std::to_string(result.long_repeat_match_length) : "") << ','
           << result.first_block_hash << ','
           << result.signature_lo << ','
           << result.signature_hi << ','
           << (result.rejected ? "true" : "false") << ','
           << '"' << result.failure_reason << '"' << ','
           << '"' << result.recipe_summary << '"' << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kAES)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kAES)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kChaCha)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kChaCha)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kZeros)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kZeros)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kOnes)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kOnes)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kPredictableA)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kPredictableA)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kPredictableB)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kPredictableB)] << ','
           << FormatDouble(result.category_scores[CategoryIndex(TrialCategory::kPredictableC)]) << ','
           << result.category_grade_labels[CategoryIndex(TrialCategory::kPredictableC)] << '\n';
  }
}

void WriteSummary(
    const std::filesystem::path& path,
    const std::vector<CandidateResult>& results,
    const Options& options) {
  std::ofstream output(path);
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan(options);
  std::size_t rejected = 0;
  for (const CandidateResult& result : results) {
    rejected += static_cast<std::size_t>(result.rejected);
  }

  output << "Twist Candidate Summary\n";
  output << "=======================\n";
  output << "input_mix=mixed\n";
  output << "seed=" << options.seed << '\n';
  output << "length_factor=" << options.length_factor << '\n';
  output << "stream_bytes=" << options.stream_bytes << '\n';
  output << "stream_windows=" << WindowCountForBytes(options.stream_bytes) << '\n';
  output << "trial_count=" << trial_plan.size() << '\n';
  output << "long_repeat_scan_bytes=" << options.long_repeat_scan_bytes << '\n';
  output << "long_repeat_windows=" << WindowCountForBytes(options.long_repeat_scan_bytes) << '\n';
  output << "long_repeat_top_count=" << options.long_repeat_top_count << '\n';
  output << "source_patterns=";
  for (std::size_t trial = 0; trial < trial_plan.size(); ++trial) {
    if (trial > 0U) {
      output << ',';
    }
    output << InputLabel(trial_plan[trial].category, trial_plan[trial].ordinal_within_category);
  }
  output << '\n';
  for (const CategoryConfig& config : CategoryConfigs()) {
    output << "trial_count_" << TrialCategoryName(config.category) << "=" << config.trial_count << '\n';
  }
  output << '\n';
  output << "evaluated_candidates=" << results.size() << '\n';
  output << "rejected_candidates=" << rejected << "\n\n";

  std::vector<std::string> grade_order = {
      "A+", "A", "A-", "B+", "B", "B-", "C+", "C", "C-", "D+", "D", "D-", "F"};

  for (const std::string& grade : grade_order) {
    bool wrote_header = false;
    std::size_t rank = 0;
    for (const CandidateResult& result : results) {
      if (result.grade != grade) {
        continue;
      }
      if (!wrote_header) {
        output << grade << "\n";
        output << std::string(grade.size(), '-') << '\n';
        wrote_header = true;
      }
      rank += 1U;
      output << rank << ". "
             << "candidate_id=" << result.candidate_id
             << " function=" << result.function_name
             << " composite=" << FormatDouble(result.composite_score)
             << " repeat=" << FormatDouble(result.repeat_score)
             << " cycle=" << FormatDouble(result.cycle_score)
             << " uniformity=" << FormatDouble(result.uniformity_score)
             << " spread=" << FormatDouble(result.byte_count_spread_ratio)
             << " most=" << result.most_common_byte_count
             << " least=" << result.least_common_byte_count
             << " gap="
             << (result.most_common_byte_count - result.least_common_byte_count)
             << " predictability=" << FormatDouble(result.predictability_score)
             << " avalanche=" << FormatDouble(result.avalanche_score)
             << " completeness=" << FormatDouble(result.completeness_score)
             << " bic=" << FormatDouble(result.bic_score)
             << " avalanche_avg_bits=" << FormatDouble(result.avalanche_bit_ratio * 100.0)
             << "%"
             << " avalanche_min_bits=" << FormatDouble(result.avalanche_min_bit_ratio * 100.0)
             << "%"
             << " avalanche_max_bits=" << FormatDouble(result.avalanche_max_bit_ratio * 100.0)
             << "%"
             << " inclusion=" << FormatDouble(result.bit_inclusion_score)
             << " nonlinearity=" << FormatDouble(result.nonlinearity_score)
             << " cross_input_collision=" << FormatDouble(result.cross_input_collision_score)
             << " distinctness=" << FormatDouble(result.distinctness_score)
             << " repeat_scan="
             << (result.long_repeat_match_found
                     ? (std::to_string(result.long_repeat_match_length) + "@" +
                        std::to_string(result.long_repeat_match_position))
                     : (result.long_repeat_verified ? "clear" : "not-run"))
             << " rejected=" << (result.rejected ? "true" : "false")
             << " failure_reason=" << result.failure_reason << '\n';
      output << "   recipe=" << result.recipe_summary << '\n';
      for (const CategoryConfig& config : CategoryConfigs()) {
        const std::size_t index = CategoryIndex(config.category);
        if (!result.category_present[index]) {
          continue;
        }
        output << "   " << TrialCategoryName(config.category)
               << ": grade=" << result.category_grade_labels[index]
               << " composite=" << FormatDouble(result.category_scores[index])
               << " rejected=" << (result.category_rejected[index] ? "true" : "false") << '\n';
      }
      if (result.long_repeat_match_found) {
        output << "   long_repeat_match position=" << result.long_repeat_match_position
               << " length=" << result.long_repeat_match_length
               << " input=" << LabelForGlobalTrialIndex(result.long_repeat_match_trial) << '\n';
      }
    }
    if (wrote_header) {
      output << '\n';
    }
  }
}

#include "TwistCandidateHarnessHtml.inl"

std::optional<Options> ParseArgs(int argc, char** argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto require_value = [&](const char* flag) -> std::optional<std::string> {
      if (i + 1 >= argc) {
        std::cerr << "missing value for " << flag << '\n';
        return std::nullopt;
      }
      ++i;
      return std::string(argv[i]);
    };

    if (arg == "--seed") {
      const auto value = require_value("--seed");
      if (!value) {
        return std::nullopt;
      }
      options.seed = static_cast<std::uint64_t>(std::stoull(*value));
      options.seed_supplied = true;
    } else if (arg == "--input-suite") {
      const auto value = require_value("--input-suite");
      if (!value) {
        return std::nullopt;
      }
      options.legacy_input_suite = *value;
    } else if (arg == "--length-factor") {
      const auto value = require_value("--length-factor");
      if (!value) {
        return std::nullopt;
      }
      options.length_factor = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--stream-bytes") {
      const auto value = require_value("--stream-bytes");
      if (!value) {
        return std::nullopt;
      }
      options.stream_bytes = static_cast<std::size_t>(std::stoull(*value));
      options.stream_bytes_supplied = true;
    } else if (arg == "--trial-count") {
      const auto value = require_value("--trial-count");
      if (!value) {
        return std::nullopt;
      }
      std::cerr << "--trial-count is ignored; use per-category trial count knobs in Knobs.hpp\n";
    } else if (arg == "--trial-cap-per-category") {
      const auto value = require_value("--trial-cap-per-category");
      if (!value) {
        return std::nullopt;
      }
      options.trial_cap_per_category = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--cycle-block-count") {
      const auto value = require_value("--cycle-block-count");
      if (!value) {
        return std::nullopt;
      }
      std::cerr << "--cycle-block-count is ignored; run length is determined by --length-factor/--stream-bytes\n";
    } else if (arg == "--sample-windows") {
      const auto value = require_value("--sample-windows");
      if (!value) {
        return std::nullopt;
      }
      options.sample_windows = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--signature-bytes") {
      const auto value = require_value("--signature-bytes");
      if (!value) {
        return std::nullopt;
      }
      options.signature_bytes = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--avalanche-blocks") {
      const auto value = require_value("--avalanche-blocks");
      if (!value) {
        return std::nullopt;
      }
      options.avalanche_blocks = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--avalanche-trials") {
      const auto value = require_value("--avalanche-trials");
      if (!value) {
        return std::nullopt;
      }
      options.avalanche_trials = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--bic-sample-bits") {
      const auto value = require_value("--bic-sample-bits");
      if (!value) {
        return std::nullopt;
      }
      options.bic_sample_bits = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--second-order-trials") {
      const auto value = require_value("--second-order-trials");
      if (!value) {
        return std::nullopt;
      }
      options.second_order_trials = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--cross-input-signature-bytes") {
      const auto value = require_value("--cross-input-signature-bytes");
      if (!value) {
        return std::nullopt;
      }
      options.cross_input_signature_bytes = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--long-repeat-bytes") {
      const auto value = require_value("--long-repeat-bytes");
      if (!value) {
        return std::nullopt;
      }
      options.long_repeat_scan_bytes = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--long-repeat-top") {
      const auto value = require_value("--long-repeat-top");
      if (!value) {
        return std::nullopt;
      }
      options.long_repeat_top_count = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--long-repeat-min-match") {
      const auto value = require_value("--long-repeat-min-match");
      if (!value) {
        return std::nullopt;
      }
      options.long_repeat_min_match_bytes = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--top-n") {
      const auto value = require_value("--top-n");
      if (!value) {
        return std::nullopt;
      }
      options.top_n = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--candidate-id") {
      const auto value = require_value("--candidate-id");
      if (!value) {
        return std::nullopt;
      }
      options.candidate_id = std::stoi(*value);
      options.candidate_id_supplied = true;
    } else if (arg == "--limit") {
      const auto value = require_value("--limit");
      if (!value) {
        return std::nullopt;
      }
      options.limit = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--output-dir") {
      const auto value = require_value("--output-dir");
      if (!value) {
        return std::nullopt;
      }
      options.output_dir = *value;
    } else if (arg == "--help" || arg == "-h") {
      std::cout
          << "Usage: twist_candidate_harness [options]\n"
          << "  --seed <u64>\n"
          << "  --input-suite <legacy-compat-only>\n"
          << "  --length-factor <count>\n"
          << "  --stream-bytes <bytes>\n"
          << "  --trial-count <legacy-compat-only>\n"
          << "  --trial-cap-per-category <count>\n"
          << "  --sample-windows <count>\n"
          << "  --signature-bytes <bytes>\n"
          << "  --avalanche-blocks <count>\n"
          << "  --avalanche-trials <count>\n"
          << "  --bic-sample-bits <count>\n"
          << "  --second-order-trials <count>\n"
          << "  --cross-input-signature-bytes <bytes>\n"
          << "  --long-repeat-bytes <bytes>\n"
          << "  --long-repeat-top <count>\n"
          << "  --long-repeat-min-match <bytes>\n"
          << "  --top-n <count>\n"
          << "  --candidate-id <id>\n"
          << "  --limit <count>\n"
          << "  --output-dir <path>\n";
      return std::nullopt;
    } else {
      std::cerr << "unknown argument: " << arg << '\n';
      return std::nullopt;
    }
  }
  if (!options.seed_supplied) {
    options.seed = ResolveDefaultSeed();
  }
  if (!options.stream_bytes_supplied) {
    options.stream_bytes =
        PASSWORD_EXPANDED_SIZE * std::max<std::size_t>(1U, options.length_factor);
  }
  options.stream_bytes = RoundUpToWindowMultiple(options.stream_bytes);
  options.length_factor = WindowCountForBytes(options.stream_bytes);
  options.long_repeat_scan_bytes =
      RoundUpToWindowMultiple(options.long_repeat_scan_bytes);
  options.avalanche_blocks = std::max<std::size_t>(1U, options.avalanche_blocks);
  options.avalanche_trials = std::max<std::size_t>(1U, options.avalanche_trials);
  options.bic_sample_bits = std::max<std::size_t>(8U, options.bic_sample_bits);
  options.second_order_trials = std::max<std::size_t>(1U, options.second_order_trials);
  options.cross_input_signature_bytes =
      std::max<std::size_t>(64U, options.cross_input_signature_bytes);
  return options;
}

}  // namespace

int Main(int argc, char** argv) {
  const std::optional<Options> parsed_options = ParseArgs(argc, argv);
  if (!parsed_options.has_value()) {
    return 1;
  }
  const Options options = *parsed_options;

  std::vector<const RegisteredCandidate*> candidates;
  candidates.reserve(kRegisteredCandidateCount + kBaselineCandidateCount);
  for (std::size_t i = 0; i < kRegisteredCandidateCount; ++i) {
    const RegisteredCandidate& candidate = kRegisteredCandidates[i];
    if (options.candidate_id_supplied && candidate.candidate_id != options.candidate_id) {
      continue;
    }
    candidates.push_back(&candidate);
    if (options.limit > 0U && candidates.size() >= options.limit) {
      break;
    }
  }
  for (std::size_t i = 0; i < kBaselineCandidateCount; ++i) {
    const RegisteredCandidate& candidate = kBaselineCandidates[i];
    if (options.candidate_id_supplied && candidate.candidate_id != options.candidate_id) {
      continue;
    }
    candidates.push_back(&candidate);
    if (options.limit > 0U && candidates.size() >= options.limit) {
      break;
    }
  }

  if (candidates.empty()) {
    std::cerr << "no candidates matched the requested selection\n";
    return 2;
  }

  std::vector<CandidateResult> results;
  results.reserve(candidates.size());
  const auto evaluation_start = std::chrono::steady_clock::now();
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const RegisteredCandidate& candidate = *candidates[index];
    const auto candidate_start = std::chrono::steady_clock::now();
    results.push_back(EvaluateCandidate(candidate, options));
    const auto candidate_end = std::chrono::steady_clock::now();
    const double candidate_seconds =
        std::chrono::duration<double>(candidate_end - candidate_start).count();
    const double total_seconds =
        std::chrono::duration<double>(candidate_end - evaluation_start).count();
    std::cout << "progress "
              << (index + 1U) << "/" << candidates.size()
              << " candidate_id=" << candidate.candidate_id
              << " function=" << candidate.function_name
              << " elapsed=" << FormatDouble(total_seconds) << "s"
              << " candidate_time=" << FormatDouble(candidate_seconds) << "s"
              << '\n';
  }

  ComputeDistinctness(results);
  FinalizeScores(results, options);
  std::sort(results.begin(), results.end(), CompareResults);
  while (VerifyLongRepeats(results, candidates, options)) {
    FinalizeScores(results, options);
    std::sort(results.begin(), results.end(), CompareResults);
  }

  const std::filesystem::path output_dir(options.output_dir);
  std::filesystem::create_directories(output_dir);
  WriteCsv(output_dir / "twist_candidate_scores.csv", results, options);
  WriteSummary(output_dir / "twist_candidate_summary.txt", results, options);
  WriteHtmlReport(output_dir / "twist_candidate_report.html", results, options);

  std::cout << "evaluated " << results.size() << " candidates\n";
  std::cout << "scores: " << (output_dir / "twist_candidate_scores.csv") << '\n';
  std::cout << "summary: " << (output_dir / "twist_candidate_summary.txt") << '\n';
  std::cout << "html: " << (output_dir / "twist_candidate_report.html") << '\n';

  const std::size_t preview = std::min<std::size_t>(options.top_n, results.size());
  for (std::size_t i = 0; i < preview; ++i) {
    const CandidateResult& result = results[i];
    std::cout << std::setw(2) << (i + 1U) << ". candidate_id=" << result.candidate_id
              << " grade=" << result.grade
              << " composite=" << FormatDouble(result.composite_score)
              << " avalanche=" << FormatDouble(result.avalanche_score)
              << " inclusion=" << FormatDouble(result.bit_inclusion_score)
              << " function=" << result.function_name
              << " rejected=" << (result.rejected ? "true" : "false") << '\n';
  }

  return 0;
}

}  // namespace twist

int main(int argc, char** argv) {
  return twist::Main(argc, argv);
}
