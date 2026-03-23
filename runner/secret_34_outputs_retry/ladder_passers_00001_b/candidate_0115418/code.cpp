#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {

// Candidate 115419: TwistCandidate_115419
// family=ferocious_dual_worker op_budget=10 worker_shapes=2x1/2x2 twiddle1=add/i twiddle2=xor/i twiddle2_source=A lane_breaker=true braid_breaker=true jump_breaker=true swap_breaker=true mask_template=0 lightning=false typhoon=true hurricane=true
// ferocious[wa=2x1; wb=2x2; tw1=add/i; tw2=xor/i; tw2src=A; lane=on; braid=on; jump=on; swap=on; mask=workerbx0; lightning=off; typhoon=on; hurricane=on; final=5; key_rot=7; mask_bias=125]
static void TwistCandidate_115419_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pSalt, 0, kSaltBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-3931)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (846)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 27u + static_cast<unsigned int>(247u)) & 31U;
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((((aA + 72u) ^ ((aB << 7u) & 0xFFu)) & 0xFFu));
    pSalt[aSaltIndex] ^= static_cast<unsigned char>(aMixValue);
    ++aSourceIndex;
  }
  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 48u + (-2847)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-238)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = static_cast<unsigned int>((aSourceIndex * 48u + pSalt[(aSourceIndex + 66u) & 31U]) & 31U);
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pSalt[(aSaltIndex + 11U) & 31U]);
    const std::uint32_t aRotated = static_cast<std::uint32_t>(((aB << 6u) | (aB >> 2u)) & 0xFFu);
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((aA ^ aRotated ^ aCarry ^ static_cast<std::uint32_t>(66u)) & 0xFFu);
    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(aMixValue ^ aCarry));
  }
}

