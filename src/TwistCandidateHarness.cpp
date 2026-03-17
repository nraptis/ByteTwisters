#include "Knobs.hpp"
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
  std::size_t cycle_block_count = knobs::kDefaultCycleBlockCount;
  std::size_t sample_windows = knobs::kDefaultSampleWindows;
  std::size_t signature_bytes = knobs::kDefaultSignatureBytes;
  std::size_t avalanche_blocks = knobs::kDefaultAvalancheBlocks;
  std::size_t avalanche_trials = knobs::kDefaultAvalancheTrials;
  std::size_t long_repeat_scan_bytes = knobs::kLongRepeatScanBytes;
  std::size_t long_repeat_top_count = knobs::kLongRepeatTopCandidateCount;
  std::size_t long_repeat_window_a = knobs::kLongRepeatWindowBytesA;
  std::size_t long_repeat_window_b = knobs::kLongRepeatWindowBytesB;
  std::size_t top_n = knobs::kTopCandidateCount;
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

struct RollingHash64 {
  static constexpr std::uint64_t kBase = 11400714819323198485ULL;

  explicit RollingHash64(std::size_t window_size) : window_size(window_size) {
    for (std::size_t i = 0; i < window_size; ++i) {
      power *= kBase;
    }
  }

  void Initialize(const std::uint8_t* data) {
    hash = 0;
    for (std::size_t i = 0; i < window_size; ++i) {
      hash = (hash * kBase) + static_cast<std::uint64_t>(data[i]) + 1ULL;
    }
  }

  void Slide(std::uint8_t outgoing, std::uint8_t incoming) {
    hash = (hash * kBase) + static_cast<std::uint64_t>(incoming) + 1ULL;
    hash -= power * (static_cast<std::uint64_t>(outgoing) + 1ULL);
  }

