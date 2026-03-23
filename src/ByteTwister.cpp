#include "ByteTwister.hpp"

#include <array>
#include <cstdlib>

namespace twist {

// Exported from shard sources for ByteTwister compatibility
// HOOK POINT: START_DIGIT=0 (re-export with a different --start-digit to renumber this local series)
// selected_candidate_ids=1071,1791,2715,3358,5041,5453,5896,5979,5995,6287,6416,6533,6972,8738,9355,9598

// HOOK POINT: local export slot 0 maps shard candidate 1071 to TwistCandidate_0000.
// Candidate 1071: TwistCandidate_0000
// family=mechanical_loops op_budget=3 loop_shapes=2x1/3x3/3x1
// mechanical_loops[l1=2x1; l2=3x3; l3=3x1; key_rot=11; twiddle=0x00f55540]
static void TwistCandidate_0000_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3528)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-2638)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 26u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0000_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-3391)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-2748)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 8u + static_cast<unsigned int>(26u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 59u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0000_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x00F55540u ^ (pRound * 59u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3528), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x555384EDu ^ static_cast<std::uint32_t>(897u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3015), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 20u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-4265), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 16u) ^ ((feedback_byte + 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 2u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 12u, 10u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 160u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1519CCC3u ^ static_cast<std::uint32_t>(17611u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4783), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5733), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-7095), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 22u + ((key_byte + 24u) & 0xFFu)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (c) ^ (twiddle_byte) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 24u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 25u), 11u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 141u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xC2C76FC9u ^ static_cast<std::uint32_t>(35u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1555), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6198), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4678), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 2u + (key_byte) + (feedback_byte))) + (c) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 20u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 168u, 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 120u), 5u);
    }
  }

}

static void TwistCandidate_0000_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (3306)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-4725)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 3u + (5083)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(23u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 2u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 59u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0000(
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
  TwistCandidate_0000_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0000_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0000_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0000_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 1 maps shard candidate 1791 to TwistCandidate_0001.
// Candidate 1791: TwistCandidate_0001
// family=mechanical_loops op_budget=3 loop_shapes=3x3/3x2/3x2
// mechanical_loops[l1=3x3; l2=3x2; l3=3x2; key_rot=18; twiddle=0x0605f080]
static void TwistCandidate_0001_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-1267)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-7096)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 158u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0001_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-997)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (4284)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 1u + static_cast<unsigned int>(158u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 49u) ^ ((b << 2u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0001_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x0605F080u ^ (pRound * 34u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1267), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8963F0D7u ^ static_cast<std::uint32_t>(8427u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3768), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3074), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-1585), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 9u + ((key_byte + 3u) & 0xFFu)) ^ (c + 28u + (salt_byte ^ 13u)) ^ ((b * 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 4u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((((d + e) ^ c ^ a ^ ((key_byte + 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu)) ^ ((key_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 77u), 1u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 31u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9E826BE9u ^ static_cast<std::uint32_t>(6069u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6549), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6125), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-5645), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (b + 17u + (salt_byte ^ 28u)) ^ ((c >> 1u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 2u) & 0xFFu) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 164u, 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 175u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x8A430C48u ^ static_cast<std::uint32_t>(10027u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5218), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-426), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4383), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 194u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 1u), 7u);
    }
  }

}

static void TwistCandidate_0001_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (2111)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-1703)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-2243)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 24u + static_cast<unsigned int>(18u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(19u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 49u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0001(
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
  TwistCandidate_0001_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0001_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0001_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0001_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 2 maps shard candidate 2715 to TwistCandidate_0002.
// Candidate 2715: TwistCandidate_0002
// family=mechanical_loops op_budget=3 loop_shapes=1x3/2x0/3x0
// mechanical_loops[l1=1x3; l2=2x0; l3=3x0; key_rot=4; twiddle=0x4d0e515c]
static void TwistCandidate_0002_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-3668)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (2216)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 69u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0002_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-2257)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (6004)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 27u + static_cast<unsigned int>(69u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 239u) ^ ((b << 4u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0002_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4D0E515Cu ^ (pRound * 14u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3668), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6CD7BF99u ^ static_cast<std::uint32_t>(6349u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5077), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 21u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 4u) & 0xFFu) ^ (salt_byte ^ 9u) ^ ((feedback_byte + 1u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + 2u) ^ (a)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 190u), 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 73u, 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6131EA5Bu ^ static_cast<std::uint32_t>(2852u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6440), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 17u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 13u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2578), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a) ^ (b) ^ (salt_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 21u), 5u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 177u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x21E5EEBDu ^ static_cast<std::uint32_t>(3621u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5338), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 24u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1319), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (7640), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 15u + (salt_byte ^ 31u)) ^ ((b << 4u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 26u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 29u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 31u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 171u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 233u), 10u);
    }
  }

}