static void TwistCandidate_115419_KeySeed(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  static constexpr unsigned char aMixBox[256] = {
      0xFEU, 0x72U, 0x9AU, 0x69U, 0x1BU, 0x51U, 0x58U, 0x30U, 0x02U, 0xE9U, 0xC2U, 0xBFU, 0x9BU, 0xD5U, 0xA8U, 0x15U,
      0x79U, 0x4CU, 0x6EU, 0xC7U, 0xE4U, 0x90U, 0xE5U, 0x8FU, 0xA7U, 0xB0U, 0xDDU, 0x85U, 0x41U, 0x19U, 0xADU, 0xC5U,
      0x3BU, 0x06U, 0xD6U, 0xDEU, 0x50U, 0x66U, 0xACU, 0xDBU, 0x1CU, 0xBDU, 0x1EU, 0xBAU, 0xE6U, 0xAEU, 0x1FU, 0xEDU,
      0x95U, 0x49U, 0xCCU, 0x12U, 0x63U, 0x55U, 0x28U, 0x96U, 0x93U, 0x27U, 0xEEU, 0x77U, 0x73U, 0x5DU, 0x43U, 0x88U,
      0x0DU, 0x5BU, 0x52U, 0xF8U, 0x09U, 0xEAU, 0x48U, 0xC1U, 0x7DU, 0xE0U, 0x98U, 0x8CU, 0x82U, 0x86U, 0xE1U, 0x00U,
      0xD7U, 0x3FU, 0xF7U, 0x2AU, 0xBBU, 0x67U, 0x1DU, 0x35U, 0xA0U, 0x4AU, 0x60U, 0x33U, 0x1AU, 0xB7U, 0x99U, 0xB9U,
      0x8BU, 0x74U, 0x01U, 0xBCU, 0xF0U, 0x6CU, 0x4FU, 0x2FU, 0xDAU, 0x4DU, 0xD4U, 0x89U, 0x8DU, 0x75U, 0xAFU, 0x5EU,
      0x7AU, 0x2CU, 0xC3U, 0x14U, 0x8AU, 0x7CU, 0xD1U, 0x9CU, 0x16U, 0x47U, 0x18U, 0x3DU, 0x56U, 0x57U, 0x37U, 0x61U,
      0x81U, 0x2EU, 0xD2U, 0xE8U, 0xC0U, 0xF3U, 0x32U, 0x44U, 0x13U, 0x38U, 0xD3U, 0x24U, 0x7FU, 0xB3U, 0xFDU, 0x3EU,
      0xB1U, 0xCFU, 0x29U, 0x78U, 0x05U, 0x0FU, 0x9FU, 0x26U, 0xEFU, 0xE7U, 0xC9U, 0xA2U, 0x7BU, 0x42U, 0xA4U, 0x3CU,
      0x92U, 0x17U, 0xA9U, 0x34U, 0xD8U, 0xFBU, 0xE2U, 0x9EU, 0xEBU, 0x94U, 0x6AU, 0x0AU, 0x0CU, 0x6DU, 0xDFU, 0x22U,
      0xF4U, 0xF5U, 0x0EU, 0xC4U, 0x83U, 0x84U, 0x59U, 0x25U, 0x4EU, 0xD0U, 0x08U, 0x36U, 0xABU, 0xA1U, 0xA6U, 0x31U,
      0x10U, 0x2DU, 0xF6U, 0xFFU, 0x03U, 0x4BU, 0x07U, 0x54U, 0x71U, 0x68U, 0x20U, 0x5CU, 0x91U, 0xF9U, 0x7EU, 0x39U,
      0x53U, 0x23U, 0x70U, 0x5AU, 0x04U, 0x45U, 0x3AU, 0xCBU, 0x97U, 0x8EU, 0xE3U, 0xCDU, 0xD9U, 0x87U, 0xF2U, 0xB4U,
      0x62U, 0x46U, 0x11U, 0xFAU, 0xECU, 0xAAU, 0xDCU, 0xA3U, 0x40U, 0xB2U, 0x6FU, 0xCAU, 0xB6U, 0xCEU, 0xBEU, 0xC6U,
      0x6BU, 0x5FU, 0xB8U, 0x9DU, 0x0BU, 0xF1U, 0xA5U, 0x64U, 0x2BU, 0x80U, 0x76U, 0xB5U, 0x21U, 0x65U, 0xFCU, 0xC8U
  };
  std::memset(pKeyStack, 0, kRoundKeyStackDepth * kRoundKeyBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aKeyIndex = 0U;
  unsigned int aKeyPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-6740)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (5349)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pKeyStack[aKeyPlaneIndex][aKeyIndex]);
    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);
    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 37u) ^ aLane ^ static_cast<std::uint32_t>(aKeyIndex * 13u) ^ static_cast<std::uint32_t>(aKeyPlaneIndex * 29u)) & 0xFFu);
    const std::uint32_t aPreMix = static_cast<std::uint32_t>((((a + 247u) ^ ((b << 7u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>(aMixBox[aPreMix]);
    pKeyStack[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(aMixValue);
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

static void TwistCandidate_115419_MaskSeed(
    unsigned char* pSource,
    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  static constexpr unsigned char aMixBox[256] = {
      0x2AU, 0x9DU, 0xADU, 0x16U, 0xD4U, 0xD1U, 0x93U, 0x9CU, 0xACU, 0xA4U, 0xECU, 0xB2U, 0x9EU, 0x80U, 0xB8U, 0xAFU,
      0xBCU, 0xDBU, 0xC3U, 0x3EU, 0xB6U, 0xB0U, 0xE5U, 0x11U, 0x33U, 0x02U, 0x05U, 0x25U, 0xFAU, 0xFBU, 0x91U, 0xB3U,
      0x6BU, 0xF4U, 0x1EU, 0x66U, 0x82U, 0x22U, 0xEEU, 0x4FU, 0xF9U, 0xB9U, 0x76U, 0x1DU, 0xA6U, 0x01U, 0xAEU, 0x12U,
      0x45U, 0xF7U, 0xA3U, 0x2EU, 0xE3U, 0x7BU, 0xC0U, 0xBEU, 0x67U, 0x43U, 0x98U, 0x99U, 0x19U, 0x6DU, 0xC1U, 0x8CU,
      0xF1U, 0x13U, 0x88U, 0xB5U, 0x30U, 0x81U, 0x28U, 0x77U, 0xA0U, 0x08U, 0x06U, 0xCDU, 0x87U, 0x38U, 0x46U, 0x40U,
      0x5EU, 0x5AU, 0xFCU, 0xB7U, 0x5CU, 0x59U, 0xBDU, 0x9FU, 0xF5U, 0x92U, 0xA7U, 0x0DU, 0x9AU, 0x3CU, 0x6EU, 0xE8U,
      0x27U, 0x94U, 0xA2U, 0x14U, 0x0FU, 0x0AU, 0x1CU, 0x6CU, 0xBBU, 0x55U, 0x18U, 0xF2U, 0x3AU, 0xC9U, 0xEAU, 0x39U,
      0x54U, 0xDEU, 0x41U, 0x07U, 0x17U, 0xCBU, 0x4DU, 0x6FU, 0x5DU, 0x85U, 0x8EU, 0x47U, 0x24U, 0xD6U, 0xDDU, 0xC2U,
      0xDAU, 0xABU, 0x7AU, 0x97U, 0x72U, 0xC4U, 0x04U, 0x00U, 0x37U, 0xE6U, 0x4BU, 0x1BU, 0x2DU, 0x95U, 0x4EU, 0xCEU,
      0x0CU, 0x74U, 0x0BU, 0x7EU, 0x7CU, 0xEBU, 0x0EU, 0x69U, 0x7DU, 0x4AU, 0xB4U, 0xD3U, 0x70U, 0x53U, 0xD8U, 0x03U,
      0x75U, 0xA1U, 0x21U, 0x3DU, 0xD0U, 0x4CU, 0x8AU, 0x49U, 0x36U, 0x34U, 0x48U, 0xA8U, 0x79U, 0x8DU, 0xCCU, 0x86U,
      0x6AU, 0x89U, 0x62U, 0x96U, 0x65U, 0x1FU, 0x35U, 0x5FU, 0xD7U, 0x83U, 0x64U, 0x5BU, 0x2FU, 0x58U, 0xF3U, 0xF0U,
      0xD9U, 0xDFU, 0x8BU, 0x44U, 0xA5U, 0x26U, 0xE2U, 0x1AU, 0x90U, 0x56U, 0xCFU, 0x57U, 0xD5U, 0x52U, 0x09U, 0xA9U,
      0xD2U, 0x7FU, 0xAAU, 0x23U, 0xDCU, 0x3FU, 0xE4U, 0xE7U, 0xFDU, 0x51U, 0x68U, 0x15U, 0xEDU, 0x2BU, 0x60U, 0x3BU,
      0x50U, 0xC6U, 0xE1U, 0xC5U, 0x71U, 0x32U, 0xFEU, 0xE9U, 0xE0U, 0x2CU, 0x29U, 0xC7U, 0x10U, 0xF8U, 0x84U, 0x78U,
      0x20U, 0x73U, 0xC8U, 0xBAU, 0xB1U, 0xBFU, 0x42U, 0x61U, 0x8FU, 0xCAU, 0xFFU, 0x9BU, 0x31U, 0xF6U, 0xEFU, 0x63U
  };
  std::memset(pMaskStack, 0, kMaskStackDepth * kMaskBytes);
  unsigned int aSourceIndex = 0U;
  unsigned int aMaskIndex = 0U;
  unsigned int aMaskPlaneIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (3571)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 8u + (-2157)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * (15u + 8u) + (-2404)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStack[aMaskPlaneIndex][aMaskIndex]);
    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);
    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 19u) ^ static_cast<std::uint32_t>(aMaskIndex * 29u) ^ static_cast<std::uint32_t>(aMaskPlaneIndex * 11u)) & 0xFFu);
    const std::uint32_t aPreMix = static_cast<std::uint32_t>((a ^ (b + 106u) ^ ((c << 2u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu);
    const std::uint32_t aMixValue = static_cast<std::uint32_t>(aMixBox[aPreMix]);
    pMaskStack[aMaskPlaneIndex][aMaskIndex] ^= static_cast<unsigned char>(aMixValue);
    ++aSourceIndex;
    ++aMaskIndex;
    if (aMaskIndex >= kMaskBytes) {
      aMaskIndex = 0U;
      ++aMaskPlaneIndex;
      if (aMaskPlaneIndex >= kMaskStackDepth) {
        aMaskPlaneIndex = 0U;
      }
    }
  }
  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 25u + (3571)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aSourceIndex * 54u + (-2157)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aSourceIndex * (25u + 54u) + (-2404)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::size_t aFlat = (static_cast<std::size_t>(aSourceIndex * 25u) + static_cast<std::size_t>(aSourceIndex * 54u) + 24u) % kMaskStackTotalBytes;
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStack[(aFlat / kMaskBytes) % kMaskStackDepth][aFlat % kMaskBytes]);
    const std::uint32_t rotated = static_cast<std::uint32_t>(((b << 4u) | (b >> 4u)) & 0xFFu);
    const std::uint32_t mix_pre = static_cast<std::uint32_t>((a ^ rotated ^ c ^ aCarry ^ static_cast<std::uint32_t>(24u)) & 0xFFu);
    const std::uint32_t aMixValue = static_cast<std::uint32_t>(aMixBox[mix_pre]);
    pMaskStack[(aFlat / kMaskBytes) % kMaskStackDepth][aFlat % kMaskBytes] ^= static_cast<unsigned char>(aMixValue ^ aCarry);
  }
}

static void TwistCandidate_115419_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddleA = static_cast<std::uint32_t>(0xA511E9B3u ^ 0x3A9AB5EDu ^ (pRound * 58u));
  aTwiddleA ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-6740), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleA ^= static_cast<std::uint32_t>(pSalt[(pRound + 0u) & 31U]) << 16U;
  std::uint32_t aTwiddleB = static_cast<std::uint32_t>(0xB44B1D93u ^ 0x3A9AB5EDu ^ (pRound * 58u));
  aTwiddleB ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5349), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleB ^= static_cast<std::uint32_t>(pSalt[(pRound + 11u) & 31U]) << 16U;
  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex1 = WrapRange(i + (-2576), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(i + (2475), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 11u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 23u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 200u + 28u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>(pMaskStack[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 11u + 3U) & 15U), static_cast<std::size_t>(i + 14u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = static_cast<std::uint32_t>((aLaneIndex ^ static_cast<std::uint32_t>(86u)) & 0xFFu);
      const int aIndex2 = WrapRange(i + (388), aStart, aEnd);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::uint32_t aRoute = ((aSourceCtrl >> 5u) ^ aKeyByte ^ aMaskByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = b; break;
        case 2U: aValue = static_cast<std::uint32_t>(a ^ b); break;
        default: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;
      }
      pWorkerA[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = RotateLeft32(aTwiddleA + ((aValue) ^ (aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias)) + static_cast<std::uint32_t>(132u), 5u);
    }
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex1 = WrapRange(i + (-3032), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(i + (2271), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 7u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 8u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 22u + 18u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>(pMaskStack[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 7u + 3U) & 15U), static_cast<std::size_t>(i + 24u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 0u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = static_cast<std::uint32_t>((aLaneIndex ^ static_cast<std::uint32_t>(177u)) & 0xFFu);
      const int aIndex2 = WrapRange(i + (-3562), aStart, aEnd);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::uint32_t aRoute = ((aSourceCtrl >> 3u) ^ aKeyByte ^ aMaskByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = b; break;
        case 2U: aValue = static_cast<std::uint32_t>(a ^ b); break;
        default: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;
      }
      pWorkerB[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = RotateLeft32(aTwiddleA ^ (aValue) ^ (aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias) ^ static_cast<std::uint32_t>(80u), 5u);
    }
  }

  // LaneBreaker
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMaskBytes) {
    const auto aWeaveSource = LoadBlockWrapped<kMaskBytes>(pSource, PASSWORD_EXPANDED_SIZE, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (3708), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto aWeaveMask = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + 215u);
    auto lane_a = LoadBlockWrapped<kMaskBytes>(pWorkerA, PASSWORD_EXPANDED_SIZE, chunk);
    auto lane_b = LoadBlockWrapped<kMaskBytes>(pWorkerB, PASSWORD_EXPANDED_SIZE, chunk);
    switch (static_cast<unsigned>(aWeaveSource[0] ^ aWeaveSource[3] ^ aWeaveMask[1]) % 4u) {
      case 0u:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aPartner = (aLane + 6u + (aWeaveSource[aLane] & 2u)) & 7U;
          if (((aWeaveSource[aLane] ^ aWeaveMask[aLane]) & 1U) != 0U) {
            const auto aTemp = lane_a[aLane];
            lane_a[aLane] = lane_b[aPartner];
            lane_b[aPartner] = aTemp;
          }
        }
        break;
      case 1u:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const unsigned aBit = static_cast<unsigned>((aWeaveSource[aLane] >> (aLane & 7U)) & 1U);
          if (aBit != 0U) {
            const std::uint8_t aMask = aWeaveMask[aLane];
            const std::uint8_t a = lane_a[aLane];
            const std::uint8_t b = lane_b[aLane];
            lane_a[aLane] = static_cast<std::uint8_t>((a & ~aMask) | (b & aMask));
            lane_b[aLane] = static_cast<std::uint8_t>((b & ~aMask) | (a & aMask));
          }
        }
        break;
      case 2u:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aPartner = (aLane + 3u + (aWeaveMask[aLane] & 3U)) & 7U;
          if (((aWeaveMask[aLane] >> (aLane & 7U)) & 1U) != 0U) {
            const std::uint8_t a = lane_a[aLane];
            const std::uint8_t b = lane_b[aPartner];
            lane_a[aLane] = static_cast<std::uint8_t>((a & 0xF0u) | (b & 0x0Fu));
            lane_b[aPartner] = static_cast<std::uint8_t>((b & 0xF0u) | (a & 0x0Fu));
          }
        }
        break;
      default:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aPartner = (aLane + 1U + ((aWeaveSource[aLane] ^ aWeaveMask[aLane]) & 3U)) & 7U;
          if (((aWeaveSource[aPartner] + aWeaveMask[aLane]) & 1U) != 0U) {
            const auto aTemp = lane_a[aPartner];
            lane_a[aPartner] = lane_b[aLane];
            lane_b[aLane] = aTemp;
          }
        }
        break;
    }
    for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
      pWorkerA[(chunk + aLane) % PASSWORD_EXPANDED_SIZE] = lane_a[aLane];
      pWorkerB[(chunk + aLane) % PASSWORD_EXPANDED_SIZE] = lane_b[aLane];
    }
  }

  // BraidBreaker
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += 32U) {
    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (6026), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const auto aSourceBlock = LoadBlockWrapped<kMaskBytes>(pSource, PASSWORD_EXPANDED_SIZE, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (1304), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto aMaskBlock = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + 372u);
    switch (static_cast<unsigned>(aSourceBlock[0] ^ aMaskBlock[0]) % 3u) {
      case 0u:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;
          if (((aSourceBlock[aLane] ^ aMaskBlock[aLane]) & 1U) != 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
      case 1u:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane + 3U) & 7U)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aMask = aMaskBlock[aLane];
          const unsigned char a = pWorkerA[aLeft];
          const unsigned char b = pWorkerB[aRight];
          pWorkerA[aLeft] = static_cast<unsigned char>((a & ~aMask) | (b & aMask));
          pWorkerB[aRight] = static_cast<unsigned char>((b & ~aMask) | (a & aMask));
        }
        break;
      default:
        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;
          if ((aSourceBlock[aLane] & 1U) != 0U) {
            const unsigned char a = pWorkerA[aLeft];
            const unsigned char b = pWorkerB[aRight];
            pWorkerA[aLeft] = static_cast<unsigned char>((a & 0xF0u) | (b & 0x0Fu));
            pWorkerB[aRight] = static_cast<unsigned char>((b & 0xF0u) | (a & 0x0Fu));
          }
        }
        break;
    }
  }

  // JumpBreaker
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += 16U) {
    const std::size_t aControlIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (5473), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (197), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const unsigned char aSourceByte = pSource[aControlIndex];
    const unsigned char aKeyByte = KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 3U + chunk) & 15U), chunk + 20U);
    const auto aMaskBlock = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + 464u);
    const unsigned char aMaskByte0 = aMaskBlock[0];
    const std::size_t aSpan = 1U + (static_cast<std::size_t>(aSourceByte ^ aKeyByte ^ aMaskByte0) % 4U);
    switch (static_cast<unsigned>(aSourceByte ^ aKeyByte) % 5u) {
      case 0u:
        break;
      case 1u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aTemp = pWorkerA[aLeft];
          pWorkerA[aLeft] = pWorkerB[aRight];
          pWorkerB[aRight] = aTemp;
        }
        break;
      case 2u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane + (aMaskByte0 & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aMask = aMaskBlock[aLane & 7U];
          const unsigned char a = pWorkerA[aLeft];
          const unsigned char b = pWorkerB[aRight];
          pWorkerA[aLeft] = static_cast<unsigned char>((a & ~aMask) | (b & aMask));
          pWorkerB[aRight] = static_cast<unsigned char>((b & ~aMask) | (a & aMask));
        }
        break;
      case 3u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane * 3U + (aSourceByte & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;
          if (((aMaskBlock[aLane & 7U] >> (aLane & 7U)) & 1U) != 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerA[aRight];
            pWorkerA[aRight] = aTemp;
          } else {
            const unsigned char aTemp = pWorkerB[aLeft];
            pWorkerB[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
      default:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane + 1U + (aKeyByte & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aMask = aMaskBlock[(aLane + aSourceByte) & 7U];
          const unsigned char a = pWorkerA[aLeft];
          const unsigned char b = pWorkerB[aRight];
          if (((aSourceByte ^ aKeyByte ^ aMask) & 1U) != 0U) {
            pWorkerA[aLeft] = static_cast<unsigned char>((a & 0xF0u) | (b & 0x0Fu));
            pWorkerB[aRight] = static_cast<unsigned char>((b & 0xF0u) | (a & 0x0Fu));
          } else {
            pWorkerA[aLeft] = ((aMask & 1U) != 0U) ? b : a;
            pWorkerB[aRight] = ((aMask & 2U) != 0U) ? a : b;
          }
        }
        break;
    }
  }

  // SwapBreaker
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += 32U) {
    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (2466), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aControlIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (1020), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const unsigned char aRuleByte = static_cast<unsigned char>(pSource[aControlIndex] ^ pSalt[(chunk / 32U + 11u) & 31U] ^ static_cast<unsigned char>(pRound));
    const std::size_t aSpan = 1U + (static_cast<std::size_t>(aRuleByte ^ pSalt[11u]) % 32U);
    switch (static_cast<unsigned>(aRuleByte) % 4u) {
      case 0u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane * 3u) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aSourceByte = pSource[(aLeft + static_cast<std::size_t>(aRuleByte)) % PASSWORD_EXPANDED_SIZE];
          const unsigned char aSaltByte = pSalt[(aLane + static_cast<std::size_t>(aRuleByte)) & 31U];
          if (((aSourceByte ^ aSaltByte) & 1U) != 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
      case 1u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane * 6u + static_cast<std::size_t>(aRuleByte)) % aSpan)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aSourceByte = pSource[(aRight + static_cast<std::size_t>(aRuleByte)) % PASSWORD_EXPANDED_SIZE];
          const unsigned char aSaltByte = pSalt[(aLane + 11u) & 31U];
          if (((aSourceByte + aSaltByte) & 3U) != 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
      case 2u:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + (aSpan - 1U - aLane)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aSourceByte = pSource[(aLeft + aRight) % PASSWORD_EXPANDED_SIZE];
          const unsigned char aSaltByte = pSalt[(aLane + 11U + static_cast<std::size_t>(aRuleByte)) & 31U];
          if (((aSourceByte ^ aSaltByte ^ aRuleByte) & 1U) != 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
      default:
        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {
          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;
          const std::size_t aRight = (aPartnerBase + ((aLane + static_cast<std::size_t>(aRuleByte)) % aSpan)) % PASSWORD_EXPANDED_SIZE;
          const unsigned char aSourceByte = pSource[(aControlIndex + aLane) % PASSWORD_EXPANDED_SIZE];
          const unsigned char aSaltByte = pSalt[(aLane + 17U + static_cast<std::size_t>(aRuleByte)) & 31U];
          if (((aSourceByte + aSaltByte + aRuleByte) & 1U) == 0U) {
            const unsigned char aTemp = pWorkerA[aLeft];
            pWorkerA[aLeft] = pWorkerB[aRight];
            pWorkerB[aRight] = aTemp;
          }
        }
        break;
    }
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex0 = WrapRange(i + (5433), aStart, aEnd);
      const int aIndex1 = WrapRange(i + (7146), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (-3090), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pWorkerB[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 6u + 1119u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>(pMaskStack[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aRoute = (c ^ aMaskByte ^ static_cast<std::uint32_t>(1u)) % 5U;
      std::uint32_t aOutA = a;
      std::uint32_t aOutB = b;
      switch ((aRoute + static_cast<std::uint32_t>(0u)) % 5U) {
        case 0U:
          aOutA = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu)));
          break;
        case 1U:
          aOutB = static_cast<std::uint32_t>((b & aMaskByte) | (a & (~aMaskByte & 0xFFu)));
          break;
        case 2U:
          if ((c & 1U) != 0U) { const std::uint32_t aTemp = aOutA; aOutA = aOutB; aOutB = aTemp; }
          break;
        case 3U:
          aOutA = static_cast<std::uint32_t>((a & 0xF0u) | (b & 0x0Fu));
          aOutB = static_cast<std::uint32_t>((b & 0xF0u) | (a & 0x0Fu));
          break;
        default:
          aOutA = ((aMaskByte & 1U) != 0U) ? b : a;
          aOutB = ((aMaskByte & 2U) != 0U) ? a : b;
          break;
      }
      pWorkerB[i] = static_cast<std::uint8_t>(aOutA);
      pWorkerA[i] = static_cast<std::uint8_t>(aOutB);
    }
  }

  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kTyphoonBlockBytes) {
    auto storm_block = LoadBlock128Wrapped(pWorkerB, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-7473), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_control = LoadBlock128Wrapped(pWorkerA, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-5040), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_mask = LoadMaskStackBlockWrapped<kTyphoonBlockBytes>(pMaskStack, chunk + 165u);
    TyphoonMatrix storm(storm_block.data());
    storm.InjectXor(storm_mask.data(), storm_mask.size(), storm_control[0] & 127U);
    storm.InjectAdd(pSalt, kSaltBytes, 18u);
    if ((static_cast<unsigned>(storm_control[3] ^ storm_mask[11] ^ pSalt[0]) % 4u) != 0u) {
    switch (static_cast<unsigned>(storm_control[3] ^ storm_mask[11] ^ pSalt[0]) % 7u) {
      case 0u: storm.ApplyFastOp(TyphoonFastOp::kXorColumnIntoColumn, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 1u: storm.ApplyFastOp(TyphoonFastOp::kAddColumnIntoColumn, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 2u: storm.ApplyFastOp(TyphoonFastOp::kRotateRowLeft, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 3u: storm.ApplyFastOp(TyphoonFastOp::kWeaveRows, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 4u: storm.ApplyFastOp(TyphoonFastOp::kWeaveColumns, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 5u: storm.ApplyFastOp(TyphoonFastOp::kSwapRows, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
      case 6u: storm.ApplyFastOp(TyphoonFastOp::kRotateRowRight, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask[23]), static_cast<std::uint8_t>(storm_control[29] + storm_mask[31])); break;
    }
    }
    if ((static_cast<unsigned>(storm_control[37] ^ storm_mask[41] ^ pSalt[1]) % 4u) != 0u) {
    switch (static_cast<unsigned>(storm_control[37] ^ storm_mask[41] ^ pSalt[1]) % 2u) {
      case 0u: storm.ApplySlowOp(TyphoonSlowOp::kTwistCross, static_cast<std::uint8_t>(storm_control[43] + storm_mask[47]), static_cast<std::uint8_t>(storm_block[53] ^ storm_mask[59])); break;
      case 1u: storm.ApplySlowOp(TyphoonSlowOp::kFlipHorizontal, static_cast<std::uint8_t>(storm_control[43] + storm_mask[47]), static_cast<std::uint8_t>(storm_block[53] ^ storm_mask[59])); break;
    }
    }
    std::array<std::uint8_t, kTyphoonBlockBytes> storm_store{};
    std::array<std::uint8_t, kTyphoonBlockBytes> storm_emit{};
    storm.Store(storm_store.data());
    for (std::size_t aLane = 0; aLane < kTyphoonBlockBytes; ++aLane) {
      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] ^ storm_control[(aLane + 7U) & 127U] ^ storm_mask[(aLane + 13U) & 127U] ^ pSalt[(aLane + 19U) & 31U]);
    }
    StoreBlock128Contiguous(pWorkerB, chunk, storm_emit);
  }

  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kHurricaneBlockBytes) {
    auto storm_block = LoadBlock256Wrapped(pWorkerA, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-1800), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_control = LoadBlock256Wrapped(pWorkerB, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-426), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_mask = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStack, chunk + 1201u);
    HurricaneMatrix storm(storm_block.data());
    storm.InjectXor(storm_mask.data(), storm_mask.size(), storm_control[0] & 0xFFU);
    storm.InjectAdd(pSalt, kSaltBytes, 5u);
    if ((static_cast<unsigned>(storm_control[13] ^ storm_mask[17] ^ pSalt[2]) % 5u) != 0u) {
    switch (static_cast<unsigned>(storm_control[13] ^ storm_mask[17] ^ pSalt[2]) % 7u) {
      case 0u: storm.ApplyFastOp(HurricaneFastOp::kAddColumnIntoColumn, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 1u: storm.ApplyFastOp(HurricaneFastOp::kRotateRowLeft, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 2u: storm.ApplyFastOp(HurricaneFastOp::kSwapRows, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 3u: storm.ApplyFastOp(HurricaneFastOp::kSwapColumns, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 4u: storm.ApplyFastOp(HurricaneFastOp::kRotateRowRight, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 5u: storm.ApplyFastOp(HurricaneFastOp::kWeaveColumns, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
      case 6u: storm.ApplyFastOp(HurricaneFastOp::kAddRowIntoRow, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask[31]), static_cast<std::uint8_t>(storm_control[43] + storm_mask[47])); break;
    }
    }
    switch (static_cast<unsigned>(storm_control[53] ^ storm_mask[59] ^ pSalt[3]) % 3u) {
      case 0u: storm.ApplySlowOp(HurricaneSlowOp::kRotateLeft, static_cast<std::uint8_t>(storm_control[61] + storm_mask[67]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask[73])); break;
      case 1u: storm.ApplySlowOp(HurricaneSlowOp::kRotateRight, static_cast<std::uint8_t>(storm_control[61] + storm_mask[67]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask[73])); break;
      case 2u: storm.ApplySlowOp(HurricaneSlowOp::kFlipVertical, static_cast<std::uint8_t>(storm_control[61] + storm_mask[67]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask[73])); break;
    }
    std::array<std::uint8_t, kHurricaneBlockBytes> storm_store{};
    std::array<std::uint8_t, kHurricaneBlockBytes> storm_emit{};
    storm.Store(storm_store.data());
    for (std::size_t aLane = 0; aLane < kHurricaneBlockBytes; ++aLane) {
      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] ^ storm_control[(aLane + 11U) & 255U] ^ storm_mask[(aLane + 17U) & 255U] ^ pSalt[aLane & 31U]);
    }
    StoreBlock256Contiguous(pWorkerA, chunk, storm_emit);
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex0 = WrapRange(i + (-7527), aStart, aEnd);
      const int aIndex1 = WrapRange(i + (-638), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (2553), aStart, aEnd);
      const int aControlIndex = WrapRange(i + (-359), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorkerB[aIndex2]);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 3u)));
      const std::uint32_t aRoute = (aSourceCtrl ^ aKeyByte ^ static_cast<std::uint32_t>(5u)) % 6U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = b; break;
        case 2U: aValue = c; break;
        case 3U: aValue = static_cast<std::uint32_t>(a ^ b); break;
        case 4U: aValue = static_cast<std::uint32_t>((b & aKeyByte) | (c & (~aKeyByte & 0xFFu))); break;
        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (b & 0x0Fu)); break;
      }
      pDest[i] = static_cast<std::uint8_t>(aValue);
    }
  }

}

