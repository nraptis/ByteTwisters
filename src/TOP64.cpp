#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace twist {

inline constexpr std::size_t PASSWORD_EXPANDED_SIZE = 7680;
inline constexpr std::size_t kMatrixBlockBytes = 16;
inline constexpr std::size_t kRoundKeyBytes = 32;
inline constexpr std::size_t kRoundKeyStackDepth = 16;
inline constexpr std::size_t kSaltBytes = 32;

using TwistFunction = void (*)(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength);
using KeySeedFunction = void (*)(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength);
using SaltSeedFunction = void (*)(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength);
using TwistBlockFunction = void (*)(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength);
using PushKeyRoundFunction = void (*)(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength);

inline std::uint32_t RotateLeft32(std::uint32_t value, unsigned int amount) {
  const unsigned int shift = amount & 31U;
  if (shift == 0U) {
    return value;
  }
  return static_cast<std::uint32_t>((value << shift) | (value >> (32U - shift)));
}

inline unsigned char KeyStackByte(
    const unsigned char pKeyStack[kRoundKeyStackDepth][kRoundKeyBytes],
    std::size_t row,
    std::size_t column) {
  if (pKeyStack == nullptr) {
    return 0U;
  }
  return pKeyStack[row % kRoundKeyStackDepth][column % kRoundKeyBytes];
}

inline void RotateKeyStack(
    unsigned char pKeyStack[kRoundKeyStackDepth][kRoundKeyBytes],
    const unsigned char next_round_key[kRoundKeyBytes]) {
  if (pKeyStack == nullptr || next_round_key == nullptr) {
    return;
  }
  for (std::size_t row = 0; row + 1U < kRoundKeyStackDepth; ++row) {
    std::memcpy(pKeyStack[row], pKeyStack[row + 1U], kRoundKeyBytes);
  }
  std::memcpy(pKeyStack[kRoundKeyStackDepth - 1U], next_round_key, kRoundKeyBytes);
}

inline int WrapRange(int index, int start, int end) {
  const int length = end - start;
  if (length <= 0) {
    return start;
  }
  int offset = (index - start) % length;
  if (offset < 0) {
    offset += length;
  }
  return start + offset;
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

struct ExportedCandidate {
  int candidate_id;
  const char* function_name;
  double rank_score;
  double avalanche_score;
  double bic_score;
  double input_mean_score;
  double input_floor_score;
  double input_tail_score;
  double composite_score;
  TwistFunction function;
  KeySeedFunction key_seed;
  SaltSeedFunction salt_seed;
  TwistBlockFunction twist_block;
  PushKeyRoundFunction push_key_round;
};

// Top candidates exported from sharded measurement
// rank_mode=balanced-shortlist
// rank_score = percentile blend of avalanche, BIC, input mean, input floor, input lower-tail, and completeness

// rank=95.212 avalanche=99.267 bic=12.730 input_mean=85.505 input_floor=73.389 input_tail=73.522 composite=57.161 candidate_id=962
// Candidate 962: TwistCandidate_0962
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x3/3x3
// mechanical_loops[l1=2x0; l2=2x3; l3=3x3; key_rot=9; twiddle=0x9861837f]
static void TwistCandidate_0962_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (4746)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (6281)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 1u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0962_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (3531)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (5222)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 17u + static_cast<unsigned int>(1u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 119u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0962_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x9861837Fu ^ (pRound * 15u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4746), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8B1D5D9Bu ^ static_cast<std::uint32_t>(13094u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5673), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4284), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 22u + ((key_byte + 9u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 155u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 49u, 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x03591C01u ^ static_cast<std::uint32_t>(4137u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2443), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-990) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 4u) & 0xFFu) ^ (salt_byte ^ 25u) ^ ((feedback_byte + 21u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 21u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 31u, 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 231u, 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x15A32581u ^ static_cast<std::uint32_t>(628u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1307), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2313), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1634), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 4u) & 0xFFu) ^ (c) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 22u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 205u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 25u), 4u);
    }
  }

}

static void TwistCandidate_0962_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (3956)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (6000)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-2905)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 24u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 119u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0962(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0962_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0962_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0962_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0962_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=94.943 avalanche=99.294 bic=12.631 input_mean=81.591 input_floor=73.411 input_tail=73.554 composite=56.231 candidate_id=353
// Candidate 353: TwistCandidate_0353
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x1/3x0
// mechanical_loops[l1=2x3; l2=2x1; l3=3x0; key_rot=31; twiddle=0x52501d26]
static void TwistCandidate_0353_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (2862)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-4341)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 225u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0353_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (2813)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (6742)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 31u + static_cast<unsigned int>(225u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 143u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0353_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x52501D26u ^ (pRound * 54u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2862), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0228F252u ^ static_cast<std::uint32_t>(5005u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6036), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1650), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 23u + ((key_byte + 3u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 30u) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 5u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 135u), 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 245u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9AE22170u ^ static_cast<std::uint32_t>(1740u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2674), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5032) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 6u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 20u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 144u, 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 139u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8309C82Au ^ static_cast<std::uint32_t>(5505u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2768), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5951), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3214), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 17u + (salt_byte)) ^ ((b << 4u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 6u) & 0xFFu) ^ ((feedback_byte + 27u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 17u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 27u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 138u), 2u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 53u), 5u);
    }
  }

}

static void TwistCandidate_0353_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (5944)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-4654)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-5692)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 29u + static_cast<unsigned int>(31u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 143u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0353(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0353_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0353_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0353_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0353_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=92.496 avalanche=99.236 bic=11.110 input_mean=85.310 input_floor=73.449 input_tail=73.460 composite=56.727 candidate_id=750
// Candidate 750: TwistCandidate_0750
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x1/3x2
// mechanical_loops[l1=2x2; l2=2x1; l3=3x2; key_rot=9; twiddle=0x328711ff]
static void TwistCandidate_0750_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (1602)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-5889)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 249u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0750_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-1891)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-3615)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 2u + static_cast<unsigned int>(249u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 38u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0750_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x328711FFu ^ (pRound * 33u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1602), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x612C721Au ^ static_cast<std::uint32_t>(12680u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (339), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-7322), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 25u + (key_byte)) ^ (b) ^ (salt_byte ^ 2u) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 18u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 173u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 228u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x739FED31u ^ static_cast<std::uint32_t>(2392u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4917), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2040) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 18u) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 60u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 68u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC81CFBCAu ^ static_cast<std::uint32_t>(12681u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7505), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3475), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1701), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ ((c >> 1u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 9u ^ (key_byte)) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 113u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 222u, 2u);
    }
  }

}

static void TwistCandidate_0750_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-4911)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (1882)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 22u + (6454)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 8u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 38u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0750(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0750_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0750_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0750_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0750_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=90.777 avalanche=99.153 bic=11.058 input_mean=81.591 input_floor=73.469 input_tail=73.474 composite=56.139 candidate_id=271
// Candidate 271: TwistCandidate_0271
// family=mechanical_loops op_budget=3 loop_shapes=3x0/3x1/3x0
// mechanical_loops[l1=3x0; l2=3x1; l3=3x0; key_rot=21; twiddle=0xa9b31e44]
static void TwistCandidate_0271_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (1297)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (2713)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 173u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0271_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (5491)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (3442)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(173u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 224u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0271_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA9B31E44u ^ (pRound * 55u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1297), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDF801CF0u ^ static_cast<std::uint32_t>(7100u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4744), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2813), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-457), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 20u + (salt_byte ^ 31u)) ^ ((b << 3u) & 0xFFu) ^ (c) ^ ((key_byte + 9u) & 0xFFu) ^ ((feedback_byte + 31u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 31u) ^ (((feedback_byte + 31u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 31u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 113u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 38u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB2B5E964u ^ static_cast<std::uint32_t>(8762u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7574), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (249), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1437), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte ^ 15u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c + 11u + (twiddle_byte)) ^ ((a >> 3u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 15u) ^ ((feedback_byte + 23u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 173u), 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 245u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCC121A50u ^ static_cast<std::uint32_t>(12204u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7290), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-335), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4579), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 9u + (salt_byte ^ 6u)) ^ (b) ^ (c) ^ (key_byte) ^ ((feedback_byte + 8u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 22u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 6u) ^ (((feedback_byte + 8u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 13u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 185u), 10u);
    }
  }

}

static void TwistCandidate_0271_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (3837)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-4746)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-6451)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 2u + static_cast<unsigned int>(21u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 2u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 224u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0271(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0271_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0271_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0271_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0271_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=90.520 avalanche=99.348 bic=12.246 input_mean=77.770 input_floor=73.409 input_tail=73.421 composite=53.509 candidate_id=810
// Candidate 810: TwistCandidate_0810
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x3/3x2
// mechanical_loops[l1=2x3; l2=2x3; l3=3x2; key_rot=30; twiddle=0xf9777f05]
static void TwistCandidate_0810_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (5957)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (443)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 175u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0810_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (2945)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (5565)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 11u + static_cast<unsigned int>(175u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 14u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0810_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF9777F05u ^ (pRound * 28u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5957), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x51493E34u ^ static_cast<std::uint32_t>(7634u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2751), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4736), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 4u) & 0xFFu) ^ (salt_byte ^ 29u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 11u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 205u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 11u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3F584C8Fu ^ static_cast<std::uint32_t>(8557u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5737), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4397) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 8u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 250u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 101u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x43F443F3u ^ static_cast<std::uint32_t>(5030u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1690), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (222), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (6498), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b + 3u + (salt_byte)) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 14u) & 0xFFu) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 243u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 240u, 10u);
    }
  }

}

static void TwistCandidate_0810_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-3128)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-2138)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-791)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 20u + static_cast<unsigned int>(30u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 14u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0810(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0810_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0810_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0810_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0810_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.887 avalanche=98.954 bic=12.936 input_mean=81.592 input_floor=73.435 input_tail=73.740 composite=54.544 candidate_id=1236
// Candidate 1236: TwistCandidate_1236
// family=mechanical_loops op_budget=3 loop_shapes=1x1/3x0/3x3
// mechanical_loops[l1=1x1; l2=3x0; l3=3x3; key_rot=29; twiddle=0x7e0e96d1]
static void TwistCandidate_1236_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (5929)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-2627)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 74u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1236_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-6104)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-5131)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 30u + static_cast<unsigned int>(74u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 129u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1236_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x7E0E96D1u ^ (pRound * 43u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5929), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1DB8EE73u ^ static_cast<std::uint32_t>(17790u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7528), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 14u + (key_byte)) ^ ((a << 1u) & 0xFFu) ^ (salt_byte ^ 18u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + 1u) ^ ((a >> 1u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 240u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 72u, 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6ACAFE75u ^ static_cast<std::uint32_t>(6832u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1981), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3700) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (1151) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ ((b << 2u) & 0xFFu) ^ (c) ^ ((key_byte + 30u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 20u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 239u), 2u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 26u, 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDAFCC784u ^ static_cast<std::uint32_t>(10836u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5456), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6483), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1103), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c + 21u + (salt_byte ^ 31u)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 3u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 254u, 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 142u), 7u);
    }
  }

}

static void TwistCandidate_1236_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (6643)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (825)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 1u + (6825)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 11u + static_cast<unsigned int>(29u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(11u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 129u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1236(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1236_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1236_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1236_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1236_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.689 avalanche=99.382 bic=10.635 input_mean=81.439 input_floor=73.489 input_tail=73.490 composite=56.193 candidate_id=1470
// Candidate 1470: TwistCandidate_1470
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x1/3x0
// mechanical_loops[l1=2x2; l2=2x1; l3=3x0; key_rot=20; twiddle=0xad434d70]
static void TwistCandidate_1470_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-4675)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (5121)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 52u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1470_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-3839)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-3200)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 16u + static_cast<unsigned int>(52u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 207u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1470_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xAD434D70u ^ (pRound * 36u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-4675), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xFBAE4A5Bu ^ static_cast<std::uint32_t>(2676u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6983), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-7469), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 9u + (key_byte)) ^ (b) ^ (salt_byte ^ 12u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 18u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 51u), 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 23u, 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x61720809u ^ static_cast<std::uint32_t>(9074u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2637), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 18u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1254), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 5u + (key_byte)) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 14u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 5u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 51u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA17D3A4Du ^ static_cast<std::uint32_t>(12083u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6718), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1861), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (3504), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 22u + (salt_byte ^ 13u)) ^ ((b << 4u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 28u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 11u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 13u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 128u), 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 202u), 13u);
    }
  }

}

static void TwistCandidate_1470_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (6773)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (2888)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 24u + (6911)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(20u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 207u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1470(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1470_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1470_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1470_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1470_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.647 avalanche=99.248 bic=10.553 input_mean=81.725 input_floor=73.483 input_tail=73.500 composite=56.218 candidate_id=1225
// Candidate 1225: TwistCandidate_1225
// family=mechanical_loops op_budget=3 loop_shapes=2x3/3x0/3x2
// mechanical_loops[l1=2x3; l2=3x0; l3=3x2; key_rot=11; twiddle=0x4b5451db]
static void TwistCandidate_1225_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (2242)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (6722)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 119u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1225_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (5217)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-7147)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 30u + static_cast<unsigned int>(119u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 80u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1225_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4B5451DBu ^ (pRound * 39u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2242), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x76C86D1Cu ^ static_cast<std::uint32_t>(476u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7162), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6075), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 12u + ((key_byte + 21u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 21u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 250u, 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 250u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x5730531Cu ^ static_cast<std::uint32_t>(2258u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6221), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1656), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2307), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ ((b << 4u) & 0xFFu) ^ (c) ^ (key_byte) ^ ((feedback_byte + 27u) & 0xFFu));
      const std::uint32_t e = (((c >> 1u) & 0xFFu) ^ (b + 23u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 27u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 165u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 86u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD0786501u ^ static_cast<std::uint32_t>(329u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3302), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5291), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2318), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b + 16u + (salt_byte ^ 17u)) ^ ((c >> 2u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 26u ^ ((key_byte + 12u) & 0xFFu)) ^ ((b << 2u) & 0xFFu) + ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 12u) & 0xFFu) + ((feedback_byte + 25u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 134u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 170u), 13u);
    }
  }

}

static void TwistCandidate_1225_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-7540)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-993)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6361)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 7u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 14u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 80u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1225(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1225_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1225_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1225_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1225_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.444 avalanche=99.411 bic=11.006 input_mean=78.070 input_floor=73.488 input_tail=73.731 composite=55.515 candidate_id=71
// Candidate 71: TwistCandidate_0071
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x0/3x3
// mechanical_loops[l1=2x2; l2=3x0; l3=3x3; key_rot=19; twiddle=0xf3d0f240]
static void TwistCandidate_0071_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-5408)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-6987)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 155u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0071_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (4250)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (741)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 30u + static_cast<unsigned int>(155u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 187u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0071_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF3D0F240u ^ (pRound * 46u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5408), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAFEDEF0Au ^ static_cast<std::uint32_t>(5400u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7024), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1513), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 1u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 25u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 146u), 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 137u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2D0AFCA7u ^ static_cast<std::uint32_t>(1970u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2800), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6145) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1375) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 22u + (salt_byte)) ^ ((b << 4u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 25u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 16u), 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 154u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x38FAB6B3u ^ static_cast<std::uint32_t>(4255u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7308), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6466), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5097), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (c + 1u + (salt_byte ^ 13u)) ^ ((b * 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 1u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 16u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 62u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 200u), 12u);
    }
  }

}