static void TwistCandidate_0002_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-1950)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (567)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (3299)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 27u + static_cast<unsigned int>(4u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 13u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 239u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0002(
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
  TwistCandidate_0002_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0002_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0002_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0002_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 3 maps shard candidate 3358 to TwistCandidate_0003.
// Candidate 3358: TwistCandidate_0003
// family=mechanical_loops op_budget=3 loop_shapes=3x1/3x3/3x2
// mechanical_loops[l1=3x1; l2=3x3; l3=3x2; key_rot=24; twiddle=0xab8d7404]
static void TwistCandidate_0003_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (4181)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-4118)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 21u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0003_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (3826)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-6590)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 3u + static_cast<unsigned int>(21u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 161u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0003_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xAB8D7404u ^ (pRound * 32u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4181), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9F740C9Du ^ static_cast<std::uint32_t>(12826u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3037), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 14u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-5600), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-4189), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 5u + ((key_byte + 5u) & 0xFFu) + (feedback_byte))) + (c) + (salt_byte ^ 6u)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (c) ^ ((a >> 2u) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte ^ 6u) ^ (feedback_byte)) & 0xFFu) ^ ((key_byte + 5u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 229u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 6u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xFD95DA8Fu ^ static_cast<std::uint32_t>(251u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3807), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5693), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2137), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 18u + (key_byte)) ^ (c + 4u + (salt_byte ^ 6u)) ^ ((b * 5u) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 3u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 65u, 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 209u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD0773C47u ^ static_cast<std::uint32_t>(9032u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5011), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3152), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (869), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 11u ^ ((key_byte + 28u) & 0xFFu)) ^ ((b << 1u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 28u) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 51u, 8u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 62u), 9u);
    }
  }

}

static void TwistCandidate_0003_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (309)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-954)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6706)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 5u + static_cast<unsigned int>(24u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(19u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 7u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 161u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0003(
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
  TwistCandidate_0003_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0003_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0003_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0003_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 4 maps shard candidate 5041 to TwistCandidate_0004.
// Candidate 5041: TwistCandidate_0004
// family=mechanical_loops op_budget=3 loop_shapes=2x2/3x1/3x3
// mechanical_loops[l1=2x2; l2=3x1; l3=3x3; key_rot=21; twiddle=0xa5e5321c]
static void TwistCandidate_0004_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-769)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-3873)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 246u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0004_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (7258)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 25u + (321)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 23u + static_cast<unsigned int>(246u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 37u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0004_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xA5E5321Cu ^ (pRound * 8u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-769), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x275254BFu ^ static_cast<std::uint32_t>(2444u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2704), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5697), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 10u + (key_byte)) ^ ((b << 1u) & 0xFFu) ^ (salt_byte ^ 4u) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a + 31u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 89u), 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 11u), 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x89F4CB03u ^ static_cast<std::uint32_t>(9804u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4556), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1561), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (6809), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 29u + (key_byte) + ((feedback_byte + 25u) & 0xFFu))) + (c) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 4u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 242u, 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 37u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x09BEE180u ^ static_cast<std::uint32_t>(9506u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4943), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 0u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 18u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 10u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1261), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3302), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 4u + ((key_byte + 23u) & 0xFFu)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 4u) & 0xFFu) ^ ((a << 1u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 23u) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 85u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 247u, 3u);
    }
  }

}

static void TwistCandidate_0004_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 20u + (6887)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (5805)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (2595)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 20u + static_cast<unsigned int>(21u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 37u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0004(
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
  TwistCandidate_0004_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0004_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0004_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0004_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 5 maps shard candidate 5453 to TwistCandidate_0005.
// Candidate 5453: TwistCandidate_0005
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x1/3x2
// mechanical_loops[l1=3x2; l2=3x1; l3=3x2; key_rot=8; twiddle=0x4ce03c41]
static void TwistCandidate_0005_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (2602)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (284)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 208u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0005_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-1762)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-7170)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 3u + static_cast<unsigned int>(208u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 171u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0005_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x4CE03C41u ^ (pRound * 42u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2602), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x62146ACCu ^ static_cast<std::uint32_t>(3209u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1295), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 25u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 16u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3654), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-5568), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ (c) ^ (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) ^ ((feedback_byte + 21u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 2u ^ ((key_byte + 12u) & 0xFFu)) ^ ((b << 5u) & 0xFFu) + ((feedback_byte + 21u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + ((key_byte + 12u) & 0xFFu) + ((feedback_byte + 21u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 12u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 204u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 172u), 1u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF282F032u ^ static_cast<std::uint32_t>(639u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1453), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 8u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (3811) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-4625) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 25u + (key_byte) + (feedback_byte))) + ((c << 5u) & 0xFFu) + (salt_byte ^ 10u)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 8u + (twiddle_byte)) ^ (a) ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 10u) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 130u, 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 49u, 10u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD5515208u ^ static_cast<std::uint32_t>(4112u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6266), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5706), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (4672), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a) ^ (b + 12u + (salt_byte)) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 25u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 125u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 25u, 13u);
    }
  }

}