static void TwistCandidate_115419_PushKeyRound(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 9u + (4527)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-4846)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 8u + (4375)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 9u + static_cast<unsigned int>(7u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(3u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 6u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 72u)));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(aMixValue));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

static void TwistCandidate_115419_PushMaskRound(
    unsigned char* pDest,
    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundMaskBuffer, 0, kMaskBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (-7357)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (5632)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 4u + (2403)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(125u)) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    pNextRoundMaskBuffer[aMaskIndex] ^= static_cast<unsigned char>(((a + c) ^ b) & 0xFFu);
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStack, pNextRoundMaskBuffer);
}

void TwistCandidate_115419(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  TwistCandidate_115419_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_115419_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_115419_MaskSeed(pSource, pMaskStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_115419_TwistBlock(aRoundSource, pWorkerA, pWorkerB, aRoundDest, aRound, aSalt, pKeyStack, pMaskStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_115419_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_115419_PushMaskRound(aRoundDest, pMaskStack, pNextRoundMaskBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

const RegisteredCandidate kRegisteredCandidates[] = {
  {
    115419,
    "TwistCandidate_115419",
    0,
    0,
    0,
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    "TwistCandidate_115419",
    &TwistCandidate_115419,
    &TwistCandidate_115419_KeySeed,
    &TwistCandidate_115419_SaltSeed,
    &TwistCandidate_115419_MaskSeed,
    &TwistCandidate_115419_TwistBlock,
    &TwistCandidate_115419_PushKeyRound,
    &TwistCandidate_115419_PushMaskRound,
  }
};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}  // namespace twist
