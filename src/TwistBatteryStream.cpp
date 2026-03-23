#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

#include "TOP64.cpp"

namespace {

constexpr std::uint64_t kDefaultSeed = 0x9E3779B97F4A7C15ULL;

struct Options {
  int candidate_index = 0;
  int candidate_id = -1;
  std::uint64_t byte_count = 0;
  std::uint64_t seed = kDefaultSeed;
  std::string password_text;
  bool output_bits = false;
  std::string output_path;
};

[[noreturn]] void Usage(const char* argv0) {
  std::cerr
      << "usage: " << argv0 << " [--candidate-index N | --candidate-id ID] --bytes COUNT [--seed N | --password-text TEXT] [--bits] [--output PATH]\n"
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
    } else {
      Usage(argv[0]);
    }
  }
  return options;
}

const twist::ExportedCandidate& ResolveCandidate(const Options& options) {
  if (options.candidate_id >= 0) {
    for (const auto& candidate : twist::kExportedTopCandidates) {
      if (candidate.candidate_id == options.candidate_id) {
        return candidate;
      }
    }
    std::cerr << "candidate_id not found: " << options.candidate_id << "\n";
    std::exit(1);
  }

  if (options.candidate_index < 0 ||
      static_cast<std::size_t>(options.candidate_index) >= twist::kExportedTopCandidateCount) {
    std::cerr << "candidate_index out of range: " << options.candidate_index << "\n";
    std::exit(1);
  }
  return twist::kExportedTopCandidates[static_cast<std::size_t>(options.candidate_index)];
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

void FillRepeatedPasswordSource(const std::string& password_text, unsigned char* source) {
  if (source == nullptr) {
    return;
  }
  if (password_text.empty()) {
    std::memset(source, 0, twist::PASSWORD_EXPANDED_SIZE);
    return;
  }

  const std::size_t password_length = password_text.size();
  const std::size_t initial_copy =
      std::min<std::size_t>(password_length, twist::PASSWORD_EXPANDED_SIZE);
  std::memcpy(source, password_text.data(), initial_copy);
  std::size_t filled = initial_copy;
  while (filled < twist::PASSWORD_EXPANDED_SIZE) {
    const std::size_t remaining = twist::PASSWORD_EXPANDED_SIZE - filled;
    const std::size_t chunk = std::min<std::size_t>(remaining, filled);
    std::memcpy(source + filled, source, chunk);
    filled += chunk;
  }
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
  const twist::ExportedCandidate& candidate = ResolveCandidate(options);

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
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> worker{};
  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> dest{};
  unsigned char key_stack[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char next_round_key[twist::kRoundKeyBytes]{};
  unsigned char salt[twist::kSaltBytes]{};

  if (!options.password_text.empty()) {
    FillRepeatedPasswordSource(options.password_text, source.data());
  } else {
    FillInitialSource(options.seed, source.data());
  }
  candidate.key_seed(source.data(), key_stack, static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
  candidate.salt_seed(source.data(), salt, static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));

  const bool stream_forever = (options.byte_count == 0U);
  std::uint64_t remaining = options.byte_count;
  unsigned int round = 0U;
  while (stream_forever || remaining > 0U) {
    candidate.twist_block(
        source.data(),
        worker.data(),
        dest.data(),
        round,
        salt,
        key_stack,
        static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));
    candidate.push_key_round(
        dest.data(),
        salt,
        key_stack,
        next_round_key,
        static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE));

    const std::size_t emit_count = static_cast<std::size_t>(
        stream_forever
            ? static_cast<std::uint64_t>(twist::PASSWORD_EXPANDED_SIZE)
            : std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(twist::PASSWORD_EXPANDED_SIZE)));
    if (options.output_bits) {
      WriteBits(*output, dest.data(), emit_count);
    } else {
      WriteBinary(*output, dest.data(), emit_count);
    }

    std::memcpy(source.data(), dest.data(), twist::PASSWORD_EXPANDED_SIZE);
    if (!stream_forever) {
      remaining -= static_cast<std::uint64_t>(emit_count);
    }
    ++round;
  }

  output->flush();
  return output->good() ? 0 : 1;
}