static void TwistCandidate_0005_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (6925)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-4769)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 3u + (-3283)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 26u + static_cast<unsigned int>(8u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(13u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 15u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 171u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0005(
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
  TwistCandidate_0005_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0005_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0005_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0005_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 6 maps shard candidate 5896 to TwistCandidate_0006.
// Candidate 5896: TwistCandidate_0006
// family=mechanical_loops op_budget=3 loop_shapes=3x2/2x1/3x0
// mechanical_loops[l1=3x2; l2=2x1; l3=3x0; key_rot=15; twiddle=0x750c7878]
static void TwistCandidate_0006_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (3445)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (1462)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 44u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0006_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 6u + (-60)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (596)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 6u + static_cast<unsigned int>(44u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 159u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0006_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x750C7878u ^ (pRound * 9u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (3445), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x68954F9Bu ^ static_cast<std::uint32_t>(3040u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-1342), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5211), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-829), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 1u + (salt_byte)) ^ ((c >> 5u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a ^ 3u ^ (key_byte)) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 84u), 11u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 25u, 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE9D201F3u ^ static_cast<std::uint32_t>(1439u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (556), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 26u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-924) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 13u + (key_byte)) ^ ((b << 2u) & 0xFFu) ^ (salt_byte ^ 4u) ^ ((feedback_byte + 25u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 29u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 25u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 129u, 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 249u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xA11D541Cu ^ static_cast<std::uint32_t>(2449u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4586), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 30u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1461), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (3598), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 23u + (salt_byte ^ 2u)) ^ (b) ^ (c) ^ ((key_byte + 29u) & 0xFFu) ^ ((feedback_byte + 3u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 2u) ^ (((feedback_byte + 3u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 3u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 77u), 10u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 46u), 5u);
    }
  }

}

static void TwistCandidate_0006_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-7106)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (2863)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 27u + (3341)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 10u + static_cast<unsigned int>(15u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 1u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 159u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0006(
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
  TwistCandidate_0006_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0006_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0006_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0006_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 7 maps shard candidate 5979 to TwistCandidate_0007.
// Candidate 5979: TwistCandidate_0007
// family=mechanical_loops op_budget=3 loop_shapes=2x3/3x2/3x0
// mechanical_loops[l1=2x3; l2=3x2; l3=3x0; key_rot=8; twiddle=0xd900dc51]
static void TwistCandidate_0007_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (2492)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (4533)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 212u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0007_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 3u + (5665)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-6119)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 3u + static_cast<unsigned int>(212u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 183u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0007_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xD900DC51u ^ (pRound * 52u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2492), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3A4A538Cu ^ static_cast<std::uint32_t>(5234u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (236), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 5u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2540), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const std::uint32_t d = (((a + 20u + ((key_byte + 18u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) ^ (salt_byte ^ 16u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 24u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a >> 2u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (feedback_byte)) & 0xFFu) ^ ((key_byte + 18u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 3u, 9u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 116u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x714ECC26u ^ static_cast<std::uint32_t>(1014u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6527), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2571), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2942), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b) ^ ((c >> 1u) & 0xFFu) ^ (twiddle_byte) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ ((b << 4u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 166u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 139u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x1C33D84Cu ^ static_cast<std::uint32_t>(4616u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5331), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 23u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-931), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (216), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 18u + (salt_byte)) ^ ((b << 4u) & 0xFFu) ^ (c) ^ ((key_byte + 17u) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu));
      const std::uint32_t e = (((c >> 1u) & 0xFFu) ^ (b + 26u + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 5u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 5u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 118u, 7u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 121u, 2u);
    }
  }

}

static void TwistCandidate_0007_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (2857)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (6119)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-1021)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 30u + static_cast<unsigned int>(8u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 183u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0007(
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
  TwistCandidate_0007_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0007_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0007_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0007_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 8 maps shard candidate 5995 to TwistCandidate_0008.
// Candidate 5995: TwistCandidate_0008
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x3/3x0
// mechanical_loops[l1=3x2; l2=3x3; l3=3x0; key_rot=31; twiddle=0x2ff91a63]
static void TwistCandidate_0008_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (465)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-7044)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 238u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0008_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-3571)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (7398)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 12u + static_cast<unsigned int>(238u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 171u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0008_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x2FF91A63u ^ (pRound * 23u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (465), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7A8D1547u ^ static_cast<std::uint32_t>(4506u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6893), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 3u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2149), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-4536), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 25u + (salt_byte)) ^ ((c >> 3u) & 0xFFu) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a) ^ ((b << 4u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 46u), 5u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 211u, 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x6BE64543u ^ static_cast<std::uint32_t>(2368u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2844), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5974) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-762) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (c + 12u + (salt_byte ^ 11u)) ^ ((b * 3u) & 0xFFu) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ (a) ^ ((c * 5u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 20u) & 0xFFu) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 95u), 3u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 38u, 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF0FB87EFu ^ static_cast<std::uint32_t>(6653u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (5229), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5998), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-4574), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 5u) & 0xFFu) ^ (key_byte) ^ ((feedback_byte + 11u) & 0xFFu));
      const std::uint32_t e = (((c >> 3u) & 0xFFu) ^ (b + 21u + (twiddle_byte)) ^ (a) ^ (salt_byte) ^ (((feedback_byte + 11u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 11u) & 0xFFu)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 95u, 4u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 136u, 4u);
    }
  }

}