static void TwistCandidate_0071_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-2995)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (2232)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 23u + (4919)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 23u + static_cast<unsigned int>(19u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 187u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0071(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0071_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0071_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0071_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0071_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.332 avalanche=99.238 bic=13.851 input_mean=81.313 input_floor=73.366 input_tail=73.368 composite=54.417 candidate_id=527
// Candidate 527: TwistCandidate_0527
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x3/3x2
// mechanical_loops[l1=2x2; l2=3x3; l3=3x2; key_rot=28; twiddle=0xeeb1a73e]
static void TwistCandidate_0527_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (2722)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (6624)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 25u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0527_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (7113)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-4254)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 26u + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 201u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0527_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xEEB1A73Eu ^ (pRound * 8u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2722), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xFCFB4B7Cu ^ static_cast<std::uint32_t>(217u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4493), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5693), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 16u + ((key_byte + 28u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 19u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 28u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 244u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 94u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8D893BC6u ^ static_cast<std::uint32_t>(5911u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7113), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6391), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-7593), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 6u + ((key_byte + 18u) & 0xFFu)) ^ (c + 14u + (salt_byte)) ^ ((b * 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ ((a << 5u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 18u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 23u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 6u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x75C37515u ^ static_cast<std::uint32_t>(1367u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2868), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2154), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (653), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 19u + (salt_byte)) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 26u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 127u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 2u), 6u);
    }
  }

}

static void TwistCandidate_0527_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-3225)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (3239)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3257)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 19u + static_cast<unsigned int>(28u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(11u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 201u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0527(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0527_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0527_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0527_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0527_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=89.323 avalanche=99.160 bic=10.739 input_mean=81.606 input_floor=73.486 input_tail=73.573 composite=56.133 candidate_id=496
// Candidate 496: TwistCandidate_0496
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x2/3x0
// mechanical_loops[l1=3x0; l2=2x2; l3=3x0; key_rot=28; twiddle=0xd868c8a4]
static void TwistCandidate_0496_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (6444)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (1022)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 119u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0496_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-5076)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-3597)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 9u + static_cast<unsigned int>(119u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 101u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0496_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xD868C8A4u ^ (pRound * 39u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6444), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x27C9EF89u ^ static_cast<std::uint32_t>(5197u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6655), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7247), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-5789), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ (c) ^ ((key_byte + 31u) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b + 3u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 5u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 31u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 31u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 115u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1DABC807u ^ static_cast<std::uint32_t>(7682u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4345), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4824) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 23u + (key_byte)) ^ ((b << 2u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 2u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 186u), 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 205u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAA3AAF6Cu ^ static_cast<std::uint32_t>(12631u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6967), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1640), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4024), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ ((b << 2u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 1u) & 0xFFu) ^ (b + 29u + (twiddle_byte)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 26u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 182u, 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 90u), 12u);
    }
  }

}

static void TwistCandidate_0496_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-2886)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (3843)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-3976)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 12u + static_cast<unsigned int>(28u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 101u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0496(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0496_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0496_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0496_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0496_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=88.349 avalanche=99.027 bic=11.659 input_mean=81.638 input_floor=73.473 input_tail=73.513 composite=54.519 candidate_id=109
// Candidate 109: TwistCandidate_0109
// family=mechanical_loops op_budget=3 loop_shapes=2x1/2x3/3x1
// mechanical_loops[l1=2x1; l2=2x3; l3=3x1; key_rot=22; twiddle=0xcf397037]
static void TwistCandidate_0109_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-5698)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (1928)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 234u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0109_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (7507)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-1044)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 9u + static_cast<unsigned int>(234u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 102u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0109_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xCF397037u ^ (pRound * 56u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5698), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xBD180048u ^ static_cast<std::uint32_t>(6122u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2163), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4638), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 4u + (key_byte)) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 14u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 60u, 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 252u, 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA44B5096u ^ static_cast<std::uint32_t>(16408u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7048), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3559), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 13u + ((key_byte + 1u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 20u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 140u, 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 6u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x19E13BDFu ^ static_cast<std::uint32_t>(5147u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3945), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2481), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-6611), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte ^ 19u)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 22u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a) ^ (key_byte) ^ ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 19u) ^ ((feedback_byte + 24u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 45u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 31u, 10u);
    }
  }

}

static void TwistCandidate_0109_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (7006)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (5678)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-4361)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(22u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 102u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0109(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0109_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0109_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0109_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0109_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=88.287 avalanche=99.015 bic=14.248 input_mean=81.486 input_floor=73.357 input_tail=73.451 composite=57.817 candidate_id=160
// Candidate 160: TwistCandidate_0160
// family=mechanical_loops op_budget=3 loop_shapes=3x2/2x0/3x0
// mechanical_loops[l1=3x2; l2=2x0; l3=3x0; key_rot=30; twiddle=0x648524fa]
static void TwistCandidate_0160_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-333)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-4986)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 144u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0160_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-4107)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (7668)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 6u + static_cast<unsigned int>(144u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 205u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0160_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x648524FAu ^ (pRound * 9u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-333), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAEE1527Eu ^ static_cast<std::uint32_t>(6119u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3591), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2384), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (7326), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 17u + (salt_byte)) ^ ((c >> 3u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 13u ^ (key_byte)) ^ (b) + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 63u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 158u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x62A6764Au ^ static_cast<std::uint32_t>(341u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4888), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 1u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6105), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 10u + (key_byte)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 28u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 75u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 133u, 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC55EE750u ^ static_cast<std::uint32_t>(10605u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1615), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7343), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4877), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ (c) ^ ((key_byte + 23u) & 0xFFu) ^ ((feedback_byte + 16u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b + 15u + (twiddle_byte)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 16u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 75u, 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 88u), 6u);
    }
  }

}

static void TwistCandidate_0160_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-6941)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (5311)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-3209)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(30u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 205u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0160(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0160_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0160_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0160_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0160_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=87.727 avalanche=99.181 bic=12.331 input_mean=77.703 input_floor=73.391 input_tail=73.426 composite=53.612 candidate_id=1472
// Candidate 1472: TwistCandidate_1472
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x0/3x2
// mechanical_loops[l1=2x3; l2=2x0; l3=3x2; key_rot=24; twiddle=0xf972942e]
static void TwistCandidate_1472_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-5735)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-4141)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 112u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1472_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (2756)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (5893)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 17u + static_cast<unsigned int>(112u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 208u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1472_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF972942Eu ^ (pRound * 58u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5735), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4D07FE33u ^ static_cast<std::uint32_t>(11493u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6727), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1696), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 29u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 29u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 41u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 27u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8277B7B6u ^ static_cast<std::uint32_t>(197u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1124), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5307) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 93u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 164u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6E19F14Bu ^ static_cast<std::uint32_t>(2578u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1343), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6171), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4936), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 2u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 16u ^ (key_byte)) ^ (b) + ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 24u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 223u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 105u), 5u);
    }
  }

}

static void TwistCandidate_1472_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-4215)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-411)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 20u + (2588)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 11u + static_cast<unsigned int>(24u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 208u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1472(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1472_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1472_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1472_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1472_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=87.421 avalanche=99.196 bic=11.197 input_mean=77.683 input_floor=73.429 input_tail=73.458 composite=53.538 candidate_id=343
// Candidate 343: TwistCandidate_0343
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x0/3x2
// mechanical_loops[l1=2x2; l2=2x0; l3=3x2; key_rot=6; twiddle=0x9ad4957d]
static void TwistCandidate_0343_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4518)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-6392)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 218u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0343_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (7661)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (1877)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 16u + static_cast<unsigned int>(218u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 57u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0343_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x9AD4957Du ^ (pRound * 26u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-4518), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9C5903F4u ^ static_cast<std::uint32_t>(2009u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3841), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 1u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1666), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 2u) & 0xFFu) ^ (salt_byte ^ 16u) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 181u, 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 47u, 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x37653DD7u ^ static_cast<std::uint32_t>(346u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-910), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2596), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 6u + ((key_byte + 26u) & 0xFFu)) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 15u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 1u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 242u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x5E5EFFAEu ^ static_cast<std::uint32_t>(865u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7319), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3385), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (3069), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 8u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 13u ^ (key_byte)) ^ ((b << 2u) & 0xFFu) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 100u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 147u), 10u);
    }
  }

}

static void TwistCandidate_0343_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (2756)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-737)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-699)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(6u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 57u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0343(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0343_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0343_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0343_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0343_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=86.983 avalanche=99.060 bic=11.629 input_mean=81.539 input_floor=73.382 input_tail=73.418 composite=55.961 candidate_id=855
// Candidate 855: TwistCandidate_0855
// family=mechanical_loops op_budget=3 loop_shapes=1x3/3x3/3x2
// mechanical_loops[l1=1x3; l2=3x3; l3=3x2; key_rot=19; twiddle=0x64119615]
static void TwistCandidate_0855_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (5471)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (6775)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 122u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0855_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-2881)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (4016)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 29u + static_cast<unsigned int>(122u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 95u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0855_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x64119615u ^ (pRound * 25u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5471), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB5296611u ^ static_cast<std::uint32_t>(2799u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2385), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 24u + (key_byte)) ^ ((a << 4u) & 0xFFu) ^ (salt_byte ^ 24u) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 7u) ^ (a)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 228u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 236u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7DB3801Fu ^ static_cast<std::uint32_t>(9336u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4389), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6712) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (1765) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 27u + ((key_byte + 25u) & 0xFFu)) ^ (c) ^ ((b * 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 2u) & 0xFFu) ^ ((a << 4u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 25u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 114u, 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 141u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3C072BC4u ^ static_cast<std::uint32_t>(14560u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6132), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-933), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-7495), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 17u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 134u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 88u), 1u);
    }
  }

}

static void TwistCandidate_0855_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (4452)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-7060)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 22u + (88)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 6u + static_cast<unsigned int>(19u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 95u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0855(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0855_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0855_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0855_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0855_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=86.935 avalanche=99.225 bic=10.271 input_mean=85.070 input_floor=73.455 input_tail=73.466 composite=56.922 candidate_id=570
// Candidate 570: TwistCandidate_0570
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x0/3x2
// mechanical_loops[l1=2x0; l2=3x0; l3=3x2; key_rot=1; twiddle=0x38f77e2f]
static void TwistCandidate_0570_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (1916)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (6666)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 76u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0570_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (6981)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-1891)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 17u + static_cast<unsigned int>(76u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 61u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0570_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x38F77E2Fu ^ (pRound * 14u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1916), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x82806DEDu ^ static_cast<std::uint32_t>(9542u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3043), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (193), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 27u + ((key_byte + 27u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 13u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 17u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 13u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 96u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 56u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6B6AC591u ^ static_cast<std::uint32_t>(10653u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5357), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6761) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1465) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 1u + (salt_byte ^ 3u)) ^ (b) ^ (c) ^ ((key_byte + 24u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 1u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 3u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 158u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 84u, 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xEBBD2EFDu ^ static_cast<std::uint32_t>(7682u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3591), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1348), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2743), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 21u + (salt_byte ^ 30u)) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 2u ^ ((key_byte + 30u) & 0xFFu)) ^ (b) + ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 30u) & 0xFFu) + ((feedback_byte + 31u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 61u, 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 204u), 8u);
    }
  }

}

static void TwistCandidate_0570_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (4213)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-1973)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 1u + (3212)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 11u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 61u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0570(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0570_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0570_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0570_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0570_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.940 avalanche=99.179 bic=12.924 input_mean=77.563 input_floor=73.384 input_tail=73.395 composite=55.229 candidate_id=1165
// Candidate 1165: TwistCandidate_1165
// family=mechanical_loops op_budget=3 loop_shapes=1x0/2x2/3x1
// mechanical_loops[l1=1x0; l2=2x2; l3=3x1; key_rot=13; twiddle=0x4a1f496e]
static void TwistCandidate_1165_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (2337)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (4605)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 241u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1165_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (3315)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-3328)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 2u + static_cast<unsigned int>(241u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 229u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1165_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4A1F496Eu ^ (pRound * 41u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2337), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x35A88229u ^ static_cast<std::uint32_t>(1334u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2681), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 18u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 2u) & 0xFFu) ^ (salt_byte ^ 23u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 26u) ^ ((a >> 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 244u), 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 49u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7BDBB1F7u ^ static_cast<std::uint32_t>(3275u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1413), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2581) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 30u + ((key_byte + 3u) & 0xFFu)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 1u) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 11u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 31u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 165u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 243u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x87073AEBu ^ static_cast<std::uint32_t>(1996u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6183), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5028), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (841), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a ^ (b + 1u + ((key_byte + 1u) & 0xFFu) + (feedback_byte))) + ((c << 4u) & 0xFFu) + (salt_byte ^ 11u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 18u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 3u) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 11u) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 22u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 25u), 2u);
    }
  }

}

static void TwistCandidate_1165_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (2518)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (7401)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-5834)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(13u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 229u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1165(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1165_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1165_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1165_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1165_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.899 avalanche=99.263 bic=10.441 input_mean=85.276 input_floor=73.386 input_tail=73.391 composite=56.827 candidate_id=1344
// Candidate 1344: TwistCandidate_1344
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x0/3x2
// mechanical_loops[l1=3x2; l2=3x0; l3=3x2; key_rot=23; twiddle=0x1bed02d2]
static void TwistCandidate_1344_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-2609)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-6204)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 77u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1344_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-5651)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (3583)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 10u + static_cast<unsigned int>(77u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 171u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1344_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x1BED02D2u ^ (pRound * 32u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2609), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xEA4C10DDu ^ static_cast<std::uint32_t>(17492u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6670), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3411), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (7411), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ ((c >> 3u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 37u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 118u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0AD49D4Eu ^ static_cast<std::uint32_t>(6225u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6794), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2461), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (3030), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 12u + (salt_byte ^ 19u)) ^ (b) ^ (c) ^ ((key_byte + 27u) & 0xFFu) ^ ((feedback_byte + 2u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b + 22u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 19u) ^ (((feedback_byte + 2u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 25u), 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 209u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1FD798FAu ^ static_cast<std::uint32_t>(14236u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5309), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-7150), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-1777), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 21u + (salt_byte ^ 18u)) ^ (c) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 7u ^ (key_byte)) ^ (b) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 99u, 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 230u), 6u);
    }
  }

}

static void TwistCandidate_1344_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (4606)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-1819)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-2513)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 3u + static_cast<unsigned int>(23u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 171u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1344(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1344_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1344_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1344_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1344_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.775 avalanche=98.897 bic=12.116 input_mean=81.531 input_floor=73.446 input_tail=73.537 composite=54.490 candidate_id=800
// Candidate 800: TwistCandidate_0800
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x0/3x2
// mechanical_loops[l1=3x0; l2=2x0; l3=3x2; key_rot=1; twiddle=0x2827308d]
static void TwistCandidate_0800_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (1879)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-1342)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 220u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0800_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (4793)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-7212)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 15u + static_cast<unsigned int>(220u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 11u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0800_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x2827308Du ^ (pRound * 39u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1879), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xED70C252u ^ static_cast<std::uint32_t>(173u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3137), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6155), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-2845), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 25u + (salt_byte)) ^ (b) ^ (c) ^ ((key_byte + 11u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 7u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + (feedback_byte)) & 0xFFu) ^ ((key_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 146u), 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 74u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x37C44F57u ^ static_cast<std::uint32_t>(10816u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1915), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6144) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 21u + ((key_byte + 9u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 28u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 146u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 207u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x73C92EE3u ^ static_cast<std::uint32_t>(15045u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7257), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1852), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5936), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 23u + (salt_byte ^ 7u)) ^ (c) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ ((b << 4u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 134u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 194u, 11u);
    }
  }

}

