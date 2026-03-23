#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "PasswordExpander.hpp"
#include "TwistTypes.hpp"

extern "C" {
#include <TestU01.h>
#include <bbattery.h>
}

namespace {

using peanutbutter::expansion::key_expansion::PasswordExpander;
using twist::RegisteredCandidate;

struct Options {
  int candidate_index = 0;
  int candidate_id = -1;
  std::uint64_t seed = 0x9E3779B97F4A7C15ULL;
  std::string password_text = "cat";
  std::string battery = "BigCrush";
};

struct StreamState {
  const RegisteredCandidate* candidate = nullptr;
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> source{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> worker_a{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> worker_b{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> dest{};
  unsigned char salt[twist::kSaltBytes]{};
  unsigned char key_stack[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char mask_stack_a[twist::kMaskStackDepth][twist::kMaskBytes]{};
  unsigned char mask_stack_b[twist::kMaskStackDepth][twist::kMaskBytes]{};
  unsigned char next_round_key[twist::kRoundKeyBytes]{};
  unsigned char next_round_mask_a[twist::kMaskBytes]{};
  unsigned char next_round_mask_b[twist::kMaskBytes]{};
  unsigned int round = 0U;
  std::size_t offset = twist::PASSWORD_EXPANDED_SIZE;
};

[[noreturn]] void Usage(const char* argv0) {
  std::cerr << "usage: " << argv0
            << " [--candidate-index N | --candidate-id ID] [--seed N] [--password-text TEXT] [--battery SmallCrush|Crush|BigCrush]\n";
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
    } else if (arg == "--seed") {
      if (!ParseUnsigned(need_value("--seed"), options.seed)) {
        Usage(argv[0]);
      }
    } else if (arg == "--password-text") {
      options.password_text = need_value("--password-text");
    } else if (arg == "--battery") {
      options.battery = need_value("--battery");
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

void FillPasswordSource(const std::string& password_text, unsigned char* source) {
  PasswordExpander::FillDoubledSource(
      reinterpret_cast<const unsigned char*>(password_text.data()),
      static_cast<unsigned int>(password_text.size()),
      source);
}

void Refill(StreamState& state) {
  PasswordExpander::TwistRegisteredCandidateBlock(
      *state.candidate,
      state.source.data(),
      state.worker_a.data(),
      state.worker_b.data(),
      state.dest.data(),
      state.round,
      state.salt,
      state.key_stack,
      state.mask_stack_a,
      state.mask_stack_b,
      static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
  PasswordExpander::PushRegisteredCandidateRound(
      *state.candidate,
      state.dest.data(),
      state.salt,
      state.key_stack,
      state.mask_stack_a,
      state.mask_stack_b,
      state.next_round_key,
      state.next_round_mask_a,
      state.next_round_mask_b,
      static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
  std::copy(state.dest.begin(), state.dest.end(), state.source.begin());
  state.offset = 0U;
  ++state.round;
}

unsigned long GetBits(void* param, void* state_ptr) {
  (void)param;
  auto* state = static_cast<StreamState*>(state_ptr);
  unsigned long value = 0UL;
  for (int i = 0; i < 4; ++i) {
    if (state->offset >= twist::PASSWORD_EXPANDED_SIZE) {
      Refill(*state);
    }
    value = (value << 8U) | static_cast<unsigned long>(state->dest[state->offset++]);
  }
  return value;
}

}  // namespace

int main(int argc, char** argv) {
  const Options options = ParseArgs(argc, argv);
  const RegisteredCandidate& candidate = ResolveCandidate(options);

  StreamState state;
  state.candidate = &candidate;
  FillPasswordSource(options.password_text, state.source.data());
  PasswordExpander::SeedRegisteredCandidate(
      candidate,
      state.source.data(),
      state.worker_a.data(),
      state.worker_b.data(),
      state.salt,
      state.key_stack,
      state.mask_stack_a,
      state.mask_stack_b,
      static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
  Refill(state);

  unif01_Gen* gen = unif01_CreateExternGenBits(
      const_cast<char*>(candidate.function_name),
      GetBits);
  gen->param = nullptr;
  gen->state = &state;

  if (options.battery == "SmallCrush") {
    bbattery_SmallCrush(gen);
  } else if (options.battery == "Crush") {
    bbattery_Crush(gen);
  } else {
    bbattery_BigCrush(gen);
  }

  unif01_DeleteExternGenBits(gen);
  return 0;
}