  std::size_t window_size;
  std::uint64_t hash = 0;
  std::uint64_t power = 1;
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
  std::size_t first_repeat_window = std::numeric_limits<std::size_t>::max();
  std::size_t first_cycle_block = std::numeric_limits<std::size_t>::max();
  std::size_t cycle_length = 0;
  std::size_t exact_repeat_64_position = std::numeric_limits<std::size_t>::max();
  std::size_t exact_repeat_128_position = std::numeric_limits<std::size_t>::max();
  std::size_t exact_repeat_64_trial = std::numeric_limits<std::size_t>::max();
  std::size_t exact_repeat_128_trial = std::numeric_limits<std::size_t>::max();
  bool repeat_found = false;
  bool cycle_found = false;
  bool exact_repeat_64_found = false;
  bool exact_repeat_128_found = false;
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
  stream << std::fixed << std::setprecision(6) << value;
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

std::vector<TrialSpec> BuildTrialPlan() {
  std::vector<TrialSpec> plan;
  plan.reserve(TotalConfiguredTrials());
  std::size_t global_index = 0;
  for (const CategoryConfig& config : CategoryConfigs()) {
    for (std::size_t ordinal = 0; ordinal < config.trial_count; ++ordinal) {
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
  static const std::vector<TrialSpec> plan = BuildTrialPlan();
  if (global_trial_index >= plan.size()) {
    return "unknown";
  }
  return InputLabel(plan[global_trial_index].category, plan[global_trial_index].ordinal_within_category);
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

ExactRepeatMatch FindFirstExactWindowRepeat(
    const std::vector<std::uint8_t>& stream,
    std::size_t window_size) {
  ExactRepeatMatch result;
  if (window_size == 0U || stream.size() < window_size) {
    return result;
  }

  const std::size_t window_count = stream.size() - window_size + 1U;
  std::size_t capacity = 1U;
  while (capacity < ((window_count * 10U) / 7U) + 1U) {
    capacity <<= 1U;
  }
  const std::size_t mask = capacity - 1U;

  std::vector<std::uint64_t> hashes(capacity, 0ULL);
  std::vector<std::uint32_t> positions(capacity, std::numeric_limits<std::uint32_t>::max());

  RollingHash64 rolling(window_size);
  rolling.Initialize(stream.data());

  for (std::size_t position = 0; position < window_count; ++position) {
    const std::uint64_t hash = Mix64(rolling.hash ^ static_cast<std::uint64_t>(window_size));
    std::size_t slot = static_cast<std::size_t>(hash) & mask;
    while (true) {
      if (positions[slot] == std::numeric_limits<std::uint32_t>::max()) {
        hashes[slot] = hash;
        positions[slot] = static_cast<std::uint32_t>(position);
        break;
      }
      if (hashes[slot] == hash) {
        const std::size_t prior = positions[slot];
        if (std::memcmp(
                stream.data() + prior,
                stream.data() + position,
                window_size) == 0) {
          if (!result.found ||
              prior < result.first_position ||
              (prior == result.first_position && position < result.repeat_position)) {
            result.found = true;
            result.first_position = prior;
            result.repeat_position = position;
            result.match_length = window_size;
          }
          break;
        }
      }
      slot = (slot + 1U) & mask;
    }

    if (position + 1U < window_count) {
      rolling.Slide(stream[position], stream[position + window_size]);
    }
  }

  return result;
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

  for (const TrialSpec& trial : trial_plan) {
    const std::uint64_t trial_seed =
        ScenarioTrialSeed(options.seed, trial.category, trial.ordinal_within_category);

    std::vector<std::uint8_t> source(PASSWORD_EXPANDED_SIZE);
    FillSourceForTrial(source, trial.category, options.seed, trial.ordinal_within_category);
    const std::vector<std::uint8_t> initial_source = source;

    std::vector<std::uint8_t> worker(PASSWORD_EXPANDED_SIZE);
    std::vector<std::uint8_t> dest(PASSWORD_EXPANDED_SIZE);
    std::vector<std::vector<std::uint8_t>> base_blocks;
    base_blocks.reserve(options.avalanche_blocks);

    const std::size_t window_stride = std::max<std::size_t>(
        1U, options.stream_bytes / std::max<std::size_t>(1U, options.sample_windows));
    std::unordered_map<Window128, std::size_t, Window128Hash> sampled_windows;
    sampled_windows.reserve(options.sample_windows * 2U);

    std::array<std::uint8_t, 16> rolling_window{};
    std::size_t rolling_position = 0;
    std::size_t trial_processed_bytes = 0;
    std::optional<std::uint8_t> previous_byte;

    const std::size_t required_blocks =
        (options.stream_bytes + PASSWORD_EXPANDED_SIZE - 1U) / PASSWORD_EXPANDED_SIZE;
    const std::size_t total_blocks = std::max(required_blocks, options.cycle_block_count);
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> block_hashes;
    block_hashes.reserve(total_blocks * 2U);
    std::vector<std::vector<std::uint8_t>> seen_blocks;
    seen_blocks.reserve(total_blocks);

    for (std::size_t block_index = 0; block_index < total_blocks; ++block_index) {
      candidate.function(source.data(), worker.data(), dest.data());

      const std::uint64_t block_hash = HashBytes64(dest.data(), dest.size());
      if (trial.global_index == 0U && block_index == 0U) {
        result.first_block_hash = block_hash;
      }
      const auto block_match = block_hashes.find(block_hash);
      if (block_match != block_hashes.end() && !result.cycle_found) {
        for (const std::size_t prior_index : block_match->second) {
          if (seen_blocks[prior_index] == dest) {
            result.cycle_found = true;
            result.first_cycle_block = std::min(result.first_cycle_block, prior_index);
            result.cycle_length = result.cycle_length == 0U
                                      ? (block_index - prior_index)
                                      : std::min(result.cycle_length, block_index - prior_index);
            break;
          }
        }
      }
      seen_blocks.emplace_back(dest.begin(), dest.end());
      block_hashes[block_hash].push_back(block_index);

      if (base_blocks.size() < options.avalanche_blocks) {
        base_blocks.emplace_back(dest.begin(), dest.end());
      }

      const std::size_t bytes_to_scan =
          trial_processed_bytes < options.stream_bytes
              ? std::min<std::size_t>(dest.size(), options.stream_bytes - trial_processed_bytes)
              : 0U;

      for (std::size_t i = 0; i < bytes_to_scan; ++i) {
        const std::uint8_t byte = dest[i];
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

        processed_bytes += 1U;
        trial_processed_bytes += 1U;
      }

      std::copy(dest.begin(), dest.end(), source.begin());
    }

    if (!base_blocks.empty()) {
      std::vector<std::uint8_t> alt_source(PASSWORD_EXPANDED_SIZE);
      std::vector<std::uint8_t> alt_worker(PASSWORD_EXPANDED_SIZE);
      std::vector<std::uint8_t> alt_dest(PASSWORD_EXPANDED_SIZE);

      for (std::size_t avalanche_trial = 0; avalanche_trial < options.avalanche_trials; ++avalanche_trial) {
        alt_source = initial_source;
        const std::uint64_t flip_token =
            Mix64(trial_seed ^ (static_cast<std::uint64_t>(avalanche_trial) * 0xBF58476D1CE4E5B9ULL));
        const std::size_t flip_index =
            static_cast<std::size_t>(flip_token % PASSWORD_EXPANDED_SIZE);
        const std::uint8_t bit_mask =
            static_cast<std::uint8_t>(1U << ((flip_token >> 8U) & 7U));
        alt_source[flip_index] ^= bit_mask;

        for (std::size_t block_index = 0; block_index < base_blocks.size(); ++block_index) {
          candidate.function(alt_source.data(), alt_worker.data(), alt_dest.data());
          const std::vector<std::uint8_t>& baseline = base_blocks[block_index];
          for (std::size_t i = 0; i < baseline.size(); ++i) {
            const std::uint8_t diff = static_cast<std::uint8_t>(baseline[i] ^ alt_dest[i]);
            differing_bytes += static_cast<std::uint64_t>(diff != 0U);
            differing_bits += static_cast<std::uint64_t>(
                std::popcount(static_cast<unsigned int>(diff)));
          }
          compared_bytes += baseline.size();
          std::copy(alt_dest.begin(), alt_dest.end(), alt_source.begin());
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
  }

  const double score_avalanche_bits =
      Clamp(((result.avalanche_bit_ratio - 0.01) / 0.24) * 100.0, 0.0, 100.0);
  const double score_avalanche_bytes =
      Clamp(((result.avalanche_byte_ratio - 0.02) / 0.48) * 100.0, 0.0, 100.0);
  result.avalanche_score = (score_avalanche_bits * 0.60) + (score_avalanche_bytes * 0.40);

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
  const std::vector<TrialSpec> all_trials = BuildTrialPlan();
  CandidateResult overall = EvaluateCandidateForPlan(candidate, options, all_trials);

  for (const CategoryConfig& config : CategoryConfigs()) {
    if (config.trial_count == 0U) {
      continue;
    }
    std::vector<TrialSpec> category_trials;
    category_trials.reserve(config.trial_count);
    for (std::size_t i = 0; i < config.trial_count; ++i) {
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
  std::vector<std::uint8_t> worker(PASSWORD_EXPANDED_SIZE);
  std::vector<std::uint8_t> dest(PASSWORD_EXPANDED_SIZE);
  std::vector<std::uint8_t> stream(stream_bytes);
  FillSourceForTrial(source, trial.category, base_seed, trial.ordinal_within_category);

  std::size_t produced = 0;
  while (produced < stream_bytes) {
    candidate.function(source.data(), worker.data(), dest.data());
    const std::size_t bytes_to_copy =
        std::min<std::size_t>(dest.size(), stream_bytes - produced);
    std::memcpy(stream.data() + produced, dest.data(), bytes_to_copy);
    produced += bytes_to_copy;
    std::copy(dest.begin(), dest.end(), source.begin());
  }

  return stream;
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

    const std::vector<TrialSpec> plan = BuildTrialPlan();
    for (const TrialSpec& trial : plan) {
      const std::vector<std::uint8_t> stream = GenerateCandidateStream(
          candidate, trial, options.seed, options.long_repeat_scan_bytes);

      std::size_t min_match_bytes = 0U;
      if (options.long_repeat_window_a > 0U && options.long_repeat_window_b > 0U) {
        min_match_bytes = std::min(options.long_repeat_window_a, options.long_repeat_window_b);
      } else if (options.long_repeat_window_a > 0U) {
        min_match_bytes = options.long_repeat_window_a;
      } else {
        min_match_bytes = options.long_repeat_window_b;
      }

      const ExactRepeatMatch repeat_match =
          FindBestRepeatedMatchLZ(stream, min_match_bytes);
      if (repeat_match.found) {
        if (!result.exact_repeat_64_found && options.long_repeat_window_a > 0U &&
            repeat_match.match_length >= options.long_repeat_window_a) {
          result.exact_repeat_64_found = true;
          result.exact_repeat_64_position = repeat_match.first_position;
          result.exact_repeat_64_trial = trial.global_index;
        }
        if (!result.exact_repeat_128_found && options.long_repeat_window_b > 0U &&
            repeat_match.match_length >= options.long_repeat_window_b) {
          result.exact_repeat_128_found = true;
          result.exact_repeat_128_position = repeat_match.first_position;
          result.exact_repeat_128_trial = trial.global_index;
        }
      }

      if (result.exact_repeat_64_found && result.exact_repeat_128_found) {
        break;
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
    if (result.exact_repeat_64_found || result.exact_repeat_128_found) {
      std::size_t earliest_repeat = std::numeric_limits<std::size_t>::max();
      std::size_t window_size = 0U;
      if (result.exact_repeat_64_found) {
        earliest_repeat = result.exact_repeat_64_position;
        window_size = 64U;
      }
      if (result.exact_repeat_128_found &&
          result.exact_repeat_128_position < earliest_repeat) {
        earliest_repeat = result.exact_repeat_128_position;
        window_size = 128U;
      }
      const double repeat_ratio =
          static_cast<double>(earliest_repeat + window_size) /
          std::max<double>(1.0, static_cast<double>(options.long_repeat_scan_bytes));
      const double exact_repeat_score =
          Clamp(100.0 * std::sqrt(repeat_ratio), 0.0, 95.0);
      result.repeat_score = std::min(result.repeat_score, exact_repeat_score);
    }

    result.composite_score =
        (result.repeat_score * 0.22) + (result.cycle_score * 0.22) +
        (result.uniformity_score * 0.22) + (result.predictability_score * 0.16) +
        (result.avalanche_score * 0.12) + (result.distinctness_score * 0.06);

    double weighted_category_score = 0.0;
    std::size_t weighted_category_trials = 0;
    for (const CategoryConfig& config : CategoryConfigs()) {
      const std::size_t index = CategoryIndex(config.category);
      if (!result.category_present[index] || config.trial_count == 0U) {
        continue;
      }
      weighted_category_score += result.category_scores[index] * static_cast<double>(config.trial_count);
      weighted_category_trials += config.trial_count;
    }
    if (weighted_category_trials > 0U) {
      result.composite_score =
          weighted_category_score / static_cast<double>(weighted_category_trials);
    }

    const std::array<std::pair<const char*, double>, 6> components = {{
        {"repeat resistance", result.repeat_score},
        {"cycle resistance", result.cycle_score},
        {"uniformity", result.uniformity_score},
        {"predictability", result.predictability_score},
        {"avalanche", result.avalanche_score},
        {"distinctness", result.distinctness_score},
    }};

    auto weakest = std::min_element(
        components.begin(), components.end(),
        [](const auto& left, const auto& right) { return left.second < right.second; });
    result.failure_reason = weakest->first;
    if (weighted_category_trials > 0U) {
      double weakest_category_score = std::numeric_limits<double>::max();
      const char* weakest_category_name = weakest->first;
      for (const CategoryConfig& config : CategoryConfigs()) {
        const std::size_t index = CategoryIndex(config.category);
        if (!result.category_present[index]) {
          continue;
        }
        if (result.category_scores[index] < weakest_category_score) {
          weakest_category_score = result.category_scores[index];
          weakest_category_name = TrialCategoryName(config.category);
        }
      }
      result.failure_reason = weakest_category_name;
    }

    if (result.exact_repeat_64_found) {
      result.failure_reason = "exact 64-byte repeat";
    }
    if (result.exact_repeat_128_found) {
      result.failure_reason = "exact 128-byte repeat";
    }

    const bool catastrophic_repeat =
        (result.repeat_found && result.first_repeat_window < 65536U) ||
        result.exact_repeat_64_found || result.exact_repeat_128_found;
    const bool catastrophic_cycle =
        result.cycle_found && result.cycle_length > 0U && result.cycle_length < 4U;
    const bool catastrophic_uniformity =
        result.entropy < 7.0 || result.max_deviation > 0.35;

    result.rejected = catastrophic_repeat || catastrophic_cycle || catastrophic_uniformity;

    if (result.rejected) {
      result.composite_score = std::min(result.composite_score, 39.9);
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
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan();
  output
      << "input_mix,candidate_id,function_name,grade,grade_rank,composite_score,repeat_score,cycle_score,"
      << "uniformity_score,predictability_score,avalanche_score,distinctness_score,"
      << "entropy,reduced_chi_squared,max_deviation,byte_count_spread_ratio,"
      << "most_common_byte_count,least_common_byte_count,equal_rate_drift,correlation,"
      << "conditional_entropy,avalanche_byte_ratio,avalanche_bit_ratio,repeat_found,"
      << "first_repeat_window,cycle_found,first_cycle_block,cycle_length,long_repeat_verified,"
      << "exact_repeat_64_found,exact_repeat_64_position,exact_repeat_64_trial,"
      << "exact_repeat_128_found,exact_repeat_128_position,exact_repeat_128_trial,"
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
           << (result.repeat_found ? "true" : "false") << ','
           << (result.repeat_found ? std::to_string(result.first_repeat_window) : "") << ','
           << (result.cycle_found ? "true" : "false") << ','
           << (result.cycle_found ? std::to_string(result.first_cycle_block) : "") << ','
           << (result.cycle_found ? std::to_string(result.cycle_length) : "") << ','
           << (result.long_repeat_verified ? "true" : "false") << ','
           << (result.exact_repeat_64_found ? "true" : "false") << ','
           << (result.exact_repeat_64_found ? std::to_string(result.exact_repeat_64_position) : "") << ','
           << (result.exact_repeat_64_found ? std::to_string(result.exact_repeat_64_trial) : "") << ','
           << (result.exact_repeat_128_found ? "true" : "false") << ','
           << (result.exact_repeat_128_found ? std::to_string(result.exact_repeat_128_position) : "") << ','
           << (result.exact_repeat_128_found ? std::to_string(result.exact_repeat_128_trial) : "") << ','
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
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan();
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
  output << "trial_count=" << trial_plan.size() << '\n';
  output << "cycle_block_count=" << options.cycle_block_count << '\n';
  output << "long_repeat_scan_bytes=" << options.long_repeat_scan_bytes << '\n';
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
             << " distinctness=" << FormatDouble(result.distinctness_score)
             << " exact64=" << (result.exact_repeat_64_found ? "true" : "false")
             << " exact128=" << (result.exact_repeat_128_found ? "true" : "false")
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
      if (result.exact_repeat_64_found) {
        output << "   exact_repeat_64 position=" << result.exact_repeat_64_position
               << " input=" << LabelForGlobalTrialIndex(result.exact_repeat_64_trial) << '\n';
      }
      if (result.exact_repeat_128_found) {
        output << "   exact_repeat_128 position=" << result.exact_repeat_128_position
               << " input=" << LabelForGlobalTrialIndex(result.exact_repeat_128_trial) << '\n';
      }
    }
    if (wrote_header) {
      output << '\n';
    }
  }
}

void WriteHtmlReport(
    const std::filesystem::path& path,
    const std::vector<CandidateResult>& results,
    const Options& options) {
  std::ofstream output(path);
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan();
  std::size_t rejected = 0;
  for (const CandidateResult& result : results) {
    rejected += static_cast<std::size_t>(result.rejected);
  }

  std::ostringstream effective_config;
  effective_config
      << "input_mix = mixed\n"
      << "seed = " << options.seed << '\n'
      << "length_factor = " << options.length_factor << '\n'
      << "stream_bytes = " << options.stream_bytes
      << (options.stream_bytes_supplied ? " (override)" : " (derived from length_factor)") << '\n'
      << "trial_count = " << trial_plan.size() << '\n'
      << "cycle_block_count = " << options.cycle_block_count << '\n'
      << "sample_windows = " << options.sample_windows << '\n'
      << "signature_bytes = " << options.signature_bytes << '\n'
      << "avalanche_blocks = " << options.avalanche_blocks << '\n'
      << "avalanche_trials = " << options.avalanche_trials << '\n'
      << "long_repeat_scan_bytes = " << options.long_repeat_scan_bytes << '\n'
      << "long_repeat_top_count = " << options.long_repeat_top_count << '\n'
      << "long_repeat_window_a = " << options.long_repeat_window_a << '\n'
      << "long_repeat_window_b = " << options.long_repeat_window_b << '\n'
      << "top_n = " << options.top_n << '\n'
      << "candidate_filter = "
      << (options.candidate_id_supplied ? std::to_string(options.candidate_id) : "all") << '\n'
      << "limit = " << (options.limit > 0U ? std::to_string(options.limit) : "none") << '\n'
      << "source_patterns = " << SourcePatternList(trial_plan);

  std::ostringstream knob_config;
  knob_config
      << "kRandomizeSeedByDefault = "
      << (knobs::kRandomizeSeedByDefault ? "true" : "false") << '\n'
      << "kRandomSeed = "
      << (knobs::kRandomSeed == 0U ? std::string("runtime-random") : std::to_string(knobs::kRandomSeed)) << '\n'
      << "kCandidateCount = " << knobs::kCandidateCount << '\n'
      << "kTopCandidateCount = " << knobs::kTopCandidateCount << '\n'
      << "kPhase1MinOps = " << knobs::kPhase1MinOps << '\n'
      << "kPhase1MaxOps = " << knobs::kPhase1MaxOps << '\n'
      << "kPhase2MinOps = " << knobs::kPhase2MinOps << '\n'
      << "kPhase2MaxOps = " << knobs::kPhase2MaxOps << '\n'
      << "kMaxTransformsTotal = " << FormatLimit(knobs::kMaxTransformsTotal) << '\n'
      << "kMaxAddOps = " << FormatLimit(knobs::kMaxAddOps) << '\n'
      << "kMaxSubOps = " << FormatLimit(knobs::kMaxSubOps) << '\n'
      << "kMaxMulOps = " << FormatLimit(knobs::kMaxMulOps) << '\n'
      << "kMaxXorOps = " << FormatLimit(knobs::kMaxXorOps) << '\n'
      << "kMaxAndOps = " << FormatLimit(knobs::kMaxAndOps) << '\n'
      << "kMaxOrOps = " << FormatLimit(knobs::kMaxOrOps) << '\n'
      << "kMaxAddConstTransforms = " << FormatLimit(knobs::kMaxAddConstTransforms) << '\n'
      << "kMaxShiftLeftTransforms = " << FormatLimit(knobs::kMaxShiftLeftTransforms) << '\n'
      << "kMaxShiftRightTransforms = " << FormatLimit(knobs::kMaxShiftRightTransforms) << '\n'
      << "kMaxNotTransforms = " << FormatLimit(knobs::kMaxNotTransforms) << '\n'
      << "kMaxSwapNibblesTransforms = " << FormatLimit(knobs::kMaxSwapNibblesTransforms) << '\n'
      << "kMaxByteLR8LeftTransforms = " << FormatLimit(knobs::kMaxByteLR8LeftTransforms) << '\n'
      << "kMaxByteLR8RightTransforms = " << FormatLimit(knobs::kMaxByteLR8RightTransforms) << '\n'
      << "kLengthFactor = " << knobs::kLengthFactor << '\n'
      << "kTrialCountAES = " << knobs::kTrialCountAES << '\n'
      << "kTrialCountChaCha = " << knobs::kTrialCountChaCha << '\n'
      << "kTrialCountZeros = " << knobs::kTrialCountZeros << '\n'
      << "kTrialCountOnes = " << knobs::kTrialCountOnes << '\n'
      << "kTrialCountPredictableA = " << knobs::kTrialCountPredictableA << '\n'
      << "kTrialCountPredictableB = " << knobs::kTrialCountPredictableB << '\n'
      << "kTrialCountPredictableC = " << knobs::kTrialCountPredictableC << '\n'
      << "kDefaultCycleBlockCount = " << knobs::kDefaultCycleBlockCount << '\n'
      << "kDefaultSampleWindows = " << knobs::kDefaultSampleWindows << '\n'
      << "kDefaultSignatureBytes = " << knobs::kDefaultSignatureBytes << '\n'
      << "kDefaultAvalancheBlocks = " << knobs::kDefaultAvalancheBlocks << '\n'
      << "kDefaultAvalancheTrials = " << knobs::kDefaultAvalancheTrials << '\n'
      << "kLongRepeatScanBytes = " << knobs::kLongRepeatScanBytes << '\n'
      << "kLongRepeatTopCandidateCount = " << knobs::kLongRepeatTopCandidateCount << '\n'
      << "kLongRepeatWindowBytesA = " << knobs::kLongRepeatWindowBytesA << '\n'
      << "kLongRepeatWindowBytesB = " << knobs::kLongRepeatWindowBytesB;

  output << "<!doctype html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"utf-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
         << "  <title>Twist Candidate Report</title>\n"
         << "  <style>\n"
         << "    :root { color-scheme: light; }\n"
         << "    body { margin: 0; font: 14px/1.5 Menlo, Monaco, 'SFMono-Regular', monospace; background: #f4efe5; color: #1f1a17; }\n"
         << "    main { max-width: 1200px; margin: 0 auto; padding: 32px 24px 48px; }\n"
         << "    h1, h2 { margin: 0 0 12px; }\n"
         << "    h1 { font-size: 28px; }\n"
         << "    h2 { font-size: 18px; margin-top: 28px; }\n"
         << "    .meta { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin: 20px 0 28px; }\n"
         << "    .config-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(360px, 1fr)); gap: 12px; margin: 18px 0 28px; }\n"
         << "    .card { background: #fffaf0; border: 1px solid #d8c9b2; border-radius: 12px; padding: 12px 14px; }\n"
         << "    .label { color: #7b5a3a; font-size: 12px; text-transform: uppercase; letter-spacing: 0.08em; }\n"
         << "    .value { font-size: 20px; margin-top: 4px; }\n"
         << "    .method { background: #fffaf0; border-left: 4px solid #ba8f56; padding: 14px 16px; border-radius: 8px; }\n"
         << "    .config-card pre { margin: 10px 0 0; white-space: pre-wrap; word-break: break-word; }\n"
         << "    table { width: 100%; border-collapse: collapse; margin-top: 14px; background: #fffaf0; border: 1px solid #d8c9b2; }\n"
         << "    th, td { padding: 10px 8px; border-bottom: 1px solid #eadcc6; vertical-align: top; text-align: left; }\n"
         << "    th { position: sticky; top: 0; background: #f2e3cc; }\n"
         << "    tr:nth-child(even) td { background: #fffcf7; }\n"
         << "    .grade { font-weight: 700; }\n"
         << "    .recipe { max-width: 520px; white-space: normal; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "<main>\n"
         << "  <h1>Twist Candidate Report</h1>\n"
         << "  <p>Sorted by grade, then composite score.</p>\n"
         << "  <section class=\"meta\">\n"
         << "    <div class=\"card\"><div class=\"label\">Input Mix</div><div class=\"value\">mixed</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Seed</div><div class=\"value\">" << options.seed << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Candidates</div><div class=\"value\">" << results.size() << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Rejected</div><div class=\"value\">" << rejected << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Trials</div><div class=\"value\">" << trial_plan.size() << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Stream Bytes</div><div class=\"value\">" << options.stream_bytes << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Cycle Blocks</div><div class=\"value\">" << options.cycle_block_count << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Long Repeat Bytes</div><div class=\"value\">" << options.long_repeat_scan_bytes << "</div></div>\n"
         << "  </section>\n"
         << "  <section class=\"method\">\n"
         << "    <strong>Methodology.</strong> Each trial starts from the configured category mix shown below, then the harness measures how well each twister converts that input into high-quality output under repeated feedback. "
         << "Population scoring still uses sampled rolling 128-bit windows plus exact block-cycle detection. "
         << "After provisional ranking, the top candidates are re-checked with exact 64-byte and 128-byte window scans over the long-repeat stream length. "
         << "The sections below show the effective runtime settings and the compiled defaults from Knobs.hpp.\n"
         << "  </section>\n"
         << "  <section class=\"config-grid\">\n"
         << "    <div class=\"card config-card\">\n"
         << "      <div class=\"label\">Effective Run Config</div>\n"
         << "      <pre>" << EscapeHtml(effective_config.str()) << "</pre>\n"
         << "    </div>\n"
         << "    <div class=\"card config-card\">\n"
         << "      <div class=\"label\">Knobs.hpp Defaults</div>\n"
         << "      <pre>" << EscapeHtml(knob_config.str()) << "</pre>\n"
         << "    </div>\n"
         << "  </section>\n";

  const std::vector<std::string> grade_order = {
      "A+", "A", "A-", "B+", "B", "B-", "C+", "C", "C-", "D+", "D", "D-", "F"};

  for (const std::string& grade : grade_order) {
    bool wrote_header = false;
    output << "  <section>\n";
    for (const CandidateResult& result : results) {
      if (result.grade != grade) {
        continue;
      }
      if (!wrote_header) {
        output << "    <h2>" << grade << "</h2>\n"
               << "    <table>\n"
               << "      <thead><tr>"
               << "<th>Rank</th><th>Candidate</th><th>Composite</th><th>Repeat</th><th>Cycle</th>"
               << "<th>Uniformity</th><th>Spread</th><th>Most</th><th>Least</th><th>Gap</th>"
               << "<th>Predictability</th><th>Avalanche</th><th>Distinctness</th>"
               << "<th>Exact64</th><th>Exact128</th><th>Rejected</th><th>Failure</th><th>Recipe</th>"
               << "</tr></thead>\n"
               << "      <tbody>\n";
        wrote_header = true;
      }
    }

    if (!wrote_header) {
      output << "  </section>\n";
      continue;
    }

    std::size_t rank = 0;
    for (const CandidateResult& result : results) {
      if (result.grade != grade) {
        continue;
      }
      rank += 1U;
      output << "        <tr>"
             << "<td>" << rank << "</td>"
             << "<td class=\"grade\">" << result.candidate_id << " / "
             << EscapeHtml(result.function_name) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.composite_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.repeat_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.cycle_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.uniformity_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.byte_count_spread_ratio)) << "</td>"
             << "<td>" << result.most_common_byte_count << "</td>"
             << "<td>" << result.least_common_byte_count << "</td>"
             << "<td>" << (result.most_common_byte_count - result.least_common_byte_count) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.predictability_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.avalanche_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.distinctness_score)) << "</td>"
             << "<td>"
             << (result.exact_repeat_64_found
                     ? (std::string("repeat@") + std::to_string(result.exact_repeat_64_position) +
                        " / " + LabelForGlobalTrialIndex(result.exact_repeat_64_trial))
                     : (result.long_repeat_verified ? "clear" : "not-run"))
             << "</td>"
             << "<td>"
             << (result.exact_repeat_128_found
                     ? (std::string("repeat@") + std::to_string(result.exact_repeat_128_position) +
                        " / " + LabelForGlobalTrialIndex(result.exact_repeat_128_trial))
                     : (result.long_repeat_verified ? "clear" : "not-run"))
             << "</td>"
             << "<td>" << (result.rejected ? "true" : "false") << "</td>"
             << "<td>" << EscapeHtml(result.failure_reason) << "</td>"
             << "<td class=\"recipe\">" << EscapeHtml(result.recipe_summary) << "</td>"
             << "</tr>\n";
    }

    output << "      </tbody>\n"
           << "    </table>\n"
           << "  </section>\n";
  }

  output << "</main>\n"
         << "</body>\n"
         << "</html>\n";
}

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
    } else if (arg == "--cycle-block-count") {
      const auto value = require_value("--cycle-block-count");
      if (!value) {
        return std::nullopt;
      }
      options.cycle_block_count = static_cast<std::size_t>(std::stoull(*value));
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
    } else if (arg == "--long-repeat-window-a") {
      const auto value = require_value("--long-repeat-window-a");
      if (!value) {
        return std::nullopt;
      }
      options.long_repeat_window_a = static_cast<std::size_t>(std::stoull(*value));
    } else if (arg == "--long-repeat-window-b") {
      const auto value = require_value("--long-repeat-window-b");
      if (!value) {
        return std::nullopt;
      }
      options.long_repeat_window_b = static_cast<std::size_t>(std::stoull(*value));
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
          << "  --cycle-block-count <count>\n"
          << "  --sample-windows <count>\n"
          << "  --signature-bytes <bytes>\n"
          << "  --avalanche-blocks <count>\n"
          << "  --avalanche-trials <count>\n"
          << "  --long-repeat-bytes <bytes>\n"
          << "  --long-repeat-top <count>\n"
          << "  --long-repeat-window-a <bytes>\n"
          << "  --long-repeat-window-b <bytes>\n"
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
  for (const RegisteredCandidate* candidate : candidates) {
    results.push_back(EvaluateCandidate(*candidate, options));
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
              << " function=" << result.function_name
              << " rejected=" << (result.rejected ? "true" : "false") << '\n';
  }

  return 0;
}

}  // namespace twist

int main(int argc, char** argv) {
  return twist::Main(argc, argv);
}