static void TwistCandidate_0800_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-1541)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (4806)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-2298)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 11u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0800(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0800_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0800_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0800_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0800_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.693 avalanche=99.147 bic=11.417 input_mean=77.828 input_floor=73.384 input_tail=73.418 composite=53.766 candidate_id=380
// Candidate 380: TwistCandidate_0380
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x0/3x3
// mechanical_loops[l1=2x0; l2=2x0; l3=3x3; key_rot=2; twiddle=0x32bdc718]
static void TwistCandidate_0380_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (6792)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-4160)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 56u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0380_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-1550)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-7500)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 4u + static_cast<unsigned int>(56u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 119u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0380_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x32BDC718u ^ (pRound * 14u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6792), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x00F28647u ^ static_cast<std::uint32_t>(4212u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2179), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6019), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 22u + ((key_byte + 1u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 70u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 207u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3C4529DFu ^ static_cast<std::uint32_t>(5130u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1701), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2534) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 2u) & 0xFFu) ^ (salt_byte ^ 3u) ^ ((feedback_byte + 7u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 7u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 149u, 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 219u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x33412B9Cu ^ static_cast<std::uint32_t>(8481u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7580), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4478), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3577), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a + 17u + ((key_byte + 11u) & 0xFFu)) ^ (c + 24u + (salt_byte ^ 17u)) ^ ((b * 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 3u) & 0xFFu) ^ (a) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 11u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 188u, 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 110u), 7u);
    }
  }

}

static void TwistCandidate_0380_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (1735)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (5889)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (3034)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 6u + static_cast<unsigned int>(2u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 12u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 119u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0380(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0380_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0380_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0380_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0380_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.130 avalanche=98.970 bic=11.543 input_mean=81.593 input_floor=73.377 input_tail=73.429 composite=57.927 candidate_id=1200
// Candidate 1200: TwistCandidate_1200
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x3/3x0
// mechanical_loops[l1=3x0; l2=2x3; l3=3x0; key_rot=26; twiddle=0x5ab883df]
static void TwistCandidate_1200_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-1920)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-2150)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 187u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1200_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-1436)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (3210)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 15u + static_cast<unsigned int>(187u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 50u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1200_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x5AB883DFu ^ (pRound * 55u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1920), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3C041C28u ^ static_cast<std::uint32_t>(1601u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5342), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 5u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3288), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-7029), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 28u + (salt_byte)) ^ (b) ^ (c) ^ (key_byte) ^ ((feedback_byte + 15u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 22u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 15u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 112u, 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 26u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x09BAF422u ^ static_cast<std::uint32_t>(18750u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6542), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6959), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 27u + (key_byte)) ^ (b) ^ (salt_byte ^ 7u) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 38u), 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 246u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x5E1A1DCFu ^ static_cast<std::uint32_t>(8958u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6785), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7091), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4918), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 3u + (salt_byte)) ^ ((b << 2u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 21u) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 14u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 20u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 20u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 185u, 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 23u), 9u);
    }
  }

}

static void TwistCandidate_1200_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-5282)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (2743)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-4441)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(26u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(13u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 50u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1200(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1200_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1200_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1200_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1200_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=85.078 avalanche=99.097 bic=11.086 input_mean=81.420 input_floor=73.393 input_tail=73.393 composite=56.037 candidate_id=1186
// Candidate 1186: TwistCandidate_1186
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x0/3x1
// mechanical_loops[l1=2x2; l2=2x0; l3=3x1; key_rot=9; twiddle=0xa3e37d5e]
static void TwistCandidate_1186_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (7523)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (2907)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 240u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1186_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (7086)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-1469)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 17u + static_cast<unsigned int>(240u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 55u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1186_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA3E37D5Eu ^ (pRound * 27u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7523), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x70501850u ^ static_cast<std::uint32_t>(9378u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (651), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6546), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 9u + ((key_byte + 25u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 22u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 25u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 151u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 65u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x720E2185u ^ static_cast<std::uint32_t>(851u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4155), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1847) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 20u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 16u) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 15u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 181u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 170u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9C207D66u ^ static_cast<std::uint32_t>(4780u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2705), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-797), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-1278), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) + ((c << 3u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c) ^ (a) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 34u), 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 116u), 9u);
    }
  }

}

static void TwistCandidate_1186_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (6340)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-391)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 7u + (3012)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 14u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 55u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1186(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1186_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1186_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1186_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1186_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=84.756 avalanche=99.091 bic=10.112 input_mean=85.390 input_floor=73.509 input_tail=73.525 composite=56.980 candidate_id=688
// Candidate 688: TwistCandidate_0688
// family=mechanical_loops op_budget=3 loop_shapes=1x1/2x3/3x2
// mechanical_loops[l1=1x1; l2=2x3; l3=3x2; key_rot=22; twiddle=0xe2ee1807]
static void TwistCandidate_0688_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-2835)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-2917)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 122u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0688_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 19u + (3947)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (1314)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 19u + static_cast<unsigned int>(122u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 41u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0688_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE2EE1807u ^ (pRound * 44u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2835), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7AEDACDAu ^ static_cast<std::uint32_t>(5932u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3189), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 4u) & 0xFFu) ^ (salt_byte ^ 1u) ^ ((feedback_byte + 7u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 18u) ^ ((a >> 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 138u, 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 165u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4D03A630u ^ static_cast<std::uint32_t>(15890u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6452), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2729) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 25u + ((key_byte + 14u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 20u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 190u, 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 8u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x184AB196u ^ static_cast<std::uint32_t>(4286u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3207), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (404), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (675), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ (b) + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 24u) & 0xFFu) + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 11u), 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 163u), 11u);
    }
  }

}

static void TwistCandidate_0688_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (1039)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (4216)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 27u + (4538)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 22u + static_cast<unsigned int>(22u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 41u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0688(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0688_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0688_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0688_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0688_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=84.318 avalanche=99.143 bic=13.829 input_mean=77.454 input_floor=73.373 input_tail=73.387 composite=53.382 candidate_id=547
// Candidate 547: TwistCandidate_0547
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x1/3x3
// mechanical_loops[l1=2x0; l2=2x1; l3=3x3; key_rot=26; twiddle=0x4cc4945f]
static void TwistCandidate_0547_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-3089)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (5777)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 67u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0547_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-7641)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (2444)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 20u + static_cast<unsigned int>(67u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 52u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0547_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4CC4945Fu ^ (pRound * 51u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3089), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE20EA849u ^ static_cast<std::uint32_t>(14158u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7537), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (429), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 12u + (key_byte)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 15u) ^ ((feedback_byte + 22u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 20u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 22u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 24u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 145u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2A3853A9u ^ static_cast<std::uint32_t>(17457u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7665), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5582), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 12u + ((key_byte + 21u) & 0xFFu)) ^ ((b << 2u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 21u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 222u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 61u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8AFEB874u ^ static_cast<std::uint32_t>(4119u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1368), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2756), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5507), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (c + 22u + (salt_byte)) ^ (b) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (c) ^ (twiddle_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 31u) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 174u, 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 20u), 5u);
    }
  }

}

static void TwistCandidate_0547_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 19u + (2000)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-220)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-3756)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 19u + static_cast<unsigned int>(26u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 52u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0547(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0547_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0547_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0547_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0547_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=84.150 avalanche=99.187 bic=10.142 input_mean=85.135 input_floor=73.422 input_tail=73.425 composite=55.076 candidate_id=143
// Candidate 143: TwistCandidate_0143
// family=mechanical_loops op_budget=3 loop_shapes=1x1/3x1/3x3
// mechanical_loops[l1=1x1; l2=3x1; l3=3x3; key_rot=18; twiddle=0x6f3c12bb]
static void TwistCandidate_0143_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-2221)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (2161)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 7u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0143_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-2273)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (4666)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 2u + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 80u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0143_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x6F3C12BBu ^ (pRound * 22u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2221), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x75379A9Au ^ static_cast<std::uint32_t>(65u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2455), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 27u + ((key_byte + 8u) & 0xFFu)) ^ ((a << 1u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + 20u) ^ ((a >> 3u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 59u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 205u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x21C7760Fu ^ static_cast<std::uint32_t>(9022u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7399), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2514), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-4137), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 23u + ((key_byte + 26u) & 0xFFu) + (feedback_byte))) + ((c << 1u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c) ^ ((a >> 4u) & 0xFFu) ^ ((key_byte + 26u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 156u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 175u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC91F2BA5u ^ static_cast<std::uint32_t>(3858u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-219), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (554), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4193), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a + 5u + ((key_byte + 11u) & 0xFFu)) ^ (c + 8u + (salt_byte ^ 20u)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 4u) & 0xFFu) ^ (a) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 11u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 209u, 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 253u, 13u);
    }
  }

}

static void TwistCandidate_0143_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-5228)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-464)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-3984)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 30u + static_cast<unsigned int>(18u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(13u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 10u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 80u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0143(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0143_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0143_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0143_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0143_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=84.118 avalanche=99.296 bic=11.743 input_mean=81.304 input_floor=73.336 input_tail=73.353 composite=54.287 candidate_id=1477
// Candidate 1477: TwistCandidate_1477
// family=mechanical_loops op_budget=3 loop_shapes=1x1/2x3/3x1
// mechanical_loops[l1=1x1; l2=2x3; l3=3x1; key_rot=19; twiddle=0x6bb10f5d]
static void TwistCandidate_1477_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (2128)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (5786)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 34u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1477_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-119)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-654)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 21u + static_cast<unsigned int>(34u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 229u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1477_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x6BB10F5Du ^ (pRound * 45u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2128), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x626DAFEEu ^ static_cast<std::uint32_t>(5679u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1535), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 2u + ((key_byte + 15u) & 0xFFu)) ^ ((a << 5u) & 0xFFu) ^ (salt_byte ^ 11u) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 27u) ^ ((a >> 2u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 15u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 103u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 72u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0712FB2Du ^ static_cast<std::uint32_t>(3468u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3592), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-734), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 9u) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 31u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 26u), 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 3u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6CE4A9F1u ^ static_cast<std::uint32_t>(1409u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7329), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5508), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3230), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a ^ (b + 23u + ((key_byte + 21u) & 0xFFu) + ((feedback_byte + 31u) & 0xFFu))) + ((c << 1u) & 0xFFu) + (salt_byte ^ 15u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c) ^ (a) ^ ((key_byte + 21u) & 0xFFu) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 15u) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 219u), 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 118u), 6u);
    }
  }

}

static void TwistCandidate_1477_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (270)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-7385)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-4446)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(19u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 229u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1477(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1477_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1477_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1477_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1477_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=84.004 avalanche=99.143 bic=11.323 input_mean=77.452 input_floor=73.458 input_tail=73.459 composite=55.178 candidate_id=728
// Candidate 728: TwistCandidate_0728
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x1/3x0
// mechanical_loops[l1=2x3; l2=2x1; l3=3x0; key_rot=31; twiddle=0x338c1b03]
static void TwistCandidate_0728_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-5727)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (5576)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 112u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0728_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-3831)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-3812)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 25u + static_cast<unsigned int>(112u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 21u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0728_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x338C1B03u ^ (pRound * 50u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5727), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x96B6D708u ^ static_cast<std::uint32_t>(5951u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6963), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6327), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 24u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 25u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 72u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 119u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC057DD71u ^ static_cast<std::uint32_t>(6613u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5303), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3356), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 10u + (key_byte)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 9u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 31u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 162u), 2u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 215u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x17736BEDu ^ static_cast<std::uint32_t>(8916u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5026), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 1u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3359), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-531), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ ((b << 5u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu) ^ ((feedback_byte + 8u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 11u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 8u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 73u), 4u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 234u), 13u);
    }
  }

}

static void TwistCandidate_0728_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (4467)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (6492)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 24u + (641)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 22u + static_cast<unsigned int>(31u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 21u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0728(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0728_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0728_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0728_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0728_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.760 avalanche=98.829 bic=12.481 input_mean=81.708 input_floor=74.016 input_tail=74.047 composite=56.348 candidate_id=1335
// Candidate 1335: TwistCandidate_1335
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x2/3x0
// mechanical_loops[l1=2x2; l2=3x2; l3=3x0; key_rot=16; twiddle=0x0631e7f1]
static void TwistCandidate_1335_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-2128)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6385)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 226u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1335_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (1779)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (1615)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 9u + static_cast<unsigned int>(226u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 164u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1335_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x0631E7F1u ^ (pRound * 16u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2128), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x96C7E266u ^ static_cast<std::uint32_t>(8927u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-523), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6715), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 10u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 35u), 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 203u, 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9F74E62Fu ^ static_cast<std::uint32_t>(449u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3078), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5588), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (2061), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 6u ^ ((key_byte + 6u) & 0xFFu)) ^ (b) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 6u) & 0xFFu) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 205u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 132u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB11DCD6Du ^ static_cast<std::uint32_t>(1968u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-435), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5223), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2820), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ ((b << 1u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 17u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 4u) & 0xFFu) ^ (b + 10u + (twiddle_byte)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 12u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 216u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 97u, 1u);
    }
  }

}

static void TwistCandidate_1335_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-798)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-2040)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-5499)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(16u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 12u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 164u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1335(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1335_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1335_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1335_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1335_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.415 avalanche=99.074 bic=10.320 input_mean=81.562 input_floor=73.465 input_tail=73.573 composite=56.048 candidate_id=1071
// Candidate 1071: TwistCandidate_1071
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x3/3x0
// mechanical_loops[l1=2x3; l2=2x3; l3=3x0; key_rot=10; twiddle=0xeb44577b]
static void TwistCandidate_1071_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (5542)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (6283)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 184u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1071_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (3173)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (1968)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(184u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 131u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1071_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xEB44577Bu ^ (pRound * 13u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5542), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCA86E971u ^ static_cast<std::uint32_t>(6090u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5233), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2112), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 2u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 8u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 10u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 52u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 61u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7658E5E6u ^ static_cast<std::uint32_t>(4876u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2591), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-271) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 24u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 14u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 42u), 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 135u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4322AECBu ^ static_cast<std::uint32_t>(6298u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1428), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 5u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3146), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4580), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 31u + (salt_byte)) ^ (b) ^ ((c * 5u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 9u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 13u + (twiddle_byte)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 9u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 33u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 215u), 13u);
    }
  }

}

static void TwistCandidate_1071_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-778)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-239)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-7492)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 16u + static_cast<unsigned int>(10u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(9u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 14u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 131u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1071(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1071_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1071_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1071_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1071_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.337 avalanche=99.157 bic=11.830 input_mean=77.507 input_floor=73.388 input_tail=73.411 composite=53.461 candidate_id=1343
// Candidate 1343: TwistCandidate_1343
// family=mechanical_loops op_budget=3 loop_shapes=2x3/3x1/3x2
// mechanical_loops[l1=2x3; l2=3x1; l3=3x2; key_rot=13; twiddle=0xe59c83ff]
static void TwistCandidate_1343_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-6845)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-3300)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 32u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1343_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-988)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-2155)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 27u + static_cast<unsigned int>(32u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 232u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1343_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE59C83FFu ^ (pRound * 18u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6845), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x134C52ABu ^ static_cast<std::uint32_t>(256u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4799), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 12u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1609), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 4u) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 21u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 205u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 52u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2E2ABAE8u ^ static_cast<std::uint32_t>(13022u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1722), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7576), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (7168), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) + ((c << 1u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c) ^ ((a >> 2u) & 0xFFu) ^ ((key_byte + 30u) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 123u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 8u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3F148001u ^ static_cast<std::uint32_t>(6186u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1746), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2314), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5618), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b + 13u + (salt_byte)) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 27u ^ ((key_byte + 28u) & 0xFFu)) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 28u) & 0xFFu) + ((feedback_byte + 20u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 62u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 8u), 4u);
    }
  }

}

