#pragma once

#include <arm_neon.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace twist {

inline constexpr std::size_t PASSWORD_EXPANDED_SIZE = 7680;
inline constexpr std::size_t kNeonWidth = 16;

using TwistFunction = void (*)(
    const uint8_t source[PASSWORD_EXPANDED_SIZE],
    uint8_t worker[PASSWORD_EXPANDED_SIZE],
    uint8_t dest[PASSWORD_EXPANDED_SIZE]);

using ByteVec = uint8x16_t;

inline ByteVec LoadVecWrapped(
    const std::uint8_t data[PASSWORD_EXPANDED_SIZE],
    std::size_t index) {
  index %= PASSWORD_EXPANDED_SIZE;
  if (index + kNeonWidth <= PASSWORD_EXPANDED_SIZE) {
    return vld1q_u8(data + index);
  }

  alignas(16) std::array<std::uint8_t, kNeonWidth> bytes{};
  for (std::size_t lane = 0; lane < bytes.size(); ++lane) {
    bytes[lane] = data[(index + lane) % PASSWORD_EXPANDED_SIZE];
  }
  return vld1q_u8(bytes.data());
}

inline void StoreVecContiguous(
    std::uint8_t data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    ByteVec value) {
  vst1q_u8(data + index, value);
}

inline void StoreVecPartial(
    std::uint8_t data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    ByteVec value,
    std::size_t bytes_to_write) {
  alignas(16) std::array<std::uint8_t, kNeonWidth> bytes{};
  vst1q_u8(bytes.data(), value);
  for (std::size_t lane = 0; lane < bytes_to_write; ++lane) {
    data[index + lane] = bytes[lane];
  }
}

inline std::array<std::uint8_t, kNeonWidth> LoadBlock16Wrapped(
    const std::uint8_t data[PASSWORD_EXPANDED_SIZE],
    std::size_t index) {
  std::array<std::uint8_t, kNeonWidth> bytes{};
  index %= PASSWORD_EXPANDED_SIZE;
  for (std::size_t lane = 0; lane < bytes.size(); ++lane) {
    bytes[lane] = data[(index + lane) % PASSWORD_EXPANDED_SIZE];
  }
  return bytes;
}

inline void StoreBlock16Contiguous(
    std::uint8_t data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    const std::array<std::uint8_t, kNeonWidth>& bytes) {
  std::memcpy(data + index, bytes.data(), bytes.size());
}

inline ByteVec BytewiseAdd(ByteVec left, ByteVec right) {
  return vaddq_u8(left, right);
}

inline ByteVec BytewiseSub(ByteVec left, ByteVec right) {
  return vsubq_u8(left, right);
}

inline ByteVec BytewiseMul(ByteVec left, ByteVec right) {
  return vmulq_u8(left, right);
}

inline ByteVec BytewiseXor(ByteVec left, ByteVec right) {
  return veorq_u8(left, right);
}

inline ByteVec BytewiseOr(ByteVec left, ByteVec right) {
  return vorrq_u8(left, right);
}

inline ByteVec BytewiseAnd(ByteVec left, ByteVec right) {
  return vandq_u8(left, right);
}

inline ByteVec BytewiseAddConstant(ByteVec word, unsigned constant) {
  return vaddq_u8(word, vdupq_n_u8(static_cast<std::uint8_t>(constant & 0xFFU)));
}

inline ByteVec BytewiseShiftLeft(ByteVec word, unsigned shift) {
  const int8x16_t counts = vdupq_n_s8(static_cast<std::int8_t>(shift));
  return vshlq_u8(word, counts);
}

inline ByteVec BytewiseShiftRight(ByteVec word, unsigned shift) {
  const int8x16_t counts = vdupq_n_s8(static_cast<std::int8_t>(-static_cast<int>(shift)));
  return vshlq_u8(word, counts);
}

inline ByteVec BytewiseNot(ByteVec word) {
  return vmvnq_u8(word);
}

inline ByteVec SwapNibbles(ByteVec word) {
  const ByteVec low_mask = vdupq_n_u8(0x0FU);
  const ByteVec high_mask = vdupq_n_u8(0xF0U);
  const ByteVec lo = vandq_u8(word, low_mask);
  const ByteVec hi = vandq_u8(word, high_mask);
  return vorrq_u8(vshlq_n_u8(lo, 4), vshrq_n_u8(hi, 4));
}

inline ByteVec ByteLR8Left(ByteVec word, unsigned count) {
  static constexpr std::uint8_t kIndices[8][16] = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8},
      {2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9},
      {3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10},
      {4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11},
      {5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12},
      {6, 7, 0, 1, 2, 3, 4, 5, 14, 15, 8, 9, 10, 11, 12, 13},
      {7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14},
  };
  return vqtbl1q_u8(word, vld1q_u8(kIndices[count & 7U]));
}

inline ByteVec ByteLR8Right(ByteVec word, unsigned count) {
  static constexpr std::uint8_t kIndices[8][16] = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14},
      {6, 7, 0, 1, 2, 3, 4, 5, 14, 15, 8, 9, 10, 11, 12, 13},
      {5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12},
      {4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11},
      {3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10},
      {2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9},
      {1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8},
  };
  return vqtbl1q_u8(word, vld1q_u8(kIndices[count & 7U]));
}

struct PhaseRecipe {
  std::array<int, 3> offsets;
  const char* op1;
  const char* op2;
  const char* op3;
  const char* e_input;
  const char* f_input;
  const char* e_transform;
  int e_transform_arg;
  const char* f_transform;
  int f_transform_arg;
};

struct RegisteredCandidate {
  int candidate_id;
  const char* function_name;
  int op_budget;
  int multiply_count;
  int subop_count;
  PhaseRecipe phase1;
  PhaseRecipe phase2;
  const char* recipe_summary;
  TwistFunction function;
};

extern const RegisteredCandidate kRegisteredCandidates[];
extern const std::size_t kRegisteredCandidateCount;
extern const RegisteredCandidate kBaselineCandidates[];
extern const std::size_t kBaselineCandidateCount;

}  // namespace twist