static void TwistCandidate_0008_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (487)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (4385)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-3028)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 7u + static_cast<unsigned int>(31u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 0u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 171u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0008(
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
  TwistCandidate_0008_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0008_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0008_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0008_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 9 maps shard candidate 6287 to TwistCandidate_0009.
// Candidate 6287: TwistCandidate_0009
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x1/3x2
// mechanical_loops[l1=3x2; l2=3x1; l3=3x2; key_rot=23; twiddle=0x81911fd7]
static void TwistCandidate_0009_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (-3595)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (4388)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 100u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0009_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-3752)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (4973)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 2u + static_cast<unsigned int>(100u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 98u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0009_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x81911FD7u ^ (pRound * 33u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3595), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x422F89C3u ^ static_cast<std::uint32_t>(8803u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2601), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 11u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5926), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (5478), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 5u) & 0xFFu) ^ (b + 11u + (salt_byte)) ^ ((c >> 4u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ ((b << 4u) & 0xFFu) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 187u, 8u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 123u), 12u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x2B947AC7u ^ static_cast<std::uint32_t>(6923u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-5211), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2942), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (1230), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a ^ (b + 21u + ((key_byte + 2u) & 0xFFu) + (feedback_byte))) + ((c << 2u) & 0xFFu) + (salt_byte ^ 10u)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (c + 18u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ ((key_byte + 2u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((d + e) ^ b ^ c ^ (salt_byte ^ 10u) ^ (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 198u), 1u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 18u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xDEC3FE5Cu ^ static_cast<std::uint32_t>(1393u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3008), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6331), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (1930), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 5u) & 0xFFu) + (a) ^ (b) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + (key_byte) + ((feedback_byte + 14u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 111u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 87u), 5u);
    }
  }

}

static void TwistCandidate_0009_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (5005)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-3653)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 21u + (-6742)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 1u + static_cast<unsigned int>(23u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(9u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 98u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0009(
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
  TwistCandidate_0009_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0009_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0009_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0009_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 10 maps shard candidate 6416 to TwistCandidate_0010.
// Candidate 6416: TwistCandidate_0010
// family=mechanical_loops op_budget=3 loop_shapes=1x1/2x1/3x0
// mechanical_loops[l1=1x1; l2=2x1; l3=3x0; key_rot=2; twiddle=0xed5e20c4]
static void TwistCandidate_0010_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-2093)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-4134)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 37u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0010_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-5567)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-6534)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 11u + static_cast<unsigned int>(37u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 226u) ^ ((b << 1u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0010_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xED5E20C4u ^ (pRound * 8u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-2093), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x4019C54Eu ^ static_cast<std::uint32_t>(1912u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-6500), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t d = (((a) ^ ((a << 1u) & 0xFFu) ^ (salt_byte ^ 31u) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + 29u) ^ ((a >> 1u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 6u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 219u), 3u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 26u), 13u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD19B29F3u ^ static_cast<std::uint32_t>(2176u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2273), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 12u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3334), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 16u + (key_byte)) ^ ((b << 4u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte + 27u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 5u) & 0xFFu) ^ (a + 9u + (twiddle_byte)) ^ ((a >> 4u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 234u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 46u, 11u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF0251971u ^ static_cast<std::uint32_t>(8558u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (1467), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (6923), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (168), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 5u + (salt_byte)) ^ (b) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 11u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = (((c >> 3u) & 0xFFu) ^ (b + 23u + (twiddle_byte)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + (feedback_byte)) & 0xFFu;
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 116u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 253u, 13u);
    }
  }

}

static void TwistCandidate_0010_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (4040)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 22u + (-6048)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 24u + (6913)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 5u + static_cast<unsigned int>(2u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(27u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 14u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 226u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0010(
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
  TwistCandidate_0010_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0010_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0010_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0010_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 11 maps shard candidate 6533 to TwistCandidate_0011.
// Candidate 6533: TwistCandidate_0011
// family=mechanical_loops op_budget=3 loop_shapes=3x3/3x2/3x3
// mechanical_loops[l1=3x3; l2=3x2; l3=3x3; key_rot=9; twiddle=0xf0f6df3c]
static void TwistCandidate_0011_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-1019)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-1140)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 46u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0011_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-3480)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-7477)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 14u + static_cast<unsigned int>(46u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 13u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0011_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xF0F6DF3Cu ^ (pRound * 13u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1019), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x702C9145u ^ static_cast<std::uint32_t>(4500u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6860), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6139), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-5221), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 18u + (key_byte)) ^ (c + 18u + (salt_byte)) ^ (b) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 1u) & 0xFFu) ^ ((a << 3u) & 0xFFu) ^ ((c * 5u) & 0xFFu) ^ (twiddle_byte) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 3u) & 0xFFu)) & 0xFFu)) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 52u), 7u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 143u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xF2FD400Fu ^ static_cast<std::uint32_t>(127u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2035), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 21u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 6u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-3299), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (5207), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a) ^ (b + 13u + (salt_byte)) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((c * 3u) & 0xFFu) + (a ^ 27u ^ ((key_byte + 4u) & 0xFFu)) ^ ((b << 3u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((key_byte + 4u) & 0xFFu) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 205u, 3u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 8u), 5u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xE4D2E947u ^ static_cast<std::uint32_t>(7297u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-903), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5933), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (2267), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a + 3u + (key_byte)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (a) ^ (c) ^ (twiddle_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ ((feedback_byte + 5u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 31u, 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 182u, 1u);
    }
  }

}