static void TwistCandidate_1343_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (2478)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (607)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 9u + (3575)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(13u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 232u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1343(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1343_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1343_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1343_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1343_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.325 avalanche=98.905 bic=12.351 input_mean=81.074 input_floor=73.431 input_tail=73.439 composite=54.167 candidate_id=297
// Candidate 297: TwistCandidate_0297
// family=mechanical_loops op_budget=3 loop_shapes=3x1/3x0/3x2
// mechanical_loops[l1=3x1; l2=3x0; l3=3x2; key_rot=12; twiddle=0xb167b508]
static void TwistCandidate_0297_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (4264)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (4767)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 66u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0297_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-2408)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-4857)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 6u + static_cast<unsigned int>(66u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 83u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0297_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xB167B508u ^ (pRound * 6u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4264), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x81FCDFDBu ^ static_cast<std::uint32_t>(1946u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2243), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4820), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-5117), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte ^ 29u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 31u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte ^ 29u) ^ (feedback_byte)) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 198u), 4u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 218u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE769C13Cu ^ static_cast<std::uint32_t>(504u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4303), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3065), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (734), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 15u) & 0xFFu) ^ ((feedback_byte + 14u) & 0xFFu));
      const std::uint32_t e = (((c >> 4u) & 0xFFu) ^ (b + 3u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 9u) ^ (((feedback_byte + 14u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 59u), 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 24u, 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA008B109u ^ static_cast<std::uint32_t>(18118u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5167), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-7409), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-5542), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 3u + (salt_byte ^ 6u)) ^ ((c >> 1u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 4u ^ ((key_byte + 25u) & 0xFFu)) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 25u) & 0xFFu) + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 122u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 230u, 8u);
    }
  }

}

static void TwistCandidate_0297_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-6823)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (4724)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (3276)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(12u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 83u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0297(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0297_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0297_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0297_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0297_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.168 avalanche=99.088 bic=10.394 input_mean=85.391 input_floor=73.388 input_tail=73.390 composite=56.816 candidate_id=533
// Candidate 533: TwistCandidate_0533
// family=mechanical_loops op_budget=3 loop_shapes=1x2/2x2/3x2
// mechanical_loops[l1=1x2; l2=2x2; l3=3x2; key_rot=9; twiddle=0x886ab410]
static void TwistCandidate_0533_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-6752)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (4316)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 44u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0533_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (4699)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (327)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 29u + static_cast<unsigned int>(44u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 124u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0533_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x886AB410u ^ (pRound * 16u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6752), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9B18FE90u ^ static_cast<std::uint32_t>(3442u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3949), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 4u + ((key_byte + 19u) & 0xFFu)) ^ ((a << 2u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 1u) ^ ((a >> 5u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 19u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 250u), 2u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 194u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD1330D56u ^ static_cast<std::uint32_t>(15345u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7117), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5637), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 3u + (key_byte)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 34u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 236u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4C9515D2u ^ static_cast<std::uint32_t>(12379u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (599), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6905), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4875), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b + 31u + (salt_byte)) ^ (c) ^ (twiddle_byte) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 11u ^ ((key_byte + 18u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 18u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 154u), 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 18u, 4u);
    }
  }

}

static void TwistCandidate_0533_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-5914)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (272)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-663)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 13u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 124u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0533(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0533_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0533_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0533_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0533_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.115 avalanche=99.031 bic=12.847 input_mean=77.654 input_floor=73.365 input_tail=73.386 composite=53.712 candidate_id=183
// Candidate 183: TwistCandidate_0183
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x0/3x0
// mechanical_loops[l1=2x3; l2=2x0; l3=3x0; key_rot=23; twiddle=0xcc625dd0]
static void TwistCandidate_0183_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (2393)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-3937)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 232u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0183_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-5895)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (874)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 14u + static_cast<unsigned int>(232u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 23u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0183_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xCC625DD0u ^ (pRound * 55u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2393), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7322F8A6u ^ static_cast<std::uint32_t>(3069u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6790), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1545), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 26u + ((key_byte + 1u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 22u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 17u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 1u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 135u, 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 127u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4A9F3441u ^ static_cast<std::uint32_t>(8585u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7354), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4055) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 1u) ^ ((feedback_byte + 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 18u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 189u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 7u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x5F1C185Cu ^ static_cast<std::uint32_t>(7717u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1463), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-761), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (7015), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 9u + (salt_byte ^ 8u)) ^ ((b << 2u) & 0xFFu) ^ (c) ^ (key_byte) ^ ((feedback_byte + 11u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b + 6u + (twiddle_byte)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 8u) ^ (((feedback_byte + 11u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 78u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 44u), 6u);
    }
  }

}

static void TwistCandidate_0183_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-6984)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-6750)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (1098)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 31u + static_cast<unsigned int>(23u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 23u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0183(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0183_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0183_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0183_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0183_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.103 avalanche=98.952 bic=12.146 input_mean=77.841 input_floor=73.442 input_tail=73.458 composite=55.457 candidate_id=719
// Candidate 719: TwistCandidate_0719
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x2/3x1
// mechanical_loops[l1=2x0; l2=3x2; l3=3x1; key_rot=23; twiddle=0xcb8be053]
static void TwistCandidate_0719_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (3257)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-6611)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 226u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0719_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-6559)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-230)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 24u + static_cast<unsigned int>(226u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 202u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0719_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xCB8BE053u ^ (pRound * 51u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (3257), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8D44BC24u ^ static_cast<std::uint32_t>(8455u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4503), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5188), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 2u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 228u, 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 245u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7F8356A7u ^ static_cast<std::uint32_t>(9773u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4079), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1663) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-7357) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 2u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ (b) + ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 23u) & 0xFFu) + ((feedback_byte + 3u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 147u), 2u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 203u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6278F75Fu ^ static_cast<std::uint32_t>(4231u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-121), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 1u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5994), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1884), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) + ((c << 1u) & 0xFFu) + (salt_byte ^ 2u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 15u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu) ^ ((key_byte + 22u) & 0xFFu) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 2u) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 1u, 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 105u), 10u);
    }
  }

}

static void TwistCandidate_0719_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (890)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-1177)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (4863)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 16u + static_cast<unsigned int>(23u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 202u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0719(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0719_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0719_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0719_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0719_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=83.099 avalanche=99.114 bic=11.313 input_mean=77.570 input_floor=73.399 input_tail=73.430 composite=53.533 candidate_id=275
// Candidate 275: TwistCandidate_0275
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x0/3x2
// mechanical_loops[l1=2x0; l2=2x0; l3=3x2; key_rot=27; twiddle=0xaf6e1c52]
static void TwistCandidate_0275_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-5569)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-2203)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 54u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0275_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (6157)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (2825)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 25u + static_cast<unsigned int>(54u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 80u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0275_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xAF6E1C52u ^ (pRound * 49u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5569), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF8C8F35Eu ^ static_cast<std::uint32_t>(11476u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4936), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1127), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 5u + ((key_byte + 26u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 21u) ^ ((feedback_byte + 8u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 3u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 26u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 174u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 5u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8392C87Bu ^ static_cast<std::uint32_t>(5291u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6894), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3294), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 8u + (key_byte)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 202u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 155u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x74B96B15u ^ static_cast<std::uint32_t>(6530u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (192), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 28u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (725), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-7447), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 27u + (salt_byte ^ 14u)) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 29u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 10u ^ ((key_byte + 25u) & 0xFFu)) ^ (b) + ((feedback_byte + 29u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 25u) & 0xFFu) + ((feedback_byte + 29u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 130u, 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 175u, 13u);
    }
  }

}

static void TwistCandidate_0275_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-6626)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-343)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-2404)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(27u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 5u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 80u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0275(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0275_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0275_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0275_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0275_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.982 avalanche=99.115 bic=11.012 input_mean=85.391 input_floor=73.336 input_tail=73.404 composite=55.142 candidate_id=566
// Candidate 566: TwistCandidate_0566
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x1/3x3
// mechanical_loops[l1=2x0; l2=3x1; l3=3x3; key_rot=15; twiddle=0x91a515cc]
static void TwistCandidate_0566_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-6281)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (5033)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 26u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0566_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-3365)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-3762)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(26u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 166u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0566_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x91A515CCu ^ (pRound * 5u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6281), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC9CCA31Bu ^ static_cast<std::uint32_t>(10242u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2359), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 18u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1925), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 22u + (key_byte)) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 7u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 205u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 14u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA73FCD6Bu ^ static_cast<std::uint32_t>(13497u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6214), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2815) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-4468) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) + ((c << 1u) & 0xFFu) + (salt_byte ^ 8u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c) ^ ((a >> 2u) & 0xFFu) ^ ((key_byte + 27u) & 0xFFu) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 8u) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 145u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 94u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6F467095u ^ static_cast<std::uint32_t>(156u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4219), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6691), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2316), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 15u + ((key_byte + 25u) & 0xFFu)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ ((a << 3u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 25u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 165u), 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 196u), 2u);
    }
  }

}

static void TwistCandidate_0566_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (7479)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (1500)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-653)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(15u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 166u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0566(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0566_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0566_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0566_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0566_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.551 avalanche=99.130 bic=10.197 input_mean=81.404 input_floor=73.402 input_tail=73.409 composite=54.519 candidate_id=456
// Candidate 456: TwistCandidate_0456
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x1/3x2
// mechanical_loops[l1=2x2; l2=3x1; l3=3x2; key_rot=7; twiddle=0xeedd72f4]
static void TwistCandidate_0456_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-5801)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 15u + (852)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 15u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0456_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-4261)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-5368)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 4u + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 54u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0456_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xEEDD72F4u ^ (pRound * 32u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5801), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x753BCB68u ^ static_cast<std::uint32_t>(11685u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3996), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1893), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 11u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 11u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 239u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 185u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7B0E1CBFu ^ static_cast<std::uint32_t>(10516u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6707), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5971), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2162), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 29u + ((key_byte + 24u) & 0xFFu) + ((feedback_byte + 8u) & 0xFFu))) + ((c << 4u) & 0xFFu) + (salt_byte ^ 3u)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 19u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu) ^ ((key_byte + 24u) & 0xFFu) ^ ((feedback_byte + 8u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 3u) ^ ((feedback_byte + 8u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 173u), 2u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 174u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x74AB7F09u ^ static_cast<std::uint32_t>(1592u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5542), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4648), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2486), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 28u + (salt_byte)) ^ (c) ^ (twiddle_byte) ^ ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 17u ^ ((key_byte + 6u) & 0xFFu)) ^ ((b << 2u) & 0xFFu) + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 6u) & 0xFFu) + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 52u, 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 107u, 7u);
    }
  }

}

static void TwistCandidate_0456_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-6171)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (7534)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 17u + (2676)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 23u + static_cast<unsigned int>(7u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 54u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0456(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0456_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0456_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0456_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0456_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.446 avalanche=99.128 bic=11.633 input_mean=77.791 input_floor=73.368 input_tail=73.372 composite=53.682 candidate_id=828
// Candidate 828: TwistCandidate_0828
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x3/3x1
// mechanical_loops[l1=2x2; l2=3x3; l3=3x1; key_rot=11; twiddle=0xa1307ac4]
static void TwistCandidate_0828_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-104)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (1020)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 111u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0828_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (4188)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-5745)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 28u + static_cast<unsigned int>(111u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 126u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0828_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA1307AC4u ^ (pRound * 26u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-104), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4AEA7619u ^ static_cast<std::uint32_t>(3881u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5719), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4746), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 4u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 115u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 100u, 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x56920BA3u ^ static_cast<std::uint32_t>(19643u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6471), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6976), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (6196), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 24u + (key_byte)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 5u) & 0xFFu) ^ (a) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 108u), 2u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 225u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6A695E77u ^ static_cast<std::uint32_t>(16963u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5327), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7447), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4189), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 24u + (key_byte) + (feedback_byte))) + ((c << 5u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c) ^ ((a >> 4u) & 0xFFu) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 212u, 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 175u, 10u);
    }
  }

}

static void TwistCandidate_0828_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-4295)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-2940)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-513)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 26u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 1u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 126u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0828(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0828_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0828_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0828_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0828_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.329 avalanche=99.297 bic=10.526 input_mean=81.489 input_floor=73.344 input_tail=73.387 composite=56.001 candidate_id=631
// Candidate 631: TwistCandidate_0631
// family=mechanical_loops op_budget=3 loop_shapes=3x1/2x1/3x1
// mechanical_loops[l1=3x1; l2=2x1; l3=3x1; key_rot=29; twiddle=0xe0e54824]
static void TwistCandidate_0631_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-2636)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (2870)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 247u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0631_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (6092)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (3052)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 28u + static_cast<unsigned int>(247u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 179u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0631_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE0E54824u ^ (pRound * 39u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2636), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8E15FB99u ^ static_cast<std::uint32_t>(4527u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6930), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2996), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-593), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) + ((c << 2u) & 0xFFu) + (salt_byte ^ 31u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c) ^ ((a >> 5u) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte ^ 31u) ^ (feedback_byte)) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 86u, 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 95u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB50DEED5u ^ static_cast<std::uint32_t>(1759u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5934), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6258) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 9u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 27u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 3u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 41u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x50AFDAF8u ^ static_cast<std::uint32_t>(4740u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1670), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3562), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-492), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a ^ (b + 23u + (key_byte) + (feedback_byte))) + (c) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c + 9u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 204u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 221u), 5u);
    }
  }

}

static void TwistCandidate_0631_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-2895)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (760)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (2658)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 11u + static_cast<unsigned int>(29u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(9u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 179u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0631(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0631_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0631_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0631_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0631_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.202 avalanche=98.891 bic=10.992 input_mean=81.477 input_floor=73.430 input_tail=73.445 composite=54.436 candidate_id=1203
// Candidate 1203: TwistCandidate_1203
// family=mechanical_loops op_budget=3 loop_shapes=1x2/3x3/3x3
// mechanical_loops[l1=1x2; l2=3x3; l3=3x3; key_rot=17; twiddle=0x691bb6d4]
static void TwistCandidate_1203_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (4754)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-4399)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 170u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1203_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (1597)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-202)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(170u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 165u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1203_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x691BB6D4u ^ (pRound * 19u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4754), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9B463C07u ^ static_cast<std::uint32_t>(4926u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6430), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 6u + (key_byte)) ^ (a) ^ (salt_byte ^ 31u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + 2u) ^ (a)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 18u, 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 129u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD9B1D947u ^ static_cast<std::uint32_t>(4476u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2744), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6667) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-553) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (c + 14u + (salt_byte)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 5u) & 0xFFu) ^ ((a << 5u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 215u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 217u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x38AA397Au ^ static_cast<std::uint32_t>(2790u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6297), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5395), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (3692), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (c + 5u + (salt_byte ^ 13u)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 242u), 3u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 149u, 7u);
    }
  }

}

static void TwistCandidate_1203_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (5507)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (6530)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-949)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 26u + static_cast<unsigned int>(17u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 165u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1203(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1203_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1203_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1203_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1203_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=82.060 avalanche=98.895 bic=14.081 input_mean=81.370 input_floor=73.347 input_tail=73.407 composite=56.010 candidate_id=583
// Candidate 583: TwistCandidate_0583
// family=mechanical_loops op_budget=3 loop_shapes=2x3/2x3/3x0
// mechanical_loops[l1=2x3; l2=2x3; l3=3x0; key_rot=2; twiddle=0x4199d3a4]
static void TwistCandidate_0583_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (7341)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-4110)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 182u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0583_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-3892)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (7164)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 17u + static_cast<unsigned int>(182u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 252u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0583_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4199D3A4u ^ (pRound * 35u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7341), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7F8E4499u ^ static_cast<std::uint32_t>(1795u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1255), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3884), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 7u) ^ ((feedback_byte + 21u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 21u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 191u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 56u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDE4E25CDu ^ static_cast<std::uint32_t>(5685u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (764), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3416), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 13u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 15u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 13u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 73u), 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 138u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9B7AB225u ^ static_cast<std::uint32_t>(401u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (101), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 19u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4446), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4146), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ ((b << 1u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 2u) & 0xFFu) ^ ((feedback_byte + 15u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 7u) ^ (((feedback_byte + 15u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 5u), 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 99u, 13u);
    }
  }

}

