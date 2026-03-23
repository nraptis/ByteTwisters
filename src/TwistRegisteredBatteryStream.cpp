#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

#include "PasswordExpander.hpp"
#include "TwistTypes.hpp"

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;
using twist::RegisteredCandidate;

constexpr std::uint64_t kDefaultSeed = 0x9E3779B97F4A7C15ULL;

struct Options {
  int candidate_index = 0;
  int candidate_id = -1;
  std::uint64_t byte_count = 0;
  std::uint64_t seed = kDefaultSeed;
  std::string password_text;
  bool output_bits = false;
  bool count_only = false;
  std::string output_path;
};

[[noreturn]] void Usage(const char* argv0) {
  std::cerr
      << "usage: " << argv0
      << " [--candidate-index N | --candidate-id ID] [--bytes COUNT] [--seed N | --password-text TEXT] [--bits] [--output PATH] [--count-only]\n"
      << "  bytes=0 means stream forever\n";
  std::exit(1);
}

bool ParseUnsigned(const char* text, std::uint64_t& value) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const unsigned long long parsed = std::strtoull(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0') {
    return false;
  }
  value = static_cast<std::uint64_t>(parsed);
  return true;
}

Options ParseArgs(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string arg(argv[index]);
    auto need_value = [&](const char* flag) -> const char* {
      if (index + 1 >= argc) {
        std::cerr << "missing value for " << flag << "\n";
        Usage(argv[0]);
      }
      return argv[++index];
    };

    if (arg == "--candidate-index") {
      std::uint64_t value = 0;
      if (!ParseUnsigned(need_value("--candidate-index"), value)) {
        Usage(argv[0]);
      }
      options.candidate_index = static_cast<int>(value);
    } else if (arg == "--candidate-id") {
      std::uint64_t value = 0;
      if (!ParseUnsigned(need_value("--candidate-id"), value)) {
        Usage(argv[0]);
      }
      options.candidate_id = static_cast<int>(value);
    } else if (arg == "--bytes") {
      if (!ParseUnsigned(need_value("--bytes"), options.byte_count)) {
        Usage(argv[0]);
      }
    } else if (arg == "--seed") {
      if (!ParseUnsigned(need_value("--seed"), options.seed)) {
        Usage(argv[0]);
      }
    } else if (arg == "--password-text") {
      options.password_text = need_value("--password-text");
    } else if (arg == "--bits") {
      options.output_bits = true;
    } else if (arg == "--output") {
      options.output_path = need_value("--output");
    } else if (arg == "--count-only") {
      options.count_only = true;
    } else {
      Usage(argv[0]);
    }
  }
  return options;
}

const RegisteredCandidate& ResolveCandidate(const Options& options) {
  if (options.candidate_id >= 0) {
    for (std::size_t i = 0; i < twist::kRegisteredCandidateCount; ++i) {
      if (twist::kRegisteredCandidates[i].candidate_id == options.candidate_id) {
        return twist::kRegisteredCandidates[i];
      }
    }
    std::cerr << "candidate_id not found: " << options.candidate_id << "\n";
    std::exit(1);
  }

  if (options.candidate_index < 0 ||
      static_cast<std::size_t>(options.candidate_index) >= twist::kRegisteredCandidateCount) {
    std::cerr << "candidate_index out of range: " << options.candidate_index << "\n";
    std::exit(1);
  }
  return twist::kRegisteredCandidates[static_cast<std::size_t>(options.candidate_index)];
}

void FillInitialSource(std::uint64_t seed, unsigned char* source) {
  std::uint64_t state = seed;
  for (std::size_t index = 0; index < twist::PASSWORD_EXPANDED_SIZE; ++index) {
    state ^= state >> 12U;
    state ^= state << 25U;
    state ^= state >> 27U;
    state *= 2685821657736338717ULL;
    source[index] = static_cast<unsigned char>(state & 0xFFU);
  }
}

void FillPasswordSource(const std::string& password_text, unsigned char* source) {
  PasswordExpander::FillDoubledSource(
      reinterpret_cast<const unsigned char*>(password_text.data()),
      static_cast<unsigned int>(password_text.size()),
      source);
}

void WriteBinary(std::ostream& output, const unsigned char* bytes, std::size_t count) {
  output.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(count));
}

void WriteBits(std::ostream& output, const unsigned char* bytes, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index) {
    unsigned char value = bytes[index];
    for (int bit = 7; bit >= 0; --bit) {
      const char digit = ((value >> bit) & 1U) != 0U ? '1' : '0';
      output.put(digit);
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  const Options options = ParseArgs(argc, argv);
  if (options.count_only) {
    std::cout << twist::kRegisteredCandidateCount << '\n';
    return 0;
  }

  const RegisteredCandidate& candidate = ResolveCandidate(options);

  std::ostream* output = &std::cout;
  std::unique_ptr<std::ofstream> file_output;
  if (!options.output_path.empty()) {
    file_output = std::make_unique<std::ofstream>(
        options.output_path,
        options.output_bits ? std::ios::out : (std::ios::out | std::ios::binary));
    if (!file_output->is_open()) {
      std::cerr << "failed to open output: " << options.output_path << "\n";
      return 1;
    }
    output = file_output.get();
  } else if (!options.output_bits) {
    std::ios::sync_with_stdio(false);
  }

  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> source{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> worker_a{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> worker_b{};
  unsigned char salt[twist::kSaltBytes]{};
  unsigned char key_stack[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char mask_stack_a[twist::kMaskStackDepth][twist::kMaskBytes]{};
  unsigned char mask_stack_b[twist::kMaskStackDepth][twist::kMaskBytes]{};
  unsigned char next_round_key[twist::kRoundKeyBytes]{};
  unsigned char next_round_mask_a[twist::kMaskBytes]{};
  unsigned char next_round_mask_b[twist::kMaskBytes]{};

  if (!options.password_text.empty()) {
    FillPasswordSource(options.password_text, source.data());
  } else {
    FillInitialSource(options.seed, source.data());
  }

  PasswordExpander::SeedRegisteredCandidate(
      candidate,
      source.data(),
      worker_a.data(),
      worker_b.data(),
      salt,
      key_stack,
      mask_stack_a,
      mask_stack_b,
      static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));

  const bool stream_forever = (options.byte_count == 0U);
  std::uint64_t remaining = options.byte_count;
  unsigned int round = 0U;
  while (stream_forever || remaining > 0U) {
    const std::size_t chunk_bytes = static_cast<std::size_t>(
        stream_forever
            ? static_cast<std::uint64_t>(twist::PASSWORD_EXPANDED_SIZE)
            : std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(twist::PASSWORD_EXPANDED_SIZE)));

    std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> dest{};
    PasswordExpander::TwistRegisteredCandidateBlock(
        candidate,
        source.data(),
        worker_a.data(),
        worker_b.data(),
        dest.data(),
        round,
        salt,
        key_stack,
        mask_stack_a,
        mask_stack_b,
        static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
    PasswordExpander::PushRegisteredCandidateRound(
        candidate,
        dest.data(),
        salt,
        key_stack,
        mask_stack_a,
        mask_stack_b,
        next_round_key,
        next_round_mask_a,
        next_round_mask_b,
        static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));

    if (options.output_bits) {
      WriteBits(*output, dest.data(), chunk_bytes);
    } else {
      WriteBinary(*output, dest.data(), chunk_bytes);
    }

    std::copy(dest.begin(), dest.end(), source.begin());
    if (!stream_forever) {
      remaining -= static_cast<std::uint64_t>(chunk_bytes);
    }
    ++round;
  }

  output->flush();
  return output->good() ? 0 : 1;
}