static void TwistCandidate_0011_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 24u + (915)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-7186)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 19u + (2293)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 24u + static_cast<unsigned int>(9u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 10u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 13u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0011(
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
  TwistCandidate_0011_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0011_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0011_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0011_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 12 maps shard candidate 6972 to TwistCandidate_0012.
// Candidate 6972: TwistCandidate_0012
// family=mechanical_loops op_budget=3 loop_shapes=3x1/3x0/3x3
// mechanical_loops[l1=3x1; l2=3x0; l3=3x3; key_rot=24; twiddle=0xdb585824]
static void TwistCandidate_0012_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 13u + (-7229)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-3448)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 68u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0012_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3610)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (5703)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 31u + static_cast<unsigned int>(68u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 194u) ^ ((b << 6u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0012_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xDB585824u ^ (pRound * 61u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-7229), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x9B22E4E5u ^ static_cast<std::uint32_t>(3868u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4220), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 5u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 9u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-2266), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-5822), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a ^ (b + 31u + ((key_byte + 13u) & 0xFFu) + ((feedback_byte + 20u) & 0xFFu))) + ((c << 4u) & 0xFFu) + (salt_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ (c + 14u + (twiddle_byte)) ^ ((a >> 1u) & 0xFFu) ^ ((key_byte + 13u) & 0xFFu) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((((d + e) ^ b ^ c ^ (salt_byte) ^ ((feedback_byte + 20u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 13u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 154u), 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 237u), 6u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x420DB766u ^ static_cast<std::uint32_t>(4045u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4476), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 0u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 24u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7249), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-6818), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 3u + (salt_byte)) ^ ((b << 4u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ ((key_byte + 10u) & 0xFFu) ^ ((feedback_byte + 15u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b + 26u + (twiddle_byte)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 15u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 15u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 49u), 10u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 67u, 3u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x427F8B61u ^ static_cast<std::uint32_t>(15200u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (6290), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 31u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5883), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (3027), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (c + 23u + (salt_byte ^ 27u)) ^ (b) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((b) ^ ((a << 5u) & 0xFFu) ^ ((c * 3u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 140u, 2u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 251u, 8u);
    }
  }

}

static void TwistCandidate_0012_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 14u + (5786)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (7245)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-7310)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 14u + static_cast<unsigned int>(24u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(15u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 9u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 194u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0012(
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
  TwistCandidate_0012_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0012_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0012_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0012_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 13 maps shard candidate 8738 to TwistCandidate_0013.
// Candidate 8738: TwistCandidate_0013
// family=mechanical_loops op_budget=3 loop_shapes=3x0/3x3/3x0
// mechanical_loops[l1=3x0; l2=3x3; l3=3x0; key_rot=30; twiddle=0x2a6a9ca2]
static void TwistCandidate_0013_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (5419)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-6669)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 43u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0013_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (7067)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (7539)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 12u + static_cast<unsigned int>(43u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 90u) ^ ((b << 3u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0013_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x2A6A9CA2u ^ (pRound * 5u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5419), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x3E1CD952u ^ static_cast<std::uint32_t>(15713u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (3361), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 2u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 4u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7106), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (5246), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a + 30u + (salt_byte)) ^ ((b << 4u) & 0xFFu) ^ (c) ^ ((key_byte + 22u) & 0xFFu) ^ ((feedback_byte + 23u) & 0xFFu));
      const std::uint32_t e = ((c) ^ (b) ^ ((a * 3u) & 0xFFu) ^ (salt_byte) ^ (((feedback_byte + 23u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((((d ^ e) + a + c + (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) + ((feedback_byte + 23u) & 0xFFu)) & 0xFFu) ^ ((key_byte + 22u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 21u), 1u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 107u, 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xFDD9BC25u ^ static_cast<std::uint32_t>(11675u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7404), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 19u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 7u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-1728), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (-2543), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = (((a + 31u + (key_byte)) ^ (c) ^ ((b * 3u) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = ((((b >> 4u) & 0xFFu) ^ (a) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ (key_byte) ^ (feedback_byte)) & 0xFFu);
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 205u), 6u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 215u), 9u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD0A37D97u ^ static_cast<std::uint32_t>(12215u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-2658), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 8u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 7u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 22u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (-6047), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-3510), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((a) ^ (b) ^ ((c * 5u) & 0xFFu) ^ ((key_byte + 3u) & 0xFFu) ^ ((feedback_byte + 27u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b + 22u + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu)) ^ (a) ^ (salt_byte ^ 31u) ^ (((feedback_byte + 27u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 3u) | (twiddle_byte >> 5u)) & 0xFFu) + ((feedback_byte + 27u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 208u, 6u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 75u, 1u);
    }
  }

}

static void TwistCandidate_0013_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-3971)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 11u + (6926)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4332)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 17u + static_cast<unsigned int>(30u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 2u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 90u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0013(
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
  TwistCandidate_0013_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0013_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0013_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0013_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 14 maps shard candidate 9355 to TwistCandidate_0014.
// Candidate 9355: TwistCandidate_0014
// family=mechanical_loops op_budget=3 loop_shapes=3x2/3x0/3x3
// mechanical_loops[l1=3x2; l2=3x0; l3=3x3; key_rot=15; twiddle=0xfa8ce5db]
static void TwistCandidate_0014_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 1u + (2343)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (3384)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 33u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0014_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (7113)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-3172)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 5u + static_cast<unsigned int>(33u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 152u) ^ ((b << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);
    ++aSourceIndex;
  }
}

static void TwistCandidate_0014_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0xFA8CE5DBu ^ (pRound * 6u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (2343), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xCD9EB2C7u ^ static_cast<std::uint32_t>(3481u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2348), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 17u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (7327), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (-6194), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = (((a) ^ (b) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a ^ 24u ^ (key_byte)) ^ ((b << 3u) & 0xFFu) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + 218u), 8u);
      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + 72u, 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x5D46B930u ^ static_cast<std::uint32_t>(17561u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (4231), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 12u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 12u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (5943) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const int index3 = WrapRange(i + (7387) - static_cast<int>(b), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pDest[index3]);
      const std::uint32_t d = ((a + 3u + (salt_byte ^ 21u)) ^ (b) ^ (c) ^ ((key_byte + 23u) & 0xFFu) ^ (feedback_byte));
      const std::uint32_t e = ((c) ^ (b + 24u + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu)) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 21u) ^ ((feedback_byte) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) + (feedback_byte)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 172u, 2u);
      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ 210u), 4u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xD28A87A1u ^ static_cast<std::uint32_t>(2732u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-4043), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 15u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 25u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (1681), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (5094), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = (((a + 13u + ((key_byte + 1u) & 0xFFu)) ^ (c) ^ (b) ^ ((feedback_byte + 19u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b >> 5u) & 0xFFu) ^ ((a << 5u) & 0xFFu) ^ (c) ^ (((twiddle_byte << 1u) | (twiddle_byte >> 7u)) & 0xFFu) ^ ((feedback_byte + 19u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = (((d + e) ^ c ^ a ^ ((key_byte + 1u) & 0xFFu) ^ ((feedback_byte + 19u) & 0xFFu)) & 0xFFu);
      pDest[i] ^= static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 102u, 5u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 101u), 9u);
    }
  }

}

static void TwistCandidate_0014_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (5004)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 16u + (2979)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-5067)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 23u + static_cast<unsigned int>(15u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(25u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 6u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 152u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0014(
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
  TwistCandidate_0014_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0014_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0014_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0014_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

// HOOK POINT: local export slot 15 maps shard candidate 9598 to TwistCandidate_0015.
// Candidate 9598: TwistCandidate_0015
// family=mechanical_loops op_budget=3 loop_shapes=3x2/2x1/3x0
// mechanical_loops[l1=3x2; l2=2x1; l3=3x0; key_rot=19; twiddle=0x7707310b]
static void TwistCandidate_0015_KeySeed(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-1423)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (3062)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 207u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
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

static void TwistCandidate_0015_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-6521)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-4199)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 5u + static_cast<unsigned int>(207u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);
    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + 176u) ^ ((b << 5u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));
    ++aSourceIndex;
  }
}