static void TwistCandidate_0583_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-2557)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (6231)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-3218)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(2u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 252u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0583(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0583_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0583_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0583_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0583_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=81.916 avalanche=98.910 bic=13.281 input_mean=77.930 input_floor=73.386 input_tail=73.442 composite=53.663 candidate_id=709
// Candidate 709: TwistCandidate_0709
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x1/3x3
// mechanical_loops[l1=2x0; l2=2x1; l3=3x3; key_rot=11; twiddle=0x16091d01]
static void TwistCandidate_0709_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-4082)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (6739)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 219u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0709_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (3370)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (4156)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 22u + static_cast<unsigned int>(219u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 25u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0709_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x16091D01u ^ (pRound * 35u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-4082), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6A9C229Cu ^ static_cast<std::uint32_t>(10846u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7389), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2480), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 15u + (key_byte)) ^ (b) ^ (salt_byte ^ 12u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 6u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 182u), 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 96u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x75256B52u ^ static_cast<std::uint32_t>(4837u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5566), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-939), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 25u + (key_byte)) ^ (b) ^ (salt_byte ^ 15u) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 28u + (twiddle_byte)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 6u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 23u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 92u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8C6616AEu ^ static_cast<std::uint32_t>(4260u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5770), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1642), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-132), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 29u + ((key_byte + 18u) & 0xFFu)) ^ (c + 11u + (salt_byte ^ 18u)) ^ (b) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 5u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 18u) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 201u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 33u), 8u);
    }
  }

}

static void TwistCandidate_0709_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (6411)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-3668)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 26u + (6166)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 12u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 25u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0709(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0709_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0709_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0709_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0709_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=81.870 avalanche=99.023 bic=12.645 input_mean=81.647 input_floor=73.339 input_tail=73.359 composite=56.188 candidate_id=804
// Candidate 804: TwistCandidate_0804
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x3/3x0
// mechanical_loops[l1=2x0; l2=2x3; l3=3x0; key_rot=28; twiddle=0x72197645]
static void TwistCandidate_0804_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-1095)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-3840)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 3u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0804_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (1649)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (5466)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 24u + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 56u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0804_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x72197645u ^ (pRound * 55u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1095), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD35A4BEFu ^ static_cast<std::uint32_t>(3942u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4472), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1197), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 13u) ^ ((feedback_byte + 13u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 13u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 71u), 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 55u, 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x078278F4u ^ static_cast<std::uint32_t>(3659u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5117), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2329) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 24u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 73u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 156u, 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x033EAAA5u ^ static_cast<std::uint32_t>(4146u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3575), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2688), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3259), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ ((b << 2u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (key_byte) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b) ^ (a) ^ (salt_byte ^ 3u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 73u, 4u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 42u), 13u);
    }
  }

}

static void TwistCandidate_0804_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-7234)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-677)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 18u + (2208)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 13u + static_cast<unsigned int>(28u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 56u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0804(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0804_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0804_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0804_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0804_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=81.556 avalanche=99.156 bic=15.084 input_mean=77.484 input_floor=73.344 input_tail=73.409 composite=78.589 candidate_id=231
// Candidate 231: TwistCandidate_0231
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x1/3x2
// mechanical_loops[l1=2x0; l2=3x1; l3=3x2; key_rot=22; twiddle=0x81cdff88]
static void TwistCandidate_0231_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-635)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (1444)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 168u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0231_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (6196)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (5334)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 5u + static_cast<unsigned int>(168u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 78u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0231_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x81CDFF88u ^ (pRound * 1u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-635), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x19F9336Fu ^ static_cast<std::uint32_t>(9110u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6165), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 18u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2138), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 24u + ((key_byte + 19u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 7u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 19u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 76u, 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 42u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD82DC776u ^ static_cast<std::uint32_t>(6073u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5698), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 19u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6150), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (6525), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 20u + ((key_byte + 26u) & 0xFFu) + ((feedback_byte + 31u) & 0xFFu))) + ((c << 2u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 3u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu) ^ ((key_byte + 26u) & 0xFFu) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 218u), 2u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 22u, 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3676DEB4u ^ static_cast<std::uint32_t>(13632u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3324), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6382), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3926), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 21u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 103u), 3u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 10u, 6u);
    }
  }

}

static void TwistCandidate_0231_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (2900)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (7516)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-5882)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 22u + static_cast<unsigned int>(22u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 78u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0231(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0231_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0231_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0231_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0231_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=81.178 avalanche=99.200 bic=9.558 input_mean=85.336 input_floor=73.402 input_tail=73.441 composite=56.936 candidate_id=389
// Candidate 389: TwistCandidate_0389
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x0/3x0
// mechanical_loops[l1=2x0; l2=3x0; l3=3x0; key_rot=26; twiddle=0x302fd74c]
static void TwistCandidate_0389_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (2012)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (3133)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 221u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0389_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (1884)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (4610)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 15u + static_cast<unsigned int>(221u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 251u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0389_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x302FD74Cu ^ (pRound * 4u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2012), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCA0E4629u ^ static_cast<std::uint32_t>(9414u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5322), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4525), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 28u + (key_byte)) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 44u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 197u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC7989953u ^ static_cast<std::uint32_t>(8944u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6182), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1187) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-1575) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 6u + (salt_byte ^ 28u)) ^ ((b << 4u) & 0xFFu) ^ (c) ^ ((key_byte + 3u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 19u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 28u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 196u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 185u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x009E4B90u ^ static_cast<std::uint32_t>(4086u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3803), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4762), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5045), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a) ^ ((b << 3u) & 0xFFu) ^ (c) ^ (key_byte) ^ ((feedback_byte + 27u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 27u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 24u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 30u, 2u);
    }
  }

}

static void TwistCandidate_0389_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (1453)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-6870)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-701)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 23u + static_cast<unsigned int>(26u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(11u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 251u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0389(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0389_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0389_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0389_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0389_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.963 avalanche=99.093 bic=10.630 input_mean=77.629 input_floor=73.430 input_tail=73.433 composite=53.443 candidate_id=654
// Candidate 654: TwistCandidate_0654
// family=mechanical_loops op_budget=3 loop_shapes=3x3/2x2/3x1
// mechanical_loops[l1=3x3; l2=2x2; l3=3x1; key_rot=3; twiddle=0xaa9cc518]
static void TwistCandidate_0654_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (6103)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (7556)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 172u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0654_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-167)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-717)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 16u + static_cast<unsigned int>(172u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 191u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0654_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xAA9CC518u ^ (pRound * 59u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6103), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9645B85Eu ^ static_cast<std::uint32_t>(3651u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4380), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6110), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (1921), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c + 31u + (salt_byte)) ^ (b) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ ((a << 5u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu)) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 239u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 238u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7BEC4DA8u ^ static_cast<std::uint32_t>(3886u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4799), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5086) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 10u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte ^ 26u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 16u, 4u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 110u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x347F3A14u ^ static_cast<std::uint32_t>(6283u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3988), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4379), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (2084), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 29u + ((key_byte + 31u) & 0xFFu) + (feedback_byte))) + (c) + (salt_byte ^ 6u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c + 28u + (twiddle_byte)) ^ (a) ^ ((key_byte + 31u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 6u) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 241u, 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 184u), 5u);
    }
  }

}

static void TwistCandidate_0654_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-4009)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (4251)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-4196)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 31u + static_cast<unsigned int>(3u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 191u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0654(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0654_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0654_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0654_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0654_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.954 avalanche=99.311 bic=11.795 input_mean=73.744 input_floor=73.411 input_tail=73.428 composite=52.722 candidate_id=474
// Candidate 474: TwistCandidate_0474
// family=mechanical_loops op_budget=3 loop_shapes=1x2/2x0/3x3
// mechanical_loops[l1=1x2; l2=2x0; l3=3x3; key_rot=7; twiddle=0xe507c822]
static void TwistCandidate_0474_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-3987)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-1229)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 227u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0474_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-813)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (5982)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 30u + static_cast<unsigned int>(227u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 170u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0474_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE507C822u ^ (pRound * 54u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3987), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x906E61E6u ^ static_cast<std::uint32_t>(4370u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7535), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 24u + ((key_byte + 22u) & 0xFFu)) ^ ((a << 1u) & 0xFFu) ^ (salt_byte ^ 5u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 6u) ^ ((a >> 4u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 22u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 56u, 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 229u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF3CC5146u ^ static_cast<std::uint32_t>(3230u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5603), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2915), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 6u + (twiddle_byte)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 22u), 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 144u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xAA9A8E43u ^ static_cast<std::uint32_t>(18730u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6295), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5744), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (6691), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 3u) & 0xFFu) ^ ((a << 5u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 13u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 210u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 83u), 8u);
    }
  }

}

static void TwistCandidate_0474_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-6765)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-1005)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (83)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(7u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 2u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 170u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0474(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0474_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0474_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0474_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0474_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.653 avalanche=99.176 bic=10.397 input_mean=85.213 input_floor=73.340 input_tail=73.380 composite=56.920 candidate_id=1159
// Candidate 1159: TwistCandidate_1159
// family=mechanical_loops op_budget=3 loop_shapes=2x1/3x0/3x0
// mechanical_loops[l1=2x1; l2=3x0; l3=3x0; key_rot=7; twiddle=0xae9127cb]
static void TwistCandidate_1159_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (2596)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (1485)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 77u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1159_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-699)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (1872)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 11u + static_cast<unsigned int>(77u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 227u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1159_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xAE9127CBu ^ (pRound * 25u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2596), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8008AFE1u ^ static_cast<std::uint32_t>(3466u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-425), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 5u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2803), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 15u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 4u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 66u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 95u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x18A22652u ^ static_cast<std::uint32_t>(6538u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6128), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3739), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (3329), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 5u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 13u) & 0xFFu));
      const std::uint32_t e = (((c >> 2u) & 0xFFu) ^ (b + 29u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 13u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 13u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 199u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 49u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x029340D0u ^ static_cast<std::uint32_t>(9039u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (874), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5268), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4645), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 25u + (salt_byte)) ^ ((b << 5u) & 0xFFu) ^ (c) ^ (key_byte) ^ ((feedback_byte + 30u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 3u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 30u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 178u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 36u), 2u);
    }
  }

}

static void TwistCandidate_1159_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-7148)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (6686)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-5553)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 18u + static_cast<unsigned int>(7u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 227u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1159(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1159_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1159_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1159_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1159_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.579 avalanche=99.157 bic=12.896 input_mean=77.433 input_floor=73.356 input_tail=73.362 composite=53.425 candidate_id=247
// Candidate 247: TwistCandidate_0247
// family=mechanical_loops op_budget=3 loop_shapes=3x1/2x0/3x2
// mechanical_loops[l1=3x1; l2=2x0; l3=3x2; key_rot=18; twiddle=0x1da76578]
static void TwistCandidate_0247_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-3361)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (3355)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 82u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0247_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-379)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-4853)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 18u + static_cast<unsigned int>(82u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 9u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0247_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x1DA76578u ^ (pRound * 29u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3361), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1092D094u ^ static_cast<std::uint32_t>(13736u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4497), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2198), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (7041), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 23u + (key_byte) + ((feedback_byte + 17u) & 0xFFu))) + (c) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 14u + (twiddle_byte)) ^ (a) ^ (key_byte) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 17u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 91u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 49u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x839B35A1u ^ static_cast<std::uint32_t>(666u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5555), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 1u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3708), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 16u + (key_byte)) ^ (b) ^ (salt_byte ^ 24u) ^ ((feedback_byte + 12u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 22u + (twiddle_byte)) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 12u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 2u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 234u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1845D5A8u ^ static_cast<std::uint32_t>(2388u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6889), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4894), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4383), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ ((c >> 3u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 23u) & 0xFFu) + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 12u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 182u), 5u);
    }
  }

}

static void TwistCandidate_0247_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (6095)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (1517)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 5u + (2672)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 1u + static_cast<unsigned int>(18u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(13u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 12u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 9u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0247(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0247_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0247_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0247_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0247_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.367 avalanche=99.067 bic=10.839 input_mean=81.191 input_floor=73.381 input_tail=73.385 composite=54.282 candidate_id=1259
// Candidate 1259: TwistCandidate_1259
// family=mechanical_loops op_budget=3 loop_shapes=1x3/3x3/3x0
// mechanical_loops[l1=1x3; l2=3x3; l3=3x0; key_rot=15; twiddle=0xd478795d]
static void TwistCandidate_1259_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (580)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (3817)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 192u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1259_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-7348)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (7624)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 21u + static_cast<unsigned int>(192u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 51u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1259_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xD478795Du ^ (pRound * 4u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (580), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7019E9B8u ^ static_cast<std::uint32_t>(5217u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6779), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ (a) ^ (salt_byte ^ 3u) ^ ((feedback_byte + 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + 7u) ^ (a)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 204u), 3u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 238u, 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2F2E7A17u ^ static_cast<std::uint32_t>(9027u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (951), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2916) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-7062) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 22u + ((key_byte + 2u) & 0xFFu)) ^ (c + 24u + (salt_byte)) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 7u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 5u) & 0xFFu) ^ ((a << 4u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 7u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 2u) & 0xFFu) ^ ((feedback_byte + 7u) & 0xFFu)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 238u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 110u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x631F96CBu ^ static_cast<std::uint32_t>(166u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2211), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2991), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5368), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 3u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 30u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 30u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 49u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 124u), 3u);
    }
  }

}

static void TwistCandidate_1259_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-4169)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-7136)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 22u + (3978)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 5u + static_cast<unsigned int>(15u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 51u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1259(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1259_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1259_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1259_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1259_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.266 avalanche=99.233 bic=14.281 input_mean=73.767 input_floor=73.384 input_tail=73.385 composite=52.766 candidate_id=1445
// Candidate 1445: TwistCandidate_1445
// family=mechanical_loops op_budget=3 loop_shapes=2x3/3x3/3x1
// mechanical_loops[l1=2x3; l2=3x3; l3=3x1; key_rot=14; twiddle=0xe8e9e619]
static void TwistCandidate_1445_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6072)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (5307)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 181u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1445_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-6043)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-3793)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 28u + static_cast<unsigned int>(181u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 33u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1445_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE8E9E619u ^ (pRound * 17u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6072), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xB4C3B89Eu ^ static_cast<std::uint32_t>(7829u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (211), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2409), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 16u + (twiddle_byte)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 252u), 4u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 85u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8414F12Au ^ static_cast<std::uint32_t>(3369u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2617), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6600), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-5848), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ ((b * 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 180u), 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 136u, 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8184160Bu ^ static_cast<std::uint32_t>(12192u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (934), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5958), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5300), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 2u + ((key_byte + 30u) & 0xFFu) + ((feedback_byte + 21u) & 0xFFu))) + ((c << 1u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c + 8u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ ((key_byte + 30u) & 0xFFu) ^ ((feedback_byte + 21u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 21u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 174u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 139u, 1u);
    }
  }

}

static void TwistCandidate_1445_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (7575)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (3364)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 6u + (4745)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 3u + static_cast<unsigned int>(14u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 14u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 33u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1445(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1445_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1445_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1445_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1445_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.212 avalanche=99.223 bic=10.627 input_mean=81.792 input_floor=73.321 input_tail=73.381 composite=57.872 candidate_id=473
// Candidate 473: TwistCandidate_0473
// family=mechanical_loops op_budget=3 loop_shapes=1x1/2x3/3x0
// mechanical_loops[l1=1x1; l2=2x3; l3=3x0; key_rot=11; twiddle=0x031c64e6]
static void TwistCandidate_0473_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (1022)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-7325)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 137u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0473_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-5974)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (2177)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 13u + static_cast<unsigned int>(137u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 150u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0473_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x031C64E6u ^ (pRound * 49u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (1022), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x586F60C5u ^ static_cast<std::uint32_t>(5954u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4815), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 3u) & 0xFFu) ^ (salt_byte ^ 28u) ^ ((feedback_byte + 6u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + 15u) ^ (a)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 252u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 45u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD9381D17u ^ static_cast<std::uint32_t>(3111u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1597), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2962), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 2u) ^ ((feedback_byte + 15u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 208u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 130u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE0311CB2u ^ static_cast<std::uint32_t>(2247u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-292), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1166), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-789), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ ((b << 4u) & 0xFFu) ^ (c) ^ ((key_byte + 30u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 14u + (twiddle_byte)) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 78u, 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 76u), 7u);
    }
  }

}

static void TwistCandidate_0473_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (6779)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-3405)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-1905)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 6u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 150u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0473(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0473_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0473_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0473_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0473_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.094 avalanche=98.937 bic=11.653 input_mean=81.698 input_floor=73.374 input_tail=73.386 composite=57.848 candidate_id=249
// Candidate 249: TwistCandidate_0249
// family=mechanical_loops op_budget=3 loop_shapes=3x0/3x1/3x3
// mechanical_loops[l1=3x0; l2=3x1; l3=3x3; key_rot=1; twiddle=0xa48029ab]
static void TwistCandidate_0249_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (4173)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (1115)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 127u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0249_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (1950)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-4208)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 26u + static_cast<unsigned int>(127u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 157u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0249_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA48029ABu ^ (pRound * 16u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4173), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1CE3BED7u ^ static_cast<std::uint32_t>(4930u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4634), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5837), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (6133), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 31u + (salt_byte ^ 15u)) ^ (b) ^ (c) ^ (key_byte) ^ ((feedback_byte + 8u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 15u) ^ (((feedback_byte + 8u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 8u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 103u), 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 76u), 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x91C74D65u ^ static_cast<std::uint32_t>(5849u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2179), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4689), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-3339), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 1u + ((key_byte + 9u) & 0xFFu) + (feedback_byte))) + (c) + (salt_byte ^ 28u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 13u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 28u) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 242u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 229u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2B2FCA02u ^ static_cast<std::uint32_t>(4010u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3322), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5493), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-6181), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 4u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 24u) & 0xFFu) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 71u, 2u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 147u), 2u);
    }
  }

}

static void TwistCandidate_0249_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-1669)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (3870)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-1632)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 21u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 3u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 157u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0249(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0249_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0249_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0249_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0249_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.092 avalanche=99.136 bic=11.994 input_mean=81.619 input_floor=73.316 input_tail=73.327 composite=56.135 candidate_id=1339
// Candidate 1339: TwistCandidate_1339
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x2/3x3
// mechanical_loops[l1=3x0; l2=2x2; l3=3x3; key_rot=13; twiddle=0xd3a05377]
static void TwistCandidate_1339_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 22u + (5035)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (4684)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 91u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1339_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (3458)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (6686)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 1u + static_cast<unsigned int>(91u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 49u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1339_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xD3A05377u ^ (pRound * 39u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5035), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF33C6709u ^ static_cast<std::uint32_t>(8544u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1754), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2878), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-3912), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ ((b << 1u) & 0xFFu) ^ (c) ^ ((key_byte + 28u) & 0xFFu) ^ ((feedback_byte + 10u) & 0xFFu));
      const std::uint32_t e = (((c >> 1u) & 0xFFu) ^ (b + 25u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 15u) ^ (((feedback_byte + 10u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 10u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 28u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 109u, 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 152u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA86D7465u ^ static_cast<std::uint32_t>(10005u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5483), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (543), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 9u + ((key_byte + 13u) & 0xFFu)) ^ (b) ^ (salt_byte ^ 27u) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 4u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 4u, 9u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 155u, 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x202C0E55u ^ static_cast<std::uint32_t>(3956u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-466), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 30u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4967), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1477), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (c + 2u + (salt_byte)) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ ((a << 3u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 4u) & 0xFFu) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 36u, 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 32u), 2u);
    }
  }

}

static void TwistCandidate_1339_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-6888)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4250)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 7u + (613)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 14u + static_cast<unsigned int>(13u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 1u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 49u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1339(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1339_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1339_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1339_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1339_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=80.055 avalanche=99.193 bic=12.508 input_mean=77.412 input_floor=73.355 input_tail=73.358 composite=53.448 candidate_id=776
// Candidate 776: TwistCandidate_0776
// family=mechanical_loops op_budget=3 loop_shapes=3x3/3x3/3x0
// mechanical_loops[l1=3x3; l2=3x3; l3=3x0; key_rot=1; twiddle=0xa64b8747]
static void TwistCandidate_0776_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-5288)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (6077)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 191u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0776_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-3701)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (4392)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 28u + static_cast<unsigned int>(191u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 232u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0776_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA64B8747u ^ (pRound * 30u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5288), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8D291DA1u ^ static_cast<std::uint32_t>(8998u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2573), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3934), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-7637), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 2u) & 0xFFu) ^ (a) ^ ((c * 3u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((((d + e) ^ c ^ a ^ ((key_byte + 8u) & 0xFFu) ^ (feedback_byte)) & 0xFFu)) ^ ((key_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 151u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 87u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF91309B4u ^ static_cast<std::uint32_t>(2280u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5569), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (226), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (7623), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 26u + (key_byte)) ^ (c + 9u + (salt_byte ^ 14u)) ^ ((b * 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 3u) & 0xFFu) ^ (a) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 48u), 7u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 106u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x73AF46F4u ^ static_cast<std::uint32_t>(1908u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1404), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2508), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (2004), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ ((b << 5u) & 0xFFu) ^ (c) ^ ((key_byte + 2u) & 0xFFu) ^ ((feedback_byte + 16u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 7u) ^ (((feedback_byte + 16u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 16u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 199u), 2u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 66u), 3u);
    }
  }

}

static void TwistCandidate_0776_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (3652)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-6463)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-2682)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(1u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 232u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0776(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0776_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0776_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0776_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0776_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.900 avalanche=98.878 bic=10.671 input_mean=85.461 input_floor=73.382 input_tail=73.438 composite=55.339 candidate_id=775
// Candidate 775: TwistCandidate_0775
// family=mechanical_loops op_budget=3 loop_shapes=2x3/3x1/3x2
// mechanical_loops[l1=2x3; l2=3x1; l3=3x2; key_rot=12; twiddle=0xb39f4e66]
static void TwistCandidate_0775_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (6634)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-954)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 87u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0775_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-6171)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (5090)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 24u + static_cast<unsigned int>(87u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 149u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0775_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xB39F4E66u ^ (pRound * 30u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6634), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x84DB30D5u ^ static_cast<std::uint32_t>(13446u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4031), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6475), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte ^ 22u) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 8u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 25u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 8u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 109u, 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 178u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7106AEBDu ^ static_cast<std::uint32_t>(3274u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2564), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-867), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (1577), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 6u + ((key_byte + 13u) & 0xFFu) + (feedback_byte))) + ((c << 4u) & 0xFFu) + (salt_byte ^ 14u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c) ^ (a) ^ ((key_byte + 13u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 14u) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 234u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 8u, 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE408894Fu ^ static_cast<std::uint32_t>(2250u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2576), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1838), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-6664), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 22u ^ (key_byte)) ^ ((b << 2u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 244u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 154u), 3u);
    }
  }

}

static void TwistCandidate_0775_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (3561)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-3132)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (5967)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 28u + static_cast<unsigned int>(12u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 6u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 149u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0775(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0775_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0775_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0775_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0775_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.778 avalanche=99.166 bic=10.438 input_mean=77.495 input_floor=73.395 input_tail=73.433 composite=53.455 candidate_id=1358
// Candidate 1358: TwistCandidate_1358
// family=mechanical_loops op_budget=3 loop_shapes=1x2/3x0/3x2
// mechanical_loops[l1=1x2; l2=3x0; l3=3x2; key_rot=7; twiddle=0x6939aa5c]
static void TwistCandidate_1358_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (7495)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-7097)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 30u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1358_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-2270)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-4023)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(30u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 223u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1358_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x6939AA5Cu ^ (pRound * 45u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7495), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x90F40650u ^ static_cast<std::uint32_t>(701u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5219), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 4u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a + 25u + (key_byte)) ^ ((a << 1u) & 0xFFu) ^ (salt_byte ^ 9u) ^ ((feedback_byte + 28u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (twiddle_byte) + 24u) ^ ((a >> 5u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 148u), 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 98u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x324C9C48u ^ static_cast<std::uint32_t>(15350u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6226), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5155) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (3969) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ (c) ^ (key_byte) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 4u) & 0xFFu) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 149u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 6u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x34934EBEu ^ static_cast<std::uint32_t>(1228u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1242), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4478), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (6948), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b + 16u + (salt_byte)) ^ ((c >> 1u) & 0xFFu) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 9u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 113u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 51u), 1u);
    }
  }

}

static void TwistCandidate_1358_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (1405)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (-5746)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (464)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(7u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(29u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 223u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1358(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1358_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1358_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1358_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1358_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.726 avalanche=99.130 bic=9.210 input_mean=89.177 input_floor=73.487 input_tail=73.615 composite=57.835 candidate_id=1329
// Candidate 1329: TwistCandidate_1329
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x0/3x3
// mechanical_loops[l1=3x0; l2=2x0; l3=3x3; key_rot=10; twiddle=0x25aac8a6]
static void TwistCandidate_1329_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (7269)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-1842)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 183u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1329_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (4628)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-1879)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 29u + static_cast<unsigned int>(183u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 246u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_1329_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x25AAC8A6u ^ (pRound * 27u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (7269), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2172FA50u ^ static_cast<std::uint32_t>(1800u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3477), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5018), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (259), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 22u + (salt_byte ^ 4u)) ^ ((b << 5u) & 0xFFu) ^ (c) ^ ((key_byte + 28u) & 0xFFu) ^ ((feedback_byte + 28u) & 0xFFu));
      const std::uint32_t e = (((c >> 4u) & 0xFFu) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte ^ 4u) ^ (((feedback_byte + 28u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 28u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 130u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 251u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF1FFAB1Au ^ static_cast<std::uint32_t>(2262u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7552), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 12u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1553), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 14u + ((key_byte + 22u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 13u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 219u, 2u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 68u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x49B93D7Au ^ static_cast<std::uint32_t>(5500u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2347), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3575), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4272), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c) ^ ((b * 3u) & 0xFFu) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 11u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 75u), 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 232u), 5u);
    }
  }

}

static void TwistCandidate_1329_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-3311)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-6959)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-4247)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 25u + static_cast<unsigned int>(10u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 246u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1329(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1329_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1329_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1329_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1329_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.670 avalanche=99.193 bic=12.237 input_mean=73.864 input_floor=73.391 input_tail=73.397 composite=52.647 candidate_id=332
// Candidate 332: TwistCandidate_0332
// family=mechanical_loops op_budget=3 loop_shapes=2x0/3x1/3x2
// mechanical_loops[l1=2x0; l2=3x1; l3=3x2; key_rot=9; twiddle=0xdcc1885f]
static void TwistCandidate_0332_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (2571)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-5799)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 76u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0332_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (1092)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (3839)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 14u + static_cast<unsigned int>(76u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 119u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0332_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xDCC1885Fu ^ (pRound * 5u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2571), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1BFD0CE9u ^ static_cast<std::uint32_t>(2845u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2366), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4222), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 4u + ((key_byte + 6u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 18u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 6u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 107u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 174u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x17B18EBBu ^ static_cast<std::uint32_t>(7875u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2618), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (4173) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (6320) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) + (c) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c) ^ (a) ^ ((key_byte + 29u) & 0xFFu) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 18u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 151u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 209u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x30E271AAu ^ static_cast<std::uint32_t>(895u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5020), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2186), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3729), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 12u + (salt_byte)) ^ ((c >> 1u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 9u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 15u ^ ((key_byte + 14u) & 0xFFu)) ^ ((b << 2u) & 0xFFu) + ((feedback_byte + 9u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 14u) & 0xFFu) + ((feedback_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 26u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 243u, 7u);
    }
  }

}

static void TwistCandidate_0332_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-6668)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-955)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 9u + (-2078)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 29u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 119u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0332(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0332_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0332_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0332_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0332_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.614 avalanche=98.968 bic=12.332 input_mean=77.583 input_floor=73.375 input_tail=73.380 composite=53.615 candidate_id=736
// Candidate 736: TwistCandidate_0736
// family=mechanical_loops op_budget=3 loop_shapes=2x2/2x0/3x2
// mechanical_loops[l1=2x2; l2=2x0; l3=3x2; key_rot=20; twiddle=0xcc0ce72d]
static void TwistCandidate_0736_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (5790)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-7243)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 33u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0736_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-6593)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (117)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 5u + static_cast<unsigned int>(33u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 83u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0736_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xCC0CE72Du ^ (pRound * 63u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5790), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3A8C9542u ^ static_cast<std::uint32_t>(6537u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1052), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3246), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 22u + ((key_byte + 9u) & 0xFFu)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 28u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 30u + (twiddle_byte)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 28u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 199u), 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 71u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF80B86A1u ^ static_cast<std::uint32_t>(12045u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5470), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6878), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 24u + (key_byte)) ^ ((b << 5u) & 0xFFu) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 27u + (twiddle_byte)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 44u), 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 88u), 8u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6CA43B9Fu ^ static_cast<std::uint32_t>(6592u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7042), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6671), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (6221), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ ((c >> 2u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 20u ^ ((key_byte + 14u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 14u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 206u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 187u, 1u);
    }
  }

}

static void TwistCandidate_0736_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (2688)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-308)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 20u + (7367)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 24u + static_cast<unsigned int>(20u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 83u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0736(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0736_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0736_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0736_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0736_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.498 avalanche=99.228 bic=13.352 input_mean=77.562 input_floor=73.298 input_tail=73.386 composite=55.156 candidate_id=79
// Candidate 79: TwistCandidate_0079
// family=mechanical_loops op_budget=3 loop_shapes=3x2/2x3/3x0
// mechanical_loops[l1=3x2; l2=2x3; l3=3x0; key_rot=11; twiddle=0x1541d312]
static void TwistCandidate_0079_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-5750)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-4133)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 154u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0079_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (2878)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (3538)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 27u + static_cast<unsigned int>(154u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 4u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0079_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x1541D312u ^ (pRound * 5u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-5750), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x0C5480E8u ^ static_cast<std::uint32_t>(9563u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1219), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2208), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (6136), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 31u + (salt_byte ^ 24u)) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ ((b << 5u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((key_byte + 5u) & 0xFFu) + (feedback_byte)) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 155u), 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 14u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8919A000u ^ static_cast<std::uint32_t>(9921u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (945), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 13u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3555), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 16u + ((key_byte + 14u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 25u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (a)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 175u), 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 186u, 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x098268D0u ^ static_cast<std::uint32_t>(8883u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5388), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3607), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-7102), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 3u + (salt_byte ^ 2u)) ^ ((b << 2u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 9u) & 0xFFu) ^ ((feedback_byte + 14u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b) ^ (a) ^ (salt_byte ^ 2u) ^ (((feedback_byte + 14u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 39u, 11u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 206u), 12u);
    }
  }

}