static void TwistCandidate_0015_TwistBlock(
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
  std::uint32_t aTwiddle = 0xBEEFBEEFu ^ 0x7707310Bu ^ (pRound * 58u);
  aTwiddle ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1423), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddle ^= static_cast<std::uint32_t>(pSalt[pRound & 31U]) << 16U;

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x7CA56619u ^ static_cast<std::uint32_t>(3152u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-3089), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 14u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 2u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (515), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[index2]);
      const int index3 = WrapRange(i + (5726), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[index3]);
      const std::uint32_t d = ((((a * 3u) & 0xFFu) ^ (b + 21u + (salt_byte ^ 1u)) ^ ((c >> 5u) & 0xFFu) ^ (((twiddle_byte << 2u) | (twiddle_byte >> 6u)) & 0xFFu) ^ (feedback_byte)) & 0xFFu);
      const std::uint32_t e = (((c) + (a) ^ (b) + (feedback_byte)) & 0xFFu);
      const std::uint32_t value = ((((d ^ e) + a + b + (key_byte) + (feedback_byte)) & 0xFFu) ^ (key_byte)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + 217u, 6u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 21u), 2u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0x653462C7u ^ static_cast<std::uint32_t>(1179u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (2984), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pDest[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 6u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (2005) + static_cast<int>(a), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pDest[index2]);
      const std::uint32_t d = (((a + 1u + (key_byte)) ^ (b) ^ (salt_byte) ^ ((feedback_byte + 31u) & 0xFFu)) & 0xFFu);
      const std::uint32_t e = ((((b * 3u) & 0xFFu) ^ (a) ^ ((a >> 3u) & 0xFFu)) & 0xFFu);
      const std::uint32_t value = ((d ^ e) + a + b + ((feedback_byte + 31u) & 0xFFu)) & 0xFFu;
      pWorker[i] = static_cast<std::uint8_t>(value);
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 248u), 9u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 140u), 7u);
    }
  }

  {
    const int start = 0;
    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    std::uint32_t lane_state = aTwiddle ^ 0xECB28879u ^ static_cast<std::uint32_t>(9139u);
    for (int i = start; i < end; ++i) {
      const int index1 = WrapRange(i + (-7213), start, end);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[index1]);
      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 13u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 9u)));
      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 27u) & 31U]);
      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>(          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);
      const std::uint32_t feedback_byte = static_cast<std::uint32_t>(          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);
      const int index2 = WrapRange(i + (569), start, end);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorker[index2]);
      const int index3 = WrapRange(i + (-2495), start, end);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorker[index3]);
      const std::uint32_t d = ((a + 23u + (salt_byte ^ 22u)) ^ ((b << 4u) & 0xFFu) ^ (c) ^ (key_byte) ^ ((feedback_byte + 30u) & 0xFFu));
      const std::uint32_t e = (((c >> 5u) & 0xFFu) ^ (b) ^ ((a * 5u) & 0xFFu) ^ (salt_byte ^ 22u) ^ (((feedback_byte + 30u) & 0xFFu) >> 1U));
      const std::uint32_t value = ((d ^ e) + a + c + (twiddle_byte) + ((feedback_byte + 30u) & 0xFFu)) & 0xFFu;
      pDest[i] = static_cast<std::uint8_t>(pDest[i] + static_cast<std::uint8_t>(value));
      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ 199u), 4u);
      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + 133u), 7u);
    }
  }

}