static void TwistCandidate_0079_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (996)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4298)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-4272)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 4u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(19u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 4u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0079(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0079_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0079_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0079_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0079_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.468 avalanche=98.978 bic=13.094 input_mean=77.600 input_floor=73.359 input_tail=73.375 composite=55.228 candidate_id=306
// Candidate 306: TwistCandidate_0306
// family=mechanical_loops op_budget=3 loop_shapes=3x0/2x3/3x2
// mechanical_loops[l1=3x0; l2=2x3; l3=3x2; key_rot=14; twiddle=0x69db34d0]
static void TwistCandidate_0306_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (3598)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-5565)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 127u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_0306_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-966)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-5751)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 12u + static_cast<unsigned int>(127u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 231u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0306_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x69DB34D0u ^ (pRound * 52u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (3598), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3223B13Eu ^ static_cast<std::uint32_t>(6497u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3517), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2469), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-511), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ (c) ^ ((key_byte + 22u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu));
      const std::uint32_t e = (((c >> 3u) & 0xFFu) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 25u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + ((feedback_byte + 25u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 22u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 132u, 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 123u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF3009105u ^ static_cast<std::uint32_t>(169u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3328), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-610), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a + 8u + (twiddle_byte)) ^ ((a >> 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 237u, 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 253u), 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9F5AD004u ^ static_cast<std::uint32_t>(1774u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5887), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 26u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-746), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4859), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b) ^ ((c >> 2u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 10u ^ ((key_byte + 4u) & 0xFFu)) ^ (b) + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 4u) & 0xFFu) + ((feedback_byte + 1u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 90u, 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 22u, 7u);
    }
  }

}

static void TwistCandidate_0306_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-7181)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-208)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-2364)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 10u + static_cast<unsigned int>(14u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 4u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 231u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0306(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_0306_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0306_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0306_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0306_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// rank=79.422 avalanche=98.939 bic=11.569 input_mean=77.494 input_floor=73.425 input_tail=73.425 composite=53.553 candidate_id=1380
// Candidate 1380: TwistCandidate_1380
// family=mechanical_loops op_budget=3 loop_shapes=2x0/2x1/3x2
// mechanical_loops[l1=2x0; l2=2x1; l3=3x2; key_rot=5; twiddle=0xe27f68ce]
static void TwistCandidate_1380_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (3614)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 3u + (6984)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 79u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
    ++aKeyIndex;
    if (aKeyIndex >= kRoundKeyBytes) {
      aKeyIndex = 0U;
      ++aKeyPlaneIndex;
      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {
        aKeyPlaneIndex = 0U;
      }
    }
  }
}

static void TwistCandidate_1380_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-5551)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (5111)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(79u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 79u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_1380_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xE27F68CEu ^ (pRound * 5u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (3614), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x47918BD9u ^ static_cast<std::uint32_t>(2195u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3596), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-892), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 23u + (key_byte)) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 194u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 137u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA0F4F66Cu ^ static_cast<std::uint32_t>(8198u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (7521), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1927) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 21u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 146u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9FDF1CD7u ^ static_cast<std::uint32_t>(3070u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1169), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 10u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2324), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4225), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 24u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 24u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 126u), 2u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 100u), 2u);
    }
  }

}

static void TwistCandidate_1380_PushKeyRound(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-6055)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (7675)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 25u + (6193)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 13u + static_cast<unsigned int>(5u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(7u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 10u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 79u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_1380(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_1380_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_1380_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_1380_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_1380_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

inline constexpr ExportedCandidate kExportedTopCandidates[] = {
    {962, "TwistCandidate_0962", 95.212, 99.267, 12.730, 85.505, 73.389, 73.522, 57.161, &TwistCandidate_0962, &TwistCandidate_0962_KeySeed, &TwistCandidate_0962_SaltSeed, &TwistCandidate_0962_TwistBlock, &TwistCandidate_0962_PushKeyRound},
    {353, "TwistCandidate_0353", 94.943, 99.294, 12.631, 81.591, 73.411, 73.554, 56.231, &TwistCandidate_0353, &TwistCandidate_0353_KeySeed, &TwistCandidate_0353_SaltSeed, &TwistCandidate_0353_TwistBlock, &TwistCandidate_0353_PushKeyRound},
    {750, "TwistCandidate_0750", 92.496, 99.236, 11.110, 85.310, 73.449, 73.460, 56.727, &TwistCandidate_0750, &TwistCandidate_0750_KeySeed, &TwistCandidate_0750_SaltSeed, &TwistCandidate_0750_TwistBlock, &TwistCandidate_0750_PushKeyRound},
    {271, "TwistCandidate_0271", 90.777, 99.153, 11.058, 81.591, 73.469, 73.474, 56.139, &TwistCandidate_0271, &TwistCandidate_0271_KeySeed, &TwistCandidate_0271_SaltSeed, &TwistCandidate_0271_TwistBlock, &TwistCandidate_0271_PushKeyRound},
    {810, "TwistCandidate_0810", 90.520, 99.348, 12.246, 77.770, 73.409, 73.421, 53.509, &TwistCandidate_0810, &TwistCandidate_0810_KeySeed, &TwistCandidate_0810_SaltSeed, &TwistCandidate_0810_TwistBlock, &TwistCandidate_0810_PushKeyRound},
    {1236, "TwistCandidate_1236", 89.887, 98.954, 12.936, 81.592, 73.435, 73.740, 54.544, &TwistCandidate_1236, &TwistCandidate_1236_KeySeed, &TwistCandidate_1236_SaltSeed, &TwistCandidate_1236_TwistBlock, &TwistCandidate_1236_PushKeyRound},
    {1470, "TwistCandidate_1470", 89.689, 99.382, 10.635, 81.439, 73.489, 73.490, 56.193, &TwistCandidate_1470, &TwistCandidate_1470_KeySeed, &TwistCandidate_1470_SaltSeed, &TwistCandidate_1470_TwistBlock, &TwistCandidate_1470_PushKeyRound},
    {1225, "TwistCandidate_1225", 89.647, 99.248, 10.553, 81.725, 73.483, 73.500, 56.218, &TwistCandidate_1225, &TwistCandidate_1225_KeySeed, &TwistCandidate_1225_SaltSeed, &TwistCandidate_1225_TwistBlock, &TwistCandidate_1225_PushKeyRound},
    {71, "TwistCandidate_0071", 89.444, 99.411, 11.006, 78.070, 73.488, 73.731, 55.515, &TwistCandidate_0071, &TwistCandidate_0071_KeySeed, &TwistCandidate_0071_SaltSeed, &TwistCandidate_0071_TwistBlock, &TwistCandidate_0071_PushKeyRound},
    {527, "TwistCandidate_0527", 89.332, 99.238, 13.851, 81.313, 73.366, 73.368, 54.417, &TwistCandidate_0527, &TwistCandidate_0527_KeySeed, &TwistCandidate_0527_SaltSeed, &TwistCandidate_0527_TwistBlock, &TwistCandidate_0527_PushKeyRound},
    {496, "TwistCandidate_0496", 89.323, 99.160, 10.739, 81.606, 73.486, 73.573, 56.133, &TwistCandidate_0496, &TwistCandidate_0496_KeySeed, &TwistCandidate_0496_SaltSeed, &TwistCandidate_0496_TwistBlock, &TwistCandidate_0496_PushKeyRound},
    {109, "TwistCandidate_0109", 88.349, 99.027, 11.659, 81.638, 73.473, 73.513, 54.519, &TwistCandidate_0109, &TwistCandidate_0109_KeySeed, &TwistCandidate_0109_SaltSeed, &TwistCandidate_0109_TwistBlock, &TwistCandidate_0109_PushKeyRound},
    {160, "TwistCandidate_0160", 88.287, 99.015, 14.248, 81.486, 73.357, 73.451, 57.817, &TwistCandidate_0160, &TwistCandidate_0160_KeySeed, &TwistCandidate_0160_SaltSeed, &TwistCandidate_0160_TwistBlock, &TwistCandidate_0160_PushKeyRound},
    {1472, "TwistCandidate_1472", 87.727, 99.181, 12.331, 77.703, 73.391, 73.426, 53.612, &TwistCandidate_1472, &TwistCandidate_1472_KeySeed, &TwistCandidate_1472_SaltSeed, &TwistCandidate_1472_TwistBlock, &TwistCandidate_1472_PushKeyRound},
    {343, "TwistCandidate_0343", 87.421, 99.196, 11.197, 77.683, 73.429, 73.458, 53.538, &TwistCandidate_0343, &TwistCandidate_0343_KeySeed, &TwistCandidate_0343_SaltSeed, &TwistCandidate_0343_TwistBlock, &TwistCandidate_0343_PushKeyRound},
    {855, "TwistCandidate_0855", 86.983, 99.060, 11.629, 81.539, 73.382, 73.418, 55.961, &TwistCandidate_0855, &TwistCandidate_0855_KeySeed, &TwistCandidate_0855_SaltSeed, &TwistCandidate_0855_TwistBlock, &TwistCandidate_0855_PushKeyRound},
    {570, "TwistCandidate_0570", 86.935, 99.225, 10.271, 85.070, 73.455, 73.466, 56.922, &TwistCandidate_0570, &TwistCandidate_0570_KeySeed, &TwistCandidate_0570_SaltSeed, &TwistCandidate_0570_TwistBlock, &TwistCandidate_0570_PushKeyRound},
    {1165, "TwistCandidate_1165", 85.940, 99.179, 12.924, 77.563, 73.384, 73.395, 55.229, &TwistCandidate_1165, &TwistCandidate_1165_KeySeed, &TwistCandidate_1165_SaltSeed, &TwistCandidate_1165_TwistBlock, &TwistCandidate_1165_PushKeyRound},
    {1344, "TwistCandidate_1344", 85.899, 99.263, 10.441, 85.276, 73.386, 73.391, 56.827, &TwistCandidate_1344, &TwistCandidate_1344_KeySeed, &TwistCandidate_1344_SaltSeed, &TwistCandidate_1344_TwistBlock, &TwistCandidate_1344_PushKeyRound},
    {800, "TwistCandidate_0800", 85.775, 98.897, 12.116, 81.531, 73.446, 73.537, 54.490, &TwistCandidate_0800, &TwistCandidate_0800_KeySeed, &TwistCandidate_0800_SaltSeed, &TwistCandidate_0800_TwistBlock, &TwistCandidate_0800_PushKeyRound},
    {380, "TwistCandidate_0380", 85.693, 99.147, 11.417, 77.828, 73.384, 73.418, 53.766, &TwistCandidate_0380, &TwistCandidate_0380_KeySeed, &TwistCandidate_0380_SaltSeed, &TwistCandidate_0380_TwistBlock, &TwistCandidate_0380_PushKeyRound},
    {1200, "TwistCandidate_1200", 85.130, 98.970, 11.543, 81.593, 73.377, 73.429, 57.927, &TwistCandidate_1200, &TwistCandidate_1200_KeySeed, &TwistCandidate_1200_SaltSeed, &TwistCandidate_1200_TwistBlock, &TwistCandidate_1200_PushKeyRound},
    {1186, "TwistCandidate_1186", 85.078, 99.097, 11.086, 81.420, 73.393, 73.393, 56.037, &TwistCandidate_1186, &TwistCandidate_1186_KeySeed, &TwistCandidate_1186_SaltSeed, &TwistCandidate_1186_TwistBlock, &TwistCandidate_1186_PushKeyRound},
    {688, "TwistCandidate_0688", 84.756, 99.091, 10.112, 85.390, 73.509, 73.525, 56.980, &TwistCandidate_0688, &TwistCandidate_0688_KeySeed, &TwistCandidate_0688_SaltSeed, &TwistCandidate_0688_TwistBlock, &TwistCandidate_0688_PushKeyRound},
    {547, "TwistCandidate_0547", 84.318, 99.143, 13.829, 77.454, 73.373, 73.387, 53.382, &TwistCandidate_0547, &TwistCandidate_0547_KeySeed, &TwistCandidate_0547_SaltSeed, &TwistCandidate_0547_TwistBlock, &TwistCandidate_0547_PushKeyRound},
    {143, "TwistCandidate_0143", 84.150, 99.187, 10.142, 85.135, 73.422, 73.425, 55.076, &TwistCandidate_0143, &TwistCandidate_0143_KeySeed, &TwistCandidate_0143_SaltSeed, &TwistCandidate_0143_TwistBlock, &TwistCandidate_0143_PushKeyRound},
    {1477, "TwistCandidate_1477", 84.118, 99.296, 11.743, 81.304, 73.336, 73.353, 54.287, &TwistCandidate_1477, &TwistCandidate_1477_KeySeed, &TwistCandidate_1477_SaltSeed, &TwistCandidate_1477_TwistBlock, &TwistCandidate_1477_PushKeyRound},
    {728, "TwistCandidate_0728", 84.004, 99.143, 11.323, 77.452, 73.458, 73.459, 55.178, &TwistCandidate_0728, &TwistCandidate_0728_KeySeed, &TwistCandidate_0728_SaltSeed, &TwistCandidate_0728_TwistBlock, &TwistCandidate_0728_PushKeyRound},
    {1335, "TwistCandidate_1335", 83.760, 98.829, 12.481, 81.708, 74.016, 74.047, 56.348, &TwistCandidate_1335, &TwistCandidate_1335_KeySeed, &TwistCandidate_1335_SaltSeed, &TwistCandidate_1335_TwistBlock, &TwistCandidate_1335_PushKeyRound},
    {1071, "TwistCandidate_1071", 83.415, 99.074, 10.320, 81.562, 73.465, 73.573, 56.048, &TwistCandidate_1071, &TwistCandidate_1071_KeySeed, &TwistCandidate_1071_SaltSeed, &TwistCandidate_1071_TwistBlock, &TwistCandidate_1071_PushKeyRound},
    {1343, "TwistCandidate_1343", 83.337, 99.157, 11.830, 77.507, 73.388, 73.411, 53.461, &TwistCandidate_1343, &TwistCandidate_1343_KeySeed, &TwistCandidate_1343_SaltSeed, &TwistCandidate_1343_TwistBlock, &TwistCandidate_1343_PushKeyRound},
    {297, "TwistCandidate_0297", 83.325, 98.905, 12.351, 81.074, 73.431, 73.439, 54.167, &TwistCandidate_0297, &TwistCandidate_0297_KeySeed, &TwistCandidate_0297_SaltSeed, &TwistCandidate_0297_TwistBlock, &TwistCandidate_0297_PushKeyRound},
    {533, "TwistCandidate_0533", 83.168, 99.088, 10.394, 85.391, 73.388, 73.390, 56.816, &TwistCandidate_0533, &TwistCandidate_0533_KeySeed, &TwistCandidate_0533_SaltSeed, &TwistCandidate_0533_TwistBlock, &TwistCandidate_0533_PushKeyRound},
    {183, "TwistCandidate_0183", 83.115, 99.031, 12.847, 77.654, 73.365, 73.386, 53.712, &TwistCandidate_0183, &TwistCandidate_0183_KeySeed, &TwistCandidate_0183_SaltSeed, &TwistCandidate_0183_TwistBlock, &TwistCandidate_0183_PushKeyRound},
    {719, "TwistCandidate_0719", 83.103, 98.952, 12.146, 77.841, 73.442, 73.458, 55.457, &TwistCandidate_0719, &TwistCandidate_0719_KeySeed, &TwistCandidate_0719_SaltSeed, &TwistCandidate_0719_TwistBlock, &TwistCandidate_0719_PushKeyRound},
    {275, "TwistCandidate_0275", 83.099, 99.114, 11.313, 77.570, 73.399, 73.430, 53.533, &TwistCandidate_0275, &TwistCandidate_0275_KeySeed, &TwistCandidate_0275_SaltSeed, &TwistCandidate_0275_TwistBlock, &TwistCandidate_0275_PushKeyRound},
    {566, "TwistCandidate_0566", 82.982, 99.115, 11.012, 85.391, 73.336, 73.404, 55.142, &TwistCandidate_0566, &TwistCandidate_0566_KeySeed, &TwistCandidate_0566_SaltSeed, &TwistCandidate_0566_TwistBlock, &TwistCandidate_0566_PushKeyRound},
    {456, "TwistCandidate_0456", 82.551, 99.130, 10.197, 81.404, 73.402, 73.409, 54.519, &TwistCandidate_0456, &TwistCandidate_0456_KeySeed, &TwistCandidate_0456_SaltSeed, &TwistCandidate_0456_TwistBlock, &TwistCandidate_0456_PushKeyRound},
    {828, "TwistCandidate_0828", 82.446, 99.128, 11.633, 77.791, 73.368, 73.372, 53.682, &TwistCandidate_0828, &TwistCandidate_0828_KeySeed, &TwistCandidate_0828_SaltSeed, &TwistCandidate_0828_TwistBlock, &TwistCandidate_0828_PushKeyRound},
    {631, "TwistCandidate_0631", 82.329, 99.297, 10.526, 81.489, 73.344, 73.387, 56.001, &TwistCandidate_0631, &TwistCandidate_0631_KeySeed, &TwistCandidate_0631_SaltSeed, &TwistCandidate_0631_TwistBlock, &TwistCandidate_0631_PushKeyRound},
    {1203, "TwistCandidate_1203", 82.202, 98.891, 10.992, 81.477, 73.430, 73.445, 54.436, &TwistCandidate_1203, &TwistCandidate_1203_KeySeed, &TwistCandidate_1203_SaltSeed, &TwistCandidate_1203_TwistBlock, &TwistCandidate_1203_PushKeyRound},
    {583, "TwistCandidate_0583", 82.060, 98.895, 14.081, 81.370, 73.347, 73.407, 56.010, &TwistCandidate_0583, &TwistCandidate_0583_KeySeed, &TwistCandidate_0583_SaltSeed, &TwistCandidate_0583_TwistBlock, &TwistCandidate_0583_PushKeyRound},
    {709, "TwistCandidate_0709", 81.916, 98.910, 13.281, 77.930, 73.386, 73.442, 53.663, &TwistCandidate_0709, &TwistCandidate_0709_KeySeed, &TwistCandidate_0709_SaltSeed, &TwistCandidate_0709_TwistBlock, &TwistCandidate_0709_PushKeyRound},
    {804, "TwistCandidate_0804", 81.870, 99.023, 12.645, 81.647, 73.339, 73.359, 56.188, &TwistCandidate_0804, &TwistCandidate_0804_KeySeed, &TwistCandidate_0804_SaltSeed, &TwistCandidate_0804_TwistBlock, &TwistCandidate_0804_PushKeyRound},
    {231, "TwistCandidate_0231", 81.556, 99.156, 15.084, 77.484, 73.344, 73.409, 78.589, &TwistCandidate_0231, &TwistCandidate_0231_KeySeed, &TwistCandidate_0231_SaltSeed, &TwistCandidate_0231_TwistBlock, &TwistCandidate_0231_PushKeyRound},
    {389, "TwistCandidate_0389", 81.178, 99.200, 9.558, 85.336, 73.402, 73.441, 56.936, &TwistCandidate_0389, &TwistCandidate_0389_KeySeed, &TwistCandidate_0389_SaltSeed, &TwistCandidate_0389_TwistBlock, &TwistCandidate_0389_PushKeyRound},
    {654, "TwistCandidate_0654", 80.963, 99.093, 10.630, 77.629, 73.430, 73.433, 53.443, &TwistCandidate_0654, &TwistCandidate_0654_KeySeed, &TwistCandidate_0654_SaltSeed, &TwistCandidate_0654_TwistBlock, &TwistCandidate_0654_PushKeyRound},
    {474, "TwistCandidate_0474", 80.954, 99.311, 11.795, 73.744, 73.411, 73.428, 52.722, &TwistCandidate_0474, &TwistCandidate_0474_KeySeed, &TwistCandidate_0474_SaltSeed, &TwistCandidate_0474_TwistBlock, &TwistCandidate_0474_PushKeyRound},
    {1159, "TwistCandidate_1159", 80.653, 99.176, 10.397, 85.213, 73.340, 73.380, 56.920, &TwistCandidate_1159, &TwistCandidate_1159_KeySeed, &TwistCandidate_1159_SaltSeed, &TwistCandidate_1159_TwistBlock, &TwistCandidate_1159_PushKeyRound},
    {247, "TwistCandidate_0247", 80.579, 99.157, 12.896, 77.433, 73.356, 73.362, 53.425, &TwistCandidate_0247, &TwistCandidate_0247_KeySeed, &TwistCandidate_0247_SaltSeed, &TwistCandidate_0247_TwistBlock, &TwistCandidate_0247_PushKeyRound},
    {1259, "TwistCandidate_1259", 80.367, 99.067, 10.839, 81.191, 73.381, 73.385, 54.282, &TwistCandidate_1259, &TwistCandidate_1259_KeySeed, &TwistCandidate_1259_SaltSeed, &TwistCandidate_1259_TwistBlock, &TwistCandidate_1259_PushKeyRound},
    {1445, "TwistCandidate_1445", 80.266, 99.233, 14.281, 73.767, 73.384, 73.385, 52.766, &TwistCandidate_1445, &TwistCandidate_1445_KeySeed, &TwistCandidate_1445_SaltSeed, &TwistCandidate_1445_TwistBlock, &TwistCandidate_1445_PushKeyRound},
    {473, "TwistCandidate_0473", 80.212, 99.223, 10.627, 81.792, 73.321, 73.381, 57.872, &TwistCandidate_0473, &TwistCandidate_0473_KeySeed, &TwistCandidate_0473_SaltSeed, &TwistCandidate_0473_TwistBlock, &TwistCandidate_0473_PushKeyRound},
    {249, "TwistCandidate_0249", 80.094, 98.937, 11.653, 81.698, 73.374, 73.386, 57.848, &TwistCandidate_0249, &TwistCandidate_0249_KeySeed, &TwistCandidate_0249_SaltSeed, &TwistCandidate_0249_TwistBlock, &TwistCandidate_0249_PushKeyRound},
    {1339, "TwistCandidate_1339", 80.092, 99.136, 11.994, 81.619, 73.316, 73.327, 56.135, &TwistCandidate_1339, &TwistCandidate_1339_KeySeed, &TwistCandidate_1339_SaltSeed, &TwistCandidate_1339_TwistBlock, &TwistCandidate_1339_PushKeyRound},
    {776, "TwistCandidate_0776", 80.055, 99.193, 12.508, 77.412, 73.355, 73.358, 53.448, &TwistCandidate_0776, &TwistCandidate_0776_KeySeed, &TwistCandidate_0776_SaltSeed, &TwistCandidate_0776_TwistBlock, &TwistCandidate_0776_PushKeyRound},
    {775, "TwistCandidate_0775", 79.900, 98.878, 10.671, 85.461, 73.382, 73.438, 55.339, &TwistCandidate_0775, &TwistCandidate_0775_KeySeed, &TwistCandidate_0775_SaltSeed, &TwistCandidate_0775_TwistBlock, &TwistCandidate_0775_PushKeyRound},
    {1358, "TwistCandidate_1358", 79.778, 99.166, 10.438, 77.495, 73.395, 73.433, 53.455, &TwistCandidate_1358, &TwistCandidate_1358_KeySeed, &TwistCandidate_1358_SaltSeed, &TwistCandidate_1358_TwistBlock, &TwistCandidate_1358_PushKeyRound},
    {1329, "TwistCandidate_1329", 79.726, 99.130, 9.210, 89.177, 73.487, 73.615, 57.835, &TwistCandidate_1329, &TwistCandidate_1329_KeySeed, &TwistCandidate_1329_SaltSeed, &TwistCandidate_1329_TwistBlock, &TwistCandidate_1329_PushKeyRound},
    {332, "TwistCandidate_0332", 79.670, 99.193, 12.237, 73.864, 73.391, 73.397, 52.647, &TwistCandidate_0332, &TwistCandidate_0332_KeySeed, &TwistCandidate_0332_SaltSeed, &TwistCandidate_0332_TwistBlock, &TwistCandidate_0332_PushKeyRound},
    {736, "TwistCandidate_0736", 79.614, 98.968, 12.332, 77.583, 73.375, 73.380, 53.615, &TwistCandidate_0736, &TwistCandidate_0736_KeySeed, &TwistCandidate_0736_SaltSeed, &TwistCandidate_0736_TwistBlock, &TwistCandidate_0736_PushKeyRound},
    {79, "TwistCandidate_0079", 79.498, 99.228, 13.352, 77.562, 73.298, 73.386, 55.156, &TwistCandidate_0079, &TwistCandidate_0079_KeySeed, &TwistCandidate_0079_SaltSeed, &TwistCandidate_0079_TwistBlock, &TwistCandidate_0079_PushKeyRound},
    {306, "TwistCandidate_0306", 79.468, 98.978, 13.094, 77.600, 73.359, 73.375, 55.228, &TwistCandidate_0306, &TwistCandidate_0306_KeySeed, &TwistCandidate_0306_SaltSeed, &TwistCandidate_0306_TwistBlock, &TwistCandidate_0306_PushKeyRound},
    {1380, "TwistCandidate_1380", 79.422, 98.939, 11.569, 77.494, 73.425, 73.425, 53.553, &TwistCandidate_1380, &TwistCandidate_1380_KeySeed, &TwistCandidate_1380_SaltSeed, &TwistCandidate_1380_TwistBlock, &TwistCandidate_1380_PushKeyRound},
};
inline constexpr std::size_t kExportedTopCandidateCount = sizeof(kExportedTopCandidates) / sizeof(kExportedTopCandidates[0]);

namespace {
constexpr PhaseRecipe kEmptyPhaseRecipe{{0, 0, 0}, "", "", "", "", "", "", 0, "", 0};
}  // namespace

extern const RegisteredCandidate kRegisteredCandidates[] = {
    {962, "TwistCandidate_0962", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0962", &TwistCandidate_0962},
    {353, "TwistCandidate_0353", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0353", &TwistCandidate_0353},
    {750, "TwistCandidate_0750", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0750", &TwistCandidate_0750},
    {271, "TwistCandidate_0271", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0271", &TwistCandidate_0271},
    {810, "TwistCandidate_0810", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0810", &TwistCandidate_0810},
    {1236, "TwistCandidate_1236", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1236", &TwistCandidate_1236},
    {1470, "TwistCandidate_1470", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1470", &TwistCandidate_1470},
    {1225, "TwistCandidate_1225", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1225", &TwistCandidate_1225},
    {71, "TwistCandidate_0071", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0071", &TwistCandidate_0071},
    {527, "TwistCandidate_0527", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0527", &TwistCandidate_0527},
    {496, "TwistCandidate_0496", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0496", &TwistCandidate_0496},
    {109, "TwistCandidate_0109", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0109", &TwistCandidate_0109},
    {160, "TwistCandidate_0160", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0160", &TwistCandidate_0160},
    {1472, "TwistCandidate_1472", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1472", &TwistCandidate_1472},
    {343, "TwistCandidate_0343", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0343", &TwistCandidate_0343},
    {855, "TwistCandidate_0855", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0855", &TwistCandidate_0855},
    {570, "TwistCandidate_0570", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0570", &TwistCandidate_0570},
    {1165, "TwistCandidate_1165", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1165", &TwistCandidate_1165},
    {1344, "TwistCandidate_1344", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1344", &TwistCandidate_1344},
    {800, "TwistCandidate_0800", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0800", &TwistCandidate_0800},
    {380, "TwistCandidate_0380", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0380", &TwistCandidate_0380},
    {1200, "TwistCandidate_1200", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1200", &TwistCandidate_1200},
    {1186, "TwistCandidate_1186", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1186", &TwistCandidate_1186},
    {688, "TwistCandidate_0688", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0688", &TwistCandidate_0688},
    {547, "TwistCandidate_0547", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0547", &TwistCandidate_0547},
    {143, "TwistCandidate_0143", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0143", &TwistCandidate_0143},
    {1477, "TwistCandidate_1477", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1477", &TwistCandidate_1477},
    {728, "TwistCandidate_0728", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0728", &TwistCandidate_0728},
    {1335, "TwistCandidate_1335", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1335", &TwistCandidate_1335},
    {1071, "TwistCandidate_1071", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1071", &TwistCandidate_1071},
    {1343, "TwistCandidate_1343", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1343", &TwistCandidate_1343},
    {297, "TwistCandidate_0297", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0297", &TwistCandidate_0297},
    {533, "TwistCandidate_0533", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0533", &TwistCandidate_0533},
    {183, "TwistCandidate_0183", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0183", &TwistCandidate_0183},
    {719, "TwistCandidate_0719", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0719", &TwistCandidate_0719},
    {275, "TwistCandidate_0275", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0275", &TwistCandidate_0275},
    {566, "TwistCandidate_0566", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0566", &TwistCandidate_0566},
    {456, "TwistCandidate_0456", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0456", &TwistCandidate_0456},
    {828, "TwistCandidate_0828", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0828", &TwistCandidate_0828},
    {631, "TwistCandidate_0631", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0631", &TwistCandidate_0631},
    {1203, "TwistCandidate_1203", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1203", &TwistCandidate_1203},
    {583, "TwistCandidate_0583", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0583", &TwistCandidate_0583},
    {709, "TwistCandidate_0709", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0709", &TwistCandidate_0709},
    {804, "TwistCandidate_0804", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0804", &TwistCandidate_0804},
    {231, "TwistCandidate_0231", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0231", &TwistCandidate_0231},
    {389, "TwistCandidate_0389", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0389", &TwistCandidate_0389},
    {654, "TwistCandidate_0654", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0654", &TwistCandidate_0654},
    {474, "TwistCandidate_0474", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0474", &TwistCandidate_0474},
    {1159, "TwistCandidate_1159", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1159", &TwistCandidate_1159},
    {247, "TwistCandidate_0247", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0247", &TwistCandidate_0247},
    {1259, "TwistCandidate_1259", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1259", &TwistCandidate_1259},
    {1445, "TwistCandidate_1445", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1445", &TwistCandidate_1445},
    {473, "TwistCandidate_0473", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0473", &TwistCandidate_0473},
    {249, "TwistCandidate_0249", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0249", &TwistCandidate_0249},
    {1339, "TwistCandidate_1339", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1339", &TwistCandidate_1339},
    {776, "TwistCandidate_0776", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0776", &TwistCandidate_0776},
    {775, "TwistCandidate_0775", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0775", &TwistCandidate_0775},
    {1358, "TwistCandidate_1358", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1358", &TwistCandidate_1358},
    {1329, "TwistCandidate_1329", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1329", &TwistCandidate_1329},
    {332, "TwistCandidate_0332", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0332", &TwistCandidate_0332},
    {736, "TwistCandidate_0736", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0736", &TwistCandidate_0736},
    {79, "TwistCandidate_0079", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0079", &TwistCandidate_0079},
    {306, "TwistCandidate_0306", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_0306", &TwistCandidate_0306},
    {1380, "TwistCandidate_1380", 0, 0, 0, kEmptyPhaseRecipe, kEmptyPhaseRecipe, "TwistCandidate_1380", &TwistCandidate_1380},
};
extern const std::size_t kRegisteredCandidateCount = sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}  // namespace twist