static void TwistCandidate_0015_PushKeyRound(
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
    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (-3450)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-3937)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-2345)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 28u + static_cast<unsigned int>(19u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(19u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 8u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 176u)));
    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

void TwistCandidate_0015(
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
  TwistCandidate_0015_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_0015_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_0015_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_0015_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

}  // namespace twist

namespace peanutbutter::expansion::key_expansion {

void ByteTwister::Get(unsigned char* pSource,
                      unsigned char* pWorker,
                      unsigned char* pDestination,
                      unsigned int pLength) {
  TwistBytes(mType, pSource, pWorker, pDestination, pLength);
}

void ByteTwister::SeedKey(Type pType,
                          unsigned char* pSource,
                          unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                          unsigned int pLength) {
  SeedKeyByIndex(static_cast<unsigned char>(pType), pSource, pKeyStack, pLength);
}

void ByteTwister::SeedKeyByIndex(unsigned char pType,
                                 unsigned char* pSource,
                                 unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                 unsigned int pLength) {
  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0000_KeySeed(pSource, pKeyStack, pLength); break;
    case 1u: twist::TwistCandidate_0001_KeySeed(pSource, pKeyStack, pLength); break;
    case 2u: twist::TwistCandidate_0002_KeySeed(pSource, pKeyStack, pLength); break;
    case 3u: twist::TwistCandidate_0003_KeySeed(pSource, pKeyStack, pLength); break;
    case 4u: twist::TwistCandidate_0004_KeySeed(pSource, pKeyStack, pLength); break;
    case 5u: twist::TwistCandidate_0005_KeySeed(pSource, pKeyStack, pLength); break;
    case 6u: twist::TwistCandidate_0006_KeySeed(pSource, pKeyStack, pLength); break;
    case 7u: twist::TwistCandidate_0007_KeySeed(pSource, pKeyStack, pLength); break;
    case 8u: twist::TwistCandidate_0008_KeySeed(pSource, pKeyStack, pLength); break;
    case 9u: twist::TwistCandidate_0009_KeySeed(pSource, pKeyStack, pLength); break;
    case 10u: twist::TwistCandidate_0010_KeySeed(pSource, pKeyStack, pLength); break;
    case 11u: twist::TwistCandidate_0011_KeySeed(pSource, pKeyStack, pLength); break;
    case 12u: twist::TwistCandidate_0012_KeySeed(pSource, pKeyStack, pLength); break;
    case 13u: twist::TwistCandidate_0013_KeySeed(pSource, pKeyStack, pLength); break;
    case 14u: twist::TwistCandidate_0014_KeySeed(pSource, pKeyStack, pLength); break;
    case 15u: twist::TwistCandidate_0015_KeySeed(pSource, pKeyStack, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::SeedSalt(Type pType,
                           unsigned char* pSource,
                           unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                           unsigned int pLength) {
  SeedSaltByIndex(static_cast<unsigned char>(pType), pSource, pSaltBuffer, pLength);
}

void ByteTwister::SeedSaltByIndex(unsigned char pType,
                                  unsigned char* pSource,
                                  unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                  unsigned int pLength) {
  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0000_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 1u: twist::TwistCandidate_0001_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 2u: twist::TwistCandidate_0002_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 3u: twist::TwistCandidate_0003_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 4u: twist::TwistCandidate_0004_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 5u: twist::TwistCandidate_0005_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 6u: twist::TwistCandidate_0006_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 7u: twist::TwistCandidate_0007_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 8u: twist::TwistCandidate_0008_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 9u: twist::TwistCandidate_0009_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 10u: twist::TwistCandidate_0010_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 11u: twist::TwistCandidate_0011_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 12u: twist::TwistCandidate_0012_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 13u: twist::TwistCandidate_0013_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 14u: twist::TwistCandidate_0014_SaltSeed(pSource, pSaltBuffer, pLength); break;
    case 15u: twist::TwistCandidate_0015_SaltSeed(pSource, pSaltBuffer, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::TwistBlock(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pRound,
                             const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned int pLength) {
  TwistBlockByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength);
}

void ByteTwister::TwistBlockByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pRound,
                                    const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                    unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0000_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 1u: twist::TwistCandidate_0001_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 2u: twist::TwistCandidate_0002_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 3u: twist::TwistCandidate_0003_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 4u: twist::TwistCandidate_0004_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 5u: twist::TwistCandidate_0005_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 6u: twist::TwistCandidate_0006_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 7u: twist::TwistCandidate_0007_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 8u: twist::TwistCandidate_0008_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 9u: twist::TwistCandidate_0009_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 10u: twist::TwistCandidate_0010_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 11u: twist::TwistCandidate_0011_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 12u: twist::TwistCandidate_0012_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 13u: twist::TwistCandidate_0013_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 14u: twist::TwistCandidate_0014_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    case 15u: twist::TwistCandidate_0015_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::PushKeyRound(Type pType,
                               unsigned char* pDestination,
                               const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                               unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                               unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                               unsigned int pLength) {
  PushKeyRoundByIndex(static_cast<unsigned char>(pType), pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength);
}

void ByteTwister::PushKeyRoundByIndex(unsigned char pType,
                                      unsigned char* pDestination,
                                      const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                      unsigned int pLength) {
  if (pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {
    return;
  }
  switch (pType % static_cast<unsigned char>(kTypeCount)) {
    case 0u: twist::TwistCandidate_0000_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 1u: twist::TwistCandidate_0001_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 2u: twist::TwistCandidate_0002_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 3u: twist::TwistCandidate_0003_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 4u: twist::TwistCandidate_0004_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 5u: twist::TwistCandidate_0005_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 6u: twist::TwistCandidate_0006_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 7u: twist::TwistCandidate_0007_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 8u: twist::TwistCandidate_0008_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 9u: twist::TwistCandidate_0009_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 10u: twist::TwistCandidate_0010_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 11u: twist::TwistCandidate_0011_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 12u: twist::TwistCandidate_0012_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 13u: twist::TwistCandidate_0013_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 14u: twist::TwistCandidate_0014_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    case 15u: twist::TwistCandidate_0015_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;
    default: std::abort();
  }
}

void ByteTwister::TwistBytes(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  TwistBytes(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytes(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                             unsigned int pLength) {
  TwistBytesByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pKeyStack, pNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytesByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pLength) {
  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};
  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};
  TwistBytesByIndex(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);
}

void ByteTwister::TwistBytesByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                    unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr) {
    return;
  }
  constexpr unsigned int kBlockLength = static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE);
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }

  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> aWorkspace{};
  unsigned char* aWorkspaceBuffer = (pWorker != nullptr) ? pWorker : aWorkspace.data();
  unsigned char aSaltBuffer[twist::kSaltBytes]{};

  SeedKeyByIndex(pType, pSource, pKeyStack, kBlockLength);
  SeedSaltByIndex(pType, pSource, aSaltBuffer, kBlockLength);
  for (unsigned int offset = 0U, round = 0U; offset < pLength; offset += kBlockLength, ++round) {
    unsigned char* aRoundSource = (round == 0U) ? pSource : (pDestination + offset - kBlockLength);
    unsigned char* aRoundDestination = pDestination + offset;
    TwistBlockByIndex(pType, aRoundSource, aWorkspaceBuffer, aRoundDestination, round, aSaltBuffer, pKeyStack, kBlockLength);
    PushKeyRoundByIndex(pType, aRoundDestination, aSaltBuffer, pKeyStack, pNextRoundKeyBuffer, kBlockLength);
  }
}

}  // namespace peanutbutter::expansion::key_expansion
