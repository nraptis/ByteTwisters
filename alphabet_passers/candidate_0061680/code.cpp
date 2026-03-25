#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistBreakers.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {

// Candidate 61681: TwistCandidate_61681
// family=ferocious_dual_worker op_budget=8 worker_shapes=1x4/2x3 worker_reverse=(False, True, False, False)/(True, False, False, True) twiddle1=mix/i twiddle2=mix/none twiddle2_source=B tsunami=custom breaker_shapes=aBreakerAB:4m3i3v030,aBreakerCD:4m2i3v032 lane_breaker=true braid_breaker=false jump_breaker=false swap_breaker=true mask_template=4 mask_seed_a=off mask_seed_b=add_wave lightning=false/1 typhoon=false hurricane=true final_reverse=(True, True, True, True)
// ferocious[wa=1x4; wb=2x3; tw1=mix/i; tw2=mix/none; tw2src=B; saltbox=128:00109ea7:a0e69b1f; tsunami=custom; final_whitening=custom; breaker_shapes=aBreakerAB:4m3i3v030,aBreakerCD:4m2i3v032; wa_rev=(False, True, False, False); wb_rev=(True, False, False, True); lane=on:1; braid=off:3; jump=off:0; swap=on:1; mask=workerax4; lightning=off:1; typhoon=off; hurricane=on; final=3/(True, True, True, True); key_rot=6; maskA_seed=off/True; maskB_seed=add_wave; maskA_bias=107; maskB_bias=59]
static std::uint32_t TwistCandidate_61681_AdvanceTwiddle32A(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState ^ 0x753E373Eu;
  aState = AdvanceTwiddle32(
      aState,
      pValue ^ 0xD09C8A9Fu,
      pExtra + 0x1A575F0Eu,
      0xD9F2A9E3u,
      10u);
  aState ^= RotateLeft32(pValue + pExtra + 0xCACBD591u, 11U);
  return static_cast<std::uint32_t>(aState + 0xACCC9EDDu);
}

static std::uint32_t TwistCandidate_61681_AdvanceTwiddle32B(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState + 0x1A575F0Eu;
  aState = AdvanceTwiddle32(
      aState ^ RotateLeft32(pExtra, 5U),
      pValue + 0xD9F2A9E3u,
      pExtra ^ 0x753E373Eu,
      0xD09C8A9Fu,
      10u);
  aState ^= RotateLeft32(pValue ^ pExtra ^ 0x45DAC1DDu, 11U);
  return static_cast<std::uint32_t>(aState ^ 0xC3A5F6EDu);
}

static void TwistCandidate_61681_ApplySaltMixBox(
    unsigned char (&pSalt)[kSaltBytes],
    std::uint32_t pState,
    std::uint32_t pBias,
    unsigned pRotate) {
  std::uint32_t aTwiddleA = TwistCandidate_61681_AdvanceTwiddle32A(
      pState ^ pBias,
      static_cast<std::uint32_t>(pSalt[(pBias + 3U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 17U));
  std::uint32_t aTwiddleB = TwistCandidate_61681_AdvanceTwiddle32B(
      pState + pBias + static_cast<std::uint32_t>(pRotate),
      static_cast<std::uint32_t>(pSalt[(pBias + 11U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 29U));
  for (std::size_t aIndex = 0; aIndex < kSaltBytes; ++aIndex) {
    const unsigned char aSaltA = pSalt[aIndex];
    const unsigned char aSaltB = pSalt[(aIndex + 7U + (FoldWordToByte(aTwiddleA) & 3U)) & 31U];
    const unsigned char aSaltC = pSalt[(aIndex + 13U + (FoldWordToByte(aTwiddleB) & 3U)) & 31U];
    aTwiddleA = TwistCandidate_61681_AdvanceTwiddle32A(
        aTwiddleA ^ static_cast<std::uint32_t>(aSaltB),
        static_cast<std::uint32_t>(aSaltA) ^ pBias,
        static_cast<std::uint32_t>(aSaltC) ^ static_cast<std::uint32_t>(aIndex * 17U + pRotate));
    aTwiddleB = TwistCandidate_61681_AdvanceTwiddle32B(
        aTwiddleB ^ static_cast<std::uint32_t>(aSaltC),
        static_cast<std::uint32_t>(aSaltB) + pBias + static_cast<std::uint32_t>(aIndex * 29U),
        aTwiddleA ^ static_cast<std::uint32_t>(aIndex));
    const unsigned char aWave = FixedMixBoxByte(static_cast<unsigned char>(
        FoldWordToByte(aTwiddleA ^ RotateLeft32(aTwiddleB, 3U + ((pRotate + static_cast<unsigned>(aIndex)) & 7U))) ^
        aSaltA ^
        aSaltB ^
        static_cast<unsigned char>(pBias + static_cast<std::uint32_t>(aIndex * 19U))));
    const unsigned char aMix = FixedMixBoxByte(static_cast<unsigned char>(
        aWave ^
        aSaltC ^
        static_cast<unsigned char>(FoldWordToByte(aTwiddleB) + static_cast<unsigned char>(aIndex * 7U))));
    if (((aTwiddleA >> (aIndex & 7U)) & 1U) == 0U) {
      pSalt[aIndex] = static_cast<unsigned char>(
          RotateLeft8(static_cast<std::uint8_t>(aSaltA + aWave), (pRotate + static_cast<unsigned>(aIndex)) & 7U) ^
          aMix);
    } else {
      pSalt[aIndex] = static_cast<unsigned char>(
          RotateLeft8(static_cast<std::uint8_t>(aSaltA ^ aWave), (pRotate + static_cast<unsigned>(aIndex + 3U)) & 7U) +
          aMix);
    }
    pSalt[(aIndex + 11U + (aMix & 3U)) & 31U] ^= static_cast<unsigned char>(aWave + FoldWordToByte(aTwiddleB));
  }
}

static void TwistCandidate_61681_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  static constexpr unsigned char aSaltMixBox[128] = {
      0x32U, 0xC7U, 0x1FU, 0x7BU, 0xEFU, 0xA0U, 0xDEU, 0x3FU, 0x82U, 0x9DU, 0xC3U, 0x87U, 0xD4U, 0xECU, 0xB7U, 0x2EU,
      0x02U, 0xF5U, 0xB0U, 0x12U, 0xDCU, 0xFEU, 0x2DU, 0x8FU, 0xB1U, 0x96U, 0xDAU, 0x06U, 0x78U, 0x0DU, 0xF6U, 0xCAU,
      0x3DU, 0x68U, 0xAAU, 0x2CU, 0xE9U, 0x70U, 0xAEU, 0x1DU, 0x56U, 0x38U, 0xFFU, 0x7EU, 0x4BU, 0xF0U, 0xADU, 0x08U,
      0xF8U, 0xE1U, 0xACU, 0x27U, 0x76U, 0x95U, 0x8AU, 0x37U, 0x5BU, 0x69U, 0xD0U, 0x47U, 0xF9U, 0x4EU, 0x79U, 0xE0U,
      0x3AU, 0xCCU, 0xB3U, 0x2AU, 0x6FU, 0xE6U, 0x7DU, 0xA6U, 0x88U, 0x7FU, 0x29U, 0x59U, 0xAFU, 0x8BU, 0xD3U, 0xB9U,
      0x75U, 0xE2U, 0xD1U, 0x55U, 0x03U, 0xEBU, 0xFDU, 0xBAU, 0xC2U, 0xC1U, 0xEDU, 0xF2U, 0x84U, 0x94U, 0xE3U, 0x66U,
      0x25U, 0x9AU, 0xF3U, 0xB8U, 0x7CU, 0x43U, 0x0CU, 0xDDU, 0x11U, 0xA5U, 0x05U, 0x4DU, 0xCBU, 0x62U, 0x6CU, 0x92U,
      0xC8U, 0x4FU, 0x4CU, 0x74U, 0x5CU, 0xE4U, 0x73U, 0x52U, 0x30U, 0xDFU, 0x09U, 0x8DU, 0x5DU, 0x49U, 0x0FU, 0x5FU
  };
  std::memset(pSalt, 0, kSaltBytes);
  std::uint32_t aSaltAcc = static_cast<std::uint32_t>(0xA5A5A5A5u ^ 240u ^ 197u);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-5027)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 4u + (3981)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 31u + static_cast<unsigned int>(240u)) & 31U;
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^
        static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aB & 0xFFu), 5u)) ^
        static_cast<std::uint32_t>(aSourceIndex + 240u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aSaltWave, aSaltIndex, aA ^ aSaltWave, aB, static_cast<std::uint32_t>(aSourceIndex), static_cast<std::uint32_t>(197u), 5u, false);
    pSalt[(aSaltIndex + 17U) & 31U] ^= static_cast<unsigned char>(aSaltMixBox[(
        aSaltWave ^ pSalt[(aSaltIndex + 5U) & 31U] ^ static_cast<std::uint32_t>(aSourceIndex)) & 127U]);
    ++aSourceIndex;
  }
  TwistCandidate_61681_ApplySaltMixBox(pSalt, aSaltAcc, static_cast<std::uint32_t>(197u), 5u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] ^ pSalt[(aSaltLane + 7U) & 31U] ^ static_cast<unsigned char>(197u) ^ static_cast<unsigned char>(aSaltLane)) & 127U]);
    pSalt[aSaltLane] = static_cast<unsigned char>(
        FixedMixBoxByte(static_cast<unsigned char>(pSalt[aSaltLane] ^ aSaltWave ^ pSalt[(aSaltLane + 13U) & 31U])) +
        static_cast<unsigned char>(aSaltWave));
  }
  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 37u + (-6894)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (5848)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = static_cast<unsigned int>((aSourceIndex * 37u + pSalt[(aSourceIndex + 130u) & 31U]) & 31U);
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pSalt[(aSaltIndex + 11U) & 31U]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^ aB ^ aCarry ^ static_cast<std::uint32_t>(130u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aCarry ^ aSaltWave, aSaltIndex, aA ^ aCarry, aB, static_cast<std::uint32_t>(aSourceIndex + aCarry), static_cast<std::uint32_t>(130u), 3u, false);
    pSalt[(aSaltIndex + 23U) & 31U] = static_cast<unsigned char>(pSalt[(aSaltIndex + 23U) & 31U] + static_cast<unsigned char>(aSaltWave));
  }
  TwistCandidate_61681_ApplySaltMixBox(pSalt, aSaltAcc ^ static_cast<std::uint32_t>(130u), static_cast<std::uint32_t>(130u), 3u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] + pSalt[(aSaltLane + 19U) & 31U] + static_cast<unsigned char>(130u + 13u)) & 127U]);
    pSalt[aSaltLane] ^= static_cast<unsigned char>(aSaltWave);
  }
}

static void TwistCandidate_61681_KeySeed(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 11u + (6367)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 1u + (4223)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pKeyStack[aKeyPlaneIndex][aKeyIndex]);
    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);
    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 37u) ^ aLane ^ static_cast<std::uint32_t>(aKeyIndex * 13u) ^ static_cast<std::uint32_t>(aKeyPlaneIndex * 29u)) & 0xFFu);
    const std::uint32_t aPreMix = static_cast<std::uint32_t>((((a + 240u) ^ ((b << 5u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>(aMixBox[aPreMix]);
    pKeyStack[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>(pKeyStack[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(aMixValue));
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

static void TwistCandidate_61681_MaskSeedA(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
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
  std::memset(pMaskStackA, 0, kMaskStackDepth * kMaskBytes);
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 12u ^ 107u);
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aReverseIndex * 30u + (-4463)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 5u + (-1966)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aReverseIndex * (30u + 5u) + (4091)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aReverseIndex) * 30u * 8U + static_cast<std::size_t>(aSourceIndex) * 5u + 12u + static_cast<std::size_t>(aTurbulence * ((34u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(12u), 7u, true);
  }
  for (int aLoopIndex = 0; aLoopIndex != static_cast<int>(PASSWORD_EXPANDED_SIZE); aLoopIndex += 1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-4463)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aSourceIndex * 35u + (-1966)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aSourceIndex * (26u + 35u) + (4091)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 26u * 8U + static_cast<std::size_t>(aSourceIndex) * 35u * 3U + 247u + static_cast<std::size_t>(aTurbulence * ((34u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStackA[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), 4u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>(247u), 4u, true);
  }
}

static void TwistCandidate_61681_MaskSeedB(
    unsigned char* pSource,
    unsigned char* pWorker,
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
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
  std::memset(pMaskStackB, 0, kMaskStackDepth * kMaskBytes);
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 213u ^ 59u);
  for (int aLoopIndex = 0; aLoopIndex != static_cast<int>(PASSWORD_EXPANDED_SIZE); aLoopIndex += 1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aReverseIndex * 7u + (1636)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aReverseIndex * 3u + (1660)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * (7u + 3u) + (-5567)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const int aWorkerIndex = WrapRange(static_cast<int>(aReverseIndex * 28u + (-6442)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t aWorkerByte = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;
    const std::uint32_t aTurbulence = static_cast<std::uint32_t>((aWorkerByte + b + static_cast<std::uint32_t>(225u)) & 0xFFu);
    const std::uint32_t aSeedA = static_cast<std::uint32_t>((a + aTurbulence) & 0xFFu);
    const std::uint32_t aSeedB = static_cast<std::uint32_t>((b ^ static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aTurbulence & 0xFFu), 2u))) & 0xFFu);
    const std::uint32_t aSeedC = static_cast<std::uint32_t>((c + static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aWorkerByte & 0xFFu), 2u))) & 0xFFu);
    const std::size_t aFlatBit = (static_cast<std::size_t>(aReverseIndex) * 7u * 8U + static_cast<std::size_t>(aReverseIndex) * 3u + 213u + static_cast<std::size_t>(aTurbulence * ((28u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackB, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(213u), 1u, false);
    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + 225u) * ((28u & 15U) + 1U))) % (kMaskStackTotalBytes * 8U);
    UpdateMaskSeedBit(pMaskStackB, aTurbulenceBit, aMaskSeedState ^ aTurbulence ^ aWorkerByte, 2u, false);
  }
}

static void TwistCandidate_61681_TsunamiBreaker(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned int pRound,
    std::uint32_t pTwiddleA,
    std::uint32_t pTwiddleB) {
  if (pSource == nullptr || pWorkerA == nullptr || pWorkerB == nullptr) {
    return;
  }
  static constexpr unsigned char aMixBox[128] = {
      0x87U, 0x63U, 0x85U, 0x7AU, 0x3DU, 0xFDU, 0x8AU, 0xF5U, 0x53U, 0xB3U, 0x49U, 0xE3U, 0x37U, 0xC1U, 0x2FU, 0x45U,
      0x7CU, 0xA3U, 0x50U, 0x7FU, 0xBEU, 0xCAU, 0x33U, 0xA7U, 0xA2U, 0xB8U, 0x31U, 0x5BU, 0x32U, 0x7EU, 0x6CU, 0x4CU,
      0xFAU, 0x73U, 0xB2U, 0x40U, 0x3CU, 0xBAU, 0x1EU, 0x54U, 0x1FU, 0x7DU, 0x66U, 0x1CU, 0x6EU, 0xA5U, 0xD7U, 0xC2U,
      0xAEU, 0x03U, 0x71U, 0x11U, 0xE1U, 0x75U, 0x9FU, 0xF7U, 0xCFU, 0x19U, 0xD6U, 0x28U, 0xE5U, 0x42U, 0x2AU, 0xE4U,
      0x04U, 0xFCU, 0x1BU, 0x17U, 0x29U, 0x5FU, 0xE6U, 0xB5U, 0x8DU, 0x00U, 0x07U, 0xC8U, 0x77U, 0x8BU, 0x5AU, 0xDAU,
      0x1DU, 0x57U, 0x98U, 0x65U, 0x9EU, 0x21U, 0xCDU, 0x67U, 0xD1U, 0xC6U, 0x02U, 0xAAU, 0x0FU, 0x6FU, 0x01U, 0xD8U,
      0xEAU, 0xEDU, 0xC5U, 0x81U, 0xA8U, 0x5CU, 0x8FU, 0x96U, 0x3BU, 0xACU, 0xB0U, 0x47U, 0x6DU, 0xE9U, 0x2DU, 0x2EU,
      0xBCU, 0xC9U, 0x72U, 0xB9U, 0xD2U, 0xAFU, 0x24U, 0x0EU, 0x69U, 0xB7U, 0x5DU, 0x68U, 0x4FU, 0x58U, 0xB4U, 0x4AU
  };
  std::uint32_t aAccA = AdvanceTwiddle32(0x63D964B7u ^ static_cast<std::uint32_t>(pRound), pTwiddleA, pTwiddleB, 0x5AA5D779u, 5u);
  std::uint32_t aAccB = AdvanceTwiddle32(0x2CCD7902u ^ static_cast<std::uint32_t>(pRound), pTwiddleB, pTwiddleA, 0xC9A3BC0Du, 8u);
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (3813), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aSourceIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-3889), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-1811), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (5022), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::uint32_t aSourceA = static_cast<std::uint32_t>(pSource[aSourceIndexA]);
    const std::uint32_t aSourceB = static_cast<std::uint32_t>(pSource[aSourceIndexB]);
    const std::uint32_t aTapA = static_cast<std::uint32_t>(pWorkerA[aWorkerIndexA]);
    const std::uint32_t aTapB = static_cast<std::uint32_t>(pWorkerB[aWorkerIndexB]);
    const std::uint32_t aSaltA = static_cast<std::uint32_t>(pSalt[(i + 29u) & 31U]);
    const std::uint32_t aSaltB = static_cast<std::uint32_t>(pSalt[(i + 15u) & 31U]);
    const std::uint32_t aBoxA = static_cast<std::uint32_t>(aMixBox[(aSourceA ^ aSaltA ^ aTapB ^ FoldWordToByte(aAccA)) & 127U]);
    const std::uint32_t aBoxB = static_cast<std::uint32_t>(aMixBox[(aSourceB + aSaltB + aTapA + FoldWordToByte(aAccB)) & 127U]);
    aAccA = AdvanceTwiddle32(aAccA ^ pTwiddleA, aTapA ^ aBoxA, aSourceB ^ aSaltB, 0x5AA5D779u, 5u);
    aAccB = AdvanceTwiddle32(aAccB ^ pTwiddleB, aTapB ^ aBoxB, aSourceA ^ aSaltA, 0xC9A3BC0Du, 8u);
    const unsigned char aWaveA = aMixBox[(FoldWordToByte(aAccA) ^ aSourceA ^ aSaltB ^ static_cast<unsigned char>(i)) & 127U];
    const unsigned char aWaveB = aMixBox[(FoldWordToByte(aAccB) ^ aSourceB ^ aSaltA ^ static_cast<unsigned char>(i >> 1U)) & 127U];
    switch ((static_cast<unsigned>(aBoxA ^ aBoxB ^ static_cast<std::uint32_t>(pRound)) + 1u) & 3U) {
      case 0U:
        pWorkerA[i] ^= aWaveB;
        pWorkerB[i] = static_cast<unsigned char>(pWorkerB[i] + aWaveA);
        break;
      case 1U:
        pWorkerA[i] = RotateLeft8(static_cast<std::uint8_t>(pWorkerA[i] + aWaveA), 1U + (aWaveB & 3U));
        pWorkerB[i] ^= RotateLeft8(aWaveB, 1U + (aWaveA & 3U));
        break;
      case 2U: {
        const std::size_t aPartner = (i + 1U + (aBoxA & 15U)) % PASSWORD_EXPANDED_SIZE;
        const unsigned char aMix = static_cast<unsigned char>(aWaveA ^ aWaveB);
        pWorkerA[i] ^= pWorkerB[aPartner] ^ aMix;
        pWorkerB[aPartner] = static_cast<unsigned char>(pWorkerB[aPartner] + aMix);
        break;
      }
      default:
        pWorkerA[i] = static_cast<unsigned char>((pWorkerA[i] & 0xF0u) | (aWaveB & 0x0Fu));
        pWorkerB[i] = static_cast<unsigned char>((pWorkerB[i] & 0x0Fu) | (aWaveA & 0xF0u));
        break;
    }
    if (((i + 1U) % 12u) == 0U) {
      const std::uint32_t aCarry = aAccA;
      aAccA = aAccB ^ RotateLeft32(aCarry, 3U);
      aAccB = aCarry + RotateLeft32(aAccB, 5U);
    }
  }
}

static void TwistCandidate_61681_FinalWhitening(
    unsigned char* pSource,
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned int pRound,
    std::uint32_t pTwiddleA,
    std::uint32_t pTwiddleB) {
  if (pSource == nullptr || pDest == nullptr) {
    return;
  }
  static constexpr unsigned char aMixBox[128] = {
      0xEBU, 0x48U, 0x8DU, 0x7CU, 0x9AU, 0x2FU, 0xF6U, 0x1CU, 0xBCU, 0x47U, 0xFBU, 0x92U, 0x02U, 0xB5U, 0x8EU, 0x8AU,
      0x0BU, 0x74U, 0xA6U, 0xADU, 0xB8U, 0x0CU, 0x6EU, 0x26U, 0x13U, 0x5DU, 0x66U, 0x89U, 0x05U, 0x31U, 0x25U, 0x32U,
      0xF5U, 0xECU, 0x6FU, 0xA0U, 0xACU, 0x30U, 0x77U, 0xA2U, 0xD5U, 0xEEU, 0xD4U, 0x42U, 0xF3U, 0x18U, 0xA1U, 0xB0U,
      0x64U, 0xF1U, 0xEAU, 0x17U, 0x0EU, 0xBFU, 0x70U, 0xA7U, 0x8CU, 0x7FU, 0x46U, 0x95U, 0x73U, 0xC4U, 0x86U, 0xE7U,
      0x34U, 0xA3U, 0x27U, 0xBDU, 0xB3U, 0xC6U, 0x2AU, 0xD8U, 0x0DU, 0x3DU, 0x6CU, 0x09U, 0xB9U, 0xBEU, 0x0AU, 0x38U,
      0x55U, 0xE4U, 0x36U, 0xDEU, 0x81U, 0x61U, 0xF2U, 0x29U, 0x7BU, 0xC9U, 0xB2U, 0x52U, 0xA8U, 0x85U, 0xBBU, 0x87U,
      0x1BU, 0xABU, 0x49U, 0x80U, 0xD9U, 0x60U, 0x5FU, 0xE9U, 0x10U, 0xFCU, 0x94U, 0xD3U, 0xAAU, 0x19U, 0x3BU, 0x7AU,
      0xAFU, 0xFDU, 0x58U, 0xDDU, 0xC3U, 0x97U, 0x59U, 0x5AU, 0x04U, 0xF4U, 0x0FU, 0x22U, 0x20U, 0xD1U, 0x88U, 0xE3U
  };
  std::uint32_t aAcc = AdvanceTwiddle32(pTwiddleA ^ static_cast<std::uint32_t>(pRound), pTwiddleB, 0x7C059E5Fu, 0x23BCD2C7u, 1u);
  // Final whitening
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (5992), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aNeighborIndex = (i + 7u) % PASSWORD_EXPANDED_SIZE;
    const std::uint32_t aSourceByte = static_cast<std::uint32_t>(pSource[aSourceIndex]);
    const std::uint32_t aDestByte = static_cast<std::uint32_t>(pDest[i]);
    const std::uint32_t aNeighborByte = static_cast<std::uint32_t>(pDest[aNeighborIndex]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[(i + 25u) & 31U]);
    const std::uint32_t aMix = static_cast<std::uint32_t>(aMixBox[(aDestByte ^ aSourceByte ^ aSaltByte ^ FoldWordToByte(aAcc)) & 127U]);
    aAcc = AdvanceTwiddle32(aAcc, aDestByte ^ aMix, aSourceByte ^ aNeighborByte ^ aSaltByte, 0x23BCD2C7u, 1u);
    const unsigned char aWave = aMixBox[(FoldWordToByte(aAcc) + aSourceByte + aNeighborByte + aSaltByte) & 127U];
    switch ((static_cast<unsigned>(aMix) + 0u) % 3U) {
      case 0U:
        pDest[i] ^= aWave;
        break;
      case 1U:
        pDest[i] = RotateLeft8(static_cast<std::uint8_t>(pDest[i] + aWave), 1U + (aWave & 3U));
        break;
      default:
        pDest[i] = static_cast<unsigned char>((pDest[i] & 0xF0u) | (aWave & 0x0Fu));
        pDest[aNeighborIndex] ^= static_cast<unsigned char>(aWave & 0xF0u);
        break;
    }
  }
}

static void TwistCandidate_61681_ApplyDualWorkerMatrixBreaker(
    const DualWorkerBreakerRecipe4& pRecipe,
    const unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned int pRound,
    unsigned char (&pBreakerTempA)[kMatrixBlockBytes],
    unsigned char (&pBreakerTempB)[kMatrixBlockBytes]) {
  if (pSource == nullptr || pWorkerA == nullptr || pWorkerB == nullptr) {
    return;
  }
  std::uint32_t aSequenceA = TwistCandidate_61681_AdvanceTwiddle32A(
      0x57163645u ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pRecipe.fast_rule) ^ static_cast<std::uint32_t>(pRecipe.key_row),
      static_cast<std::uint32_t>(pRecipe.slow_rule) ^ static_cast<std::uint32_t>(pRecipe.key_offset));
  std::uint32_t aSequenceB = TwistCandidate_61681_AdvanceTwiddle32B(
      0xE8BCE01Fu ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pRecipe.mixlane_a) ^ static_cast<std::uint32_t>(pRecipe.matrix_mode),
      static_cast<std::uint32_t>(pRecipe.mixlane_b) ^ static_cast<std::uint32_t>(pRecipe.inject_mode));
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMatrixBlockBytes) {
    DualWorkerBreakerRecipe4 aRecipe = pRecipe;
    const unsigned char aSequenceByteA = FoldWordToByte(aSequenceA);
    const unsigned char aSequenceByteB = FoldWordToByte(aSequenceB);
    aRecipe.mixlane_a = static_cast<std::uint8_t>((aRecipe.mixlane_a + (aSequenceByteA & 3U) + ((chunk / kMatrixBlockBytes) & 1U)) & 15U);
    aRecipe.mixlane_b = static_cast<std::uint8_t>((aRecipe.mixlane_b + (aSequenceByteB & 3U) + ((aSequenceByteA >> 5U) & 1U)) & 15U);
    aRecipe.emit_mode = static_cast<std::uint8_t>((aRecipe.emit_mode ^ ((aSequenceByteA >> 7U) & 1U)) & 1U);
    aRecipe.inject_mode = static_cast<std::uint8_t>((aRecipe.inject_mode + (aSequenceByteB & 3U)) & 3U);
    aRecipe.matrix_mode = static_cast<std::uint8_t>((aRecipe.matrix_mode + ((aSequenceByteA >> 2U) & 3U)) & 3U);
    aRecipe.source_mix_variant = static_cast<std::uint8_t>((aRecipe.source_mix_variant + ((aSequenceByteB >> 4U) & 3U)) & 3U);
    aRecipe.lane_mix_variant_a = static_cast<std::uint8_t>((aRecipe.lane_mix_variant_a + ((aSequenceByteA >> 3U) & 3U)) & 3U);
    aRecipe.lane_mix_variant_b = static_cast<std::uint8_t>((aRecipe.lane_mix_variant_b + ((aSequenceByteB >> 1U) & 3U)) & 3U);
    std::array<std::array<unsigned char, kMatrixBlockBytes>, 4> aSourceBlocks{};
    for (std::size_t aSourceIndexId = 0; aSourceIndexId < aSourceBlocks.size(); ++aSourceIndexId) {
      const int aSourceOffset =
          aRecipe.source_offsets[aSourceIndexId] +
          static_cast<int>((aSequenceA >> ((aSourceIndexId * 7U) & 31U)) & 0x0FU) -
          static_cast<int>((aSequenceB >> ((aSourceIndexId * 5U) & 31U)) & 0x07U);
      const std::size_t aSourceIndex = static_cast<std::size_t>(WrapRange(
          static_cast<int>(chunk) +
              aSourceOffset +
              static_cast<int>(aSourceIndexId * static_cast<std::size_t>(((aRecipe.index_mode & 3U) + 1U) * 11U)),
          0,
          static_cast<int>(PASSWORD_EXPANDED_SIZE)));
      aSourceBlocks[aSourceIndexId] = LoadBlockWrapped<kMatrixBlockBytes>(pSource, PASSWORD_EXPANDED_SIZE, aSourceIndex);
    }
    const std::size_t aControlIndex = static_cast<std::size_t>(WrapRange(
        static_cast<int>(chunk) + aRecipe.control_offset + static_cast<int>(aSequenceByteA) - 16,
        0,
        static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aPartnerIndex = static_cast<std::size_t>(WrapRange(
        static_cast<int>(chunk) + aRecipe.partner_offset + static_cast<int>(aSequenceByteB) - 16,
        0,
        static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const auto aControl = LoadBlockWrapped<kMatrixBlockBytes>(pSource, PASSWORD_EXPANDED_SIZE, aControlIndex);
    const auto aPartner = (((aSequenceByteB ^ aRecipe.index_mode) & 1U) == 0U)
        ? LoadBlockWrapped<kMatrixBlockBytes>(pWorkerB, PASSWORD_EXPANDED_SIZE, aPartnerIndex)
        : LoadBlockWrapped<kMatrixBlockBytes>(pWorkerA, PASSWORD_EXPANDED_SIZE, aPartnerIndex);
    const auto aBlockA = LoadBlockWrapped<kMatrixBlockBytes>(pWorkerA, PASSWORD_EXPANDED_SIZE, chunk);
    const auto aBlockB = LoadBlockWrapped<kMatrixBlockBytes>(pWorkerB, PASSWORD_EXPANDED_SIZE, chunk);
    const auto aMaskA = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(
        pMaskStackA,
        chunk + aRecipe.mask_flat_offset + static_cast<std::size_t>((aSequenceByteA & 7U) * 5U));
    const auto aMaskB = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(
        pMaskStackB,
        chunk + aRecipe.mask_flat_offset + static_cast<std::size_t>((aSequenceByteB & 7U) * 7U) + 11U);
    const auto aKey = LoadKeyStackBlock16Wrapped(
        pKeyStack,
        static_cast<std::size_t>((pRound + aRecipe.key_row + (aSequenceByteA & 3U)) & 15U),
        chunk + aRecipe.key_offset + static_cast<std::size_t>(aSequenceByteB & 7U));
    const auto aLaneSeedA =
        BuildRecipeMatrixBytes(aRecipe, aSourceBlocks, aBlockA, aBlockB, aControl, aPartner, aKey, aMaskA, aMaskB, 0U);
    const auto aLaneSeedB =
        BuildRecipeMatrixBytes(aRecipe, aSourceBlocks, aBlockA, aBlockB, aControl, aPartner, aKey, aMaskA, aMaskB, 1U);
    const auto aFeedSeedA =
        BuildRecipeMatrixBytes(aRecipe, aSourceBlocks, aBlockA, aBlockB, aControl, aPartner, aKey, aMaskA, aMaskB, 2U);
    const auto aFeedSeedB =
        BuildRecipeMatrixBytes(aRecipe, aSourceBlocks, aBlockA, aBlockB, aControl, aPartner, aKey, aMaskA, aMaskB, 3U);
    LightningMatrix aLaneA(aLaneSeedA.data());
    LightningMatrix aLaneB(aLaneSeedB.data());
    LightningMatrix aFeedMatrixA(aFeedSeedA.data());
    LightningMatrix aFeedMatrixB(aFeedSeedB.data());
    InjectRecipeSourceMatrices(aRecipe, aSourceBlocks, aLaneA, aLaneB, pSalt);
    if (((aSequenceByteA ^ aRecipe.matrix_mode) & 1U) == 0U) {
      aLaneA.AddWith(aFeedMatrixA);
      aLaneB.XorWith(aFeedMatrixB);
    } else {
      aLaneA.XorWith(aFeedMatrixA);
      aLaneB.AddWith(aFeedMatrixB);
    }
    if (((aSequenceByteB ^ aRecipe.emit_mode) & 1U) != 0U) {
      aLaneA.XorWith(aFeedMatrixB);
      aLaneB.AddWith(aFeedMatrixA);
    }
    ApplyRecipeLaneInjects(aRecipe, aLaneA, aLaneB, aMaskA, aMaskB, aControl, aPartner, aKey, pSalt);
    if (((aSequenceByteA + aSequenceByteB + aRecipe.inject_mode) & 1U) == 0U) {
      aLaneA.InjectXor(aPartner.data(), aPartner.size(), aPartner[0] & 15U);
      aLaneB.InjectAdd(aKey.data(), aKey.size(), aKey[1] & 15U);
    } else {
      aLaneA.InjectAdd(aKey.data(), aKey.size(), aKey[2] & 15U);
      aLaneB.InjectXor(aPartner.data(), aPartner.size(), aPartner[3] & 15U);
    }
    std::uint32_t aSourceFold = FoldBytesXor(aSourceBlocks[0].data(), aSourceBlocks[0].size());
    aSourceFold ^= FoldBytesAdd(aSourceBlocks[1].data(), aSourceBlocks[1].size());
    aSourceFold ^= FoldBytesXor(aSourceBlocks[2].data(), aSourceBlocks[2].size()) << 1U;
    aSourceFold ^= FoldBytesAdd(aSourceBlocks[3].data(), aSourceBlocks[3].size()) << 2U;
    const std::uint32_t aSelector =
        FoldBytesXor(aControl.data(), aControl.size()) ^
        FoldBytesAdd(aMaskA.data(), aMaskA.size()) ^
        FoldBytesXor(aMaskB.data(), aMaskB.size()) ^
        FoldBytesXor(aKey.data(), aKey.size()) ^
        FoldBytesAdd(aPartner.data(), aPartner.size()) ^
        aSourceFold ^
        aSequenceA ^
        RotateLeft32(aSequenceB, 7U) ^
        static_cast<std::uint32_t>(aRecipe.fast_rule) ^
        static_cast<std::uint32_t>(pRound * 17U);
    aLaneA.ApplyFastOp(
        static_cast<LightningFastOp>((aSelector + aSequenceByteA) % 12U),
        static_cast<std::uint8_t>(aControl[0] ^ aKey[aRecipe.mixlane_a & 15U] ^ FoldRecipeSourceLane(aSourceBlocks, aSequenceByteA & 15U, aRecipe)),
        static_cast<std::uint8_t>(aMaskA[1] + aMaskB[2] + aRecipe.mixlane_b + FoldRecipeSourceLane(aSourceBlocks, 1U + (aSequenceByteB & 7U), aRecipe)));
    aLaneB.ApplyFastOp(
        static_cast<LightningFastOp>(((aSelector >> 3U) + aSequenceByteB) % 12U),
        static_cast<std::uint8_t>(aControl[2] + aKey[(aRecipe.mixlane_b + 3U) & 15U] + FoldRecipeSourceLane(aSourceBlocks, 2U + (aSequenceByteA & 7U), aRecipe)),
        static_cast<std::uint8_t>(aMaskB[3] ^ aMaskA[4] ^ aRecipe.mixlane_a ^ FoldRecipeSourceLane(aSourceBlocks, 3U + (aSequenceByteB & 7U), aRecipe)));
    const std::uint32_t aSlowSelector =
        aSelector ^
        FoldBytesAdd(aPartner.data(), aPartner.size()) ^
        RotateLeft32(aSourceFold, 5U) ^
        aSequenceB ^
        static_cast<std::uint32_t>(aRecipe.slow_rule);
    aLaneA.ApplySlowOp(
        static_cast<LightningSlowOp>((aSlowSelector + aSequenceByteB) % 8U),
        static_cast<std::uint8_t>(aControl[4] + aMaskA[5] + aMaskB[6] + FoldRecipeSourceLane(aSourceBlocks, 4U + (aSequenceByteA & 3U), aRecipe)),
        static_cast<std::uint8_t>(aKey[6] ^ pSalt[(aRecipe.salt_offset + 5U) & 31U] ^ FoldRecipeSourceLane(aSourceBlocks, 5U + (aSequenceByteB & 3U), aRecipe)));
    aLaneB.ApplySlowOp(
        static_cast<LightningSlowOp>(((aSlowSelector >> 5U) + aSequenceByteA) % 8U),
        static_cast<std::uint8_t>(aControl[7] ^ aMaskB[8] ^ aMaskA[9] ^ FoldRecipeSourceLane(aSourceBlocks, 6U + (aSequenceByteA & 3U), aRecipe)),
        static_cast<std::uint8_t>(aKey[9] + pSalt[(aRecipe.salt_offset + 11U) & 31U] + FoldRecipeSourceLane(aSourceBlocks, 7U + (aSequenceByteB & 3U), aRecipe)));
    ApplyLightningMixColumns(
        aLaneA,
        pSalt,
        static_cast<std::uint8_t>(aRecipe.salt_offset + FoldRecipeSourceLane(aSourceBlocks, 8U + (aSequenceByteA & 3U), aRecipe)),
        static_cast<std::uint8_t>(aRecipe.fast_rule ^ FoldWordToByte(aSelector ^ aSequenceA)),
        aRecipe.lane_mix_variant_a);
    ApplyLightningMixColumns(
        aLaneB,
        pSalt,
        static_cast<std::uint8_t>(aRecipe.salt_offset + 11U + FoldRecipeSourceLane(aSourceBlocks, 9U + (aSequenceByteB & 3U), aRecipe)),
        static_cast<std::uint8_t>(aRecipe.slow_rule ^ FoldWordToByte(aSlowSelector ^ aSequenceB)),
        aRecipe.lane_mix_variant_b);
    aLaneA.Store(pBreakerTempA);
    aLaneB.Store(pBreakerTempB);
    for (std::size_t aLane = 0; aLane < kMatrixBlockBytes; ++aLane) {
      const unsigned char aSourceFeedback = FoldRecipeSourceLane(aSourceBlocks, aLane + (aSequenceByteA & 3U), aRecipe);
      const unsigned char aFeedback = FixedMixBoxByte(static_cast<unsigned char>(
          aSourceFeedback ^
          aControl[(aLane + aRecipe.mixlane_b) & 15U] ^
          aKey[(aLane + 3U + (aSequenceByteB & 1U)) & 15U] ^
          aMaskA[(aLane + 5U) & 15U] ^
          aMaskB[(aLane + 9U) & 15U] ^
          aPartner[(aLane + aRecipe.mixlane_a + aRecipe.index_mode + (aSequenceByteA & 1U)) & 15U] ^
          pSalt[(aLane + aRecipe.salt_offset + (aSequenceByteB & 3U)) & 31U] ^
          aSequenceByteA ^
          aSequenceByteB));
      const unsigned char aStoreA = pBreakerTempA[aLane];
      const unsigned char aStoreB = pBreakerTempB[aLane];
      if (((aSequenceByteA + static_cast<unsigned char>(aLane) + aRecipe.emit_mode) & 1U) == 0U) {
        pWorkerA[chunk + aLane] = static_cast<unsigned char>(
            aStoreA ^
            RotateLeft8(pBreakerTempB[(aLane + aRecipe.mixlane_a + aRecipe.index_mode) & 15U], 1U) ^
            aFeedback);
        pWorkerB[chunk + aLane] = static_cast<unsigned char>(
            aStoreB +
            RotateLeft8(pBreakerTempA[(aLane + aRecipe.mixlane_b + (aRecipe.index_mode & 3U)) & 15U], 3U) +
            aFeedback);
      } else {
        pWorkerA[chunk + aLane] = static_cast<unsigned char>(
            aStoreA +
            RotateLeft8(pBreakerTempB[(aLane + aRecipe.mixlane_a + aRecipe.index_mode) & 15U], 1U) +
            aFeedback);
        pWorkerB[chunk + aLane] = static_cast<unsigned char>(
            aStoreB ^
            RotateLeft8(pBreakerTempA[(aLane + aRecipe.mixlane_b + (aRecipe.index_mode & 3U)) & 15U], 3U) ^
            aFeedback);
      }
    }
    aSequenceA = TwistCandidate_61681_AdvanceTwiddle32A(
        aSequenceA ^ aSelector,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ aKey[1] ^ aPartner[2]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk));
    aSequenceB = TwistCandidate_61681_AdvanceTwiddle32B(
        aSequenceB ^ aSlowSelector,
        static_cast<std::uint32_t>(pBreakerTempB[3] ^ aMaskA[4] ^ aMaskB[5]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk >> 4U));
  }
}

static void TwistCandidate_61681_ApplyStepMixPulse(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned int pRound,
    unsigned char (&pBreakerTempA)[kMatrixBlockBytes],
    unsigned char (&pBreakerTempB)[kMatrixBlockBytes]) {
  if (pSource == nullptr || pWorkerA == nullptr || pWorkerB == nullptr) {
    return;
  }
  std::uint32_t aPulseA = TwistCandidate_61681_AdvanceTwiddle32A(
      0x5C8F9014u ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pSalt[20U]),
      0x9E3E422Cu ^ static_cast<std::uint32_t>(pSalt[20U]));
  std::uint32_t aPulseB = TwistCandidate_61681_AdvanceTwiddle32B(
      0x56B10914u + static_cast<std::uint32_t>(pRound * 17U),
      static_cast<std::uint32_t>(pSalt[20U]),
      0x71660A60u ^ static_cast<std::uint32_t>(pSalt[20U]));
  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMatrixBlockBytes) {
    const unsigned char aPulseByteA = FoldWordToByte(aPulseA);
    const unsigned char aPulseByteB = FoldWordToByte(aPulseB);
    const auto aSource = LoadBlock16Wrapped(pSource, chunk);
    const auto aSourcePrev = LoadBlock16Wrapped(
        pSource,
        static_cast<std::size_t>(WrapRange(
            static_cast<int>(chunk) - static_cast<int>((aPulseByteA & 7U) + 1U),
            0,
            static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto aSourceNext = LoadBlock16Wrapped(
        pSource,
        static_cast<std::size_t>(WrapRange(
            static_cast<int>(chunk) + static_cast<int>((aPulseByteB & 7U) + 1U),
            0,
            static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto aWorkerBlockA = LoadBlock16Wrapped(pWorkerA, chunk);
    const auto aWorkerBlockB = LoadBlock16Wrapped(pWorkerB, chunk);
    for (std::size_t aLane = 0; aLane < kMatrixBlockBytes; ++aLane) {
      const unsigned char aSourceStep = static_cast<unsigned char>(
          aSourceNext[(aLane + 1U + (aPulseByteA & 1U)) & 15U] -
          aSourcePrev[(aLane + 15U - (aPulseByteB & 1U)) & 15U]);
      const unsigned char aWorkerStepA = static_cast<unsigned char>(
          aWorkerBlockA[(aLane + 1U + ((aPulseByteA >> 1U) & 1U)) & 15U] ^
          aWorkerBlockA[(aLane + 15U - ((aPulseByteB >> 1U) & 1U)) & 15U]);
      const unsigned char aWorkerStepB = static_cast<unsigned char>(
          aWorkerBlockB[(aLane + 1U + ((aPulseByteB >> 2U) & 1U)) & 15U] -
          aWorkerBlockB[(aLane + 15U - ((aPulseByteA >> 2U) & 1U)) & 15U]);
      const unsigned char aWave = FixedMixBoxByte(static_cast<unsigned char>(
          aSourceStep ^
          RotateLeft8(aWorkerStepA, 1U + ((aPulseByteA + static_cast<unsigned char>(aLane)) & 3U)) ^
          RotateLeft8(aWorkerStepB, 1U + ((aPulseByteB + static_cast<unsigned char>(aLane)) & 3U)) ^
          aSource[(aLane + 3U + (aPulseByteA & 3U)) & 15U] ^
          pSalt[(aLane + 20U + (aPulseByteB & 3U)) & 31U] ^
          aPulseByteA ^
          static_cast<unsigned char>(pRound + static_cast<unsigned int>(aLane))));
      const unsigned char aMix = FixedMixBoxByte(static_cast<unsigned char>(
          aWave +
          aWorkerBlockA[(aLane + 5U + (aPulseByteB & 1U)) & 15U] +
          RotateLeft8(aWorkerBlockB[(aLane + 9U + (aPulseByteA & 1U)) & 15U], 1U + ((aPulseByteA >> 5U) & 3U)) +
          pSalt[(aLane + 20U + (aPulseByteA & 3U)) & 31U]));
      if (((aPulseByteA ^ aPulseByteB ^ static_cast<unsigned char>(aLane)) & 1U) == 0U) {
        pBreakerTempA[aLane] = static_cast<unsigned char>(
            aWorkerBlockA[aLane] ^
            aWave ^
            RotateLeft8(aWorkerBlockB[(aLane + (aPulseByteB & 7U)) & 15U], 1U + (aPulseByteA & 3U)));
        pBreakerTempB[aLane] = static_cast<unsigned char>(
            aWorkerBlockB[aLane] +
            aMix +
            RotateLeft8(aSource[(aLane + (aPulseByteA & 7U)) & 15U], 1U + (aPulseByteB & 3U)));
      } else {
        pBreakerTempA[aLane] = static_cast<unsigned char>(
            aWorkerBlockA[aLane] +
            aWave +
            RotateLeft8(aSource[(aLane + (aPulseByteA & 7U)) & 15U], 1U + (aPulseByteA & 3U)));
        pBreakerTempB[aLane] = static_cast<unsigned char>(
            aWorkerBlockB[aLane] ^
            aMix ^
            RotateLeft8(aWorkerBlockA[(aLane + (aPulseByteB & 7U)) & 15U], 1U + (aPulseByteB & 3U)));
      }
    }
    std::memcpy(pWorkerA + chunk, pBreakerTempA, kMatrixBlockBytes);
    std::memcpy(pWorkerB + chunk, pBreakerTempB, kMatrixBlockBytes);
    const std::uint32_t aPulseFold =
        FoldBytesXor(pBreakerTempA, kMatrixBlockBytes) ^
        RotateLeft32(FoldBytesAdd(pBreakerTempB, kMatrixBlockBytes), 3U) ^
        static_cast<std::uint32_t>(aPulseByteA ^ aPulseByteB);
    aPulseA = TwistCandidate_61681_AdvanceTwiddle32A(
        aPulseA ^ aPulseFold,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ pBreakerTempB[5]),
        static_cast<std::uint32_t>(chunk + static_cast<std::size_t>(aPulseByteB)));
    aPulseB = TwistCandidate_61681_AdvanceTwiddle32B(
        aPulseB ^ RotateLeft32(aPulseFold, 7U),
        static_cast<std::uint32_t>(pBreakerTempB[3] + pBreakerTempA[9]),
        static_cast<std::uint32_t>((chunk >> 4U) + static_cast<std::size_t>(aPulseByteA)));
  }
}

static void TwistCandidate_61681_TwistBlock(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pBreakerTempA)[kMatrixBlockBytes],
    unsigned char (&pBreakerTempB)[kMatrixBlockBytes],
    unsigned int pRound,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::uint32_t aTwiddleA = static_cast<std::uint32_t>(0xA511E9B3u ^ 0xA5EB2B9Cu ^ (pRound * 57u));
  aTwiddleA ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (6367), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleA ^= static_cast<std::uint32_t>(pSalt[(pRound + 0u) & 31U]) << 16U;
  std::uint32_t aTwiddleB = static_cast<std::uint32_t>(0xB44B1D93u ^ 0xA5EB2B9Cu ^ (pRound * 57u));
  aTwiddleB ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4223), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleB ^= static_cast<std::uint32_t>(pSalt[(pRound + 11u) & 31U]) << 16U;
  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(i + (-1805), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(i + (7040), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerA[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 31u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 53u + 26u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (7040 + 26), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackA[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackB[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 1u + 3U) & 15U), static_cast<std::size_t>(i + 12u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 11u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = static_cast<std::uint32_t>((aLaneIndex ^ static_cast<std::uint32_t>(224u)) & 0xFFu);
      const std::uint32_t aRoute = ((aSourceCtrl >> 0u) ^ aWorkerCtrl ^ aKeyByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = static_cast<std::uint32_t>(a ^ aKeyByte); break;
        case 2U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (aKeyByte & (~aMaskByte & 0xFFu))); break;
        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (aKeyByte & 0x0Fu)); break;
      }
      pWorkerA[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = AdvanceTwiddle32(aTwiddleA ^ static_cast<std::uint32_t>(pSalt[11u]), static_cast<std::uint32_t>((aValue) ^ RotateLeft32(static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 7U)), static_cast<std::uint32_t>(139u) + static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 0x165667B1u, 7u);
    }
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(aReverseIndex + (-2505), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(aReverseIndex + (-1225), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerB[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 10u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 209u + 11u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (-1225 + 11), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackB[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackA[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 10u + 3U) & 15U), static_cast<std::size_t>(i + 30u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 28u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = 0U;
      const int aIndex2 = WrapRange(i + (-7067), aStart, aEnd);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::uint32_t aRoute = ((aSourceCtrl >> 3u) ^ aWorkerCtrl ^ aKeyByte ^ aMaskByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = b; break;
        case 2U: aValue = static_cast<std::uint32_t>(a ^ b); break;
        default: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;
      }
      pWorkerB[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleB = AdvanceTwiddle32(aTwiddleB ^ static_cast<std::uint32_t>(pSalt[28u]), static_cast<std::uint32_t>((aValue) ^ RotateLeft32(static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 7U)), static_cast<std::uint32_t>(73u) + static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 0x165667B1u, 6u);
    }
  }

  TwistCandidate_61681_TsunamiBreaker(pSource, pWorkerA, pWorkerB, pSalt, pRound, aTwiddleA, aTwiddleB);

  {
    const DualWorkerBreakerRecipe4 aBreakerAB{
        {-1805, 6706, 3399, 7488},
        7040,
        8,
        static_cast<std::size_t>(511u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(31u),
        static_cast<std::uint8_t>(28u),
        static_cast<std::uint8_t>(6u),
        static_cast<std::uint8_t>(8u),
        static_cast<std::uint8_t>(120u),
        static_cast<std::uint8_t>(110u),
        static_cast<std::uint8_t>(8u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(3u),
    };
    TwistCandidate_61681_ApplyDualWorkerMatrixBreaker(aBreakerAB, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound, pBreakerTempA, pBreakerTempB);
  }

  {
    const DualWorkerBreakerRecipe4 aBreakerCD{
        {-2505, -7067, -4385, 5111},
        -6118,
        786,
        static_cast<std::size_t>(374u),
        static_cast<std::uint8_t>(13u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(22u),
        static_cast<std::uint8_t>(10u),
        static_cast<std::uint8_t>(10u),
        static_cast<std::uint8_t>(174u),
        static_cast<std::uint8_t>(109u),
        static_cast<std::uint8_t>(11u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(2u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(2u),
    };
    TwistCandidate_61681_ApplyDualWorkerMatrixBreaker(aBreakerCD, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound, pBreakerTempA, pBreakerTempB);
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex0 = WrapRange(i + (-5428), aStart, aEnd);
      const int aIndex1 = WrapRange(i + (1250), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (4974), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pWorkerA[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerB[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 28u + 480u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatOther = WrapRange(static_cast<int>(aMaskFlat) + (1 + 4), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackA[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackB[(static_cast<std::size_t>(aMaskFlatOther) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatOther) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 5U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aRoute = (c ^ aMaskByte ^ aMaskByteB ^ static_cast<std::uint32_t>(1u)) % 5U;
      std::uint32_t aOutA = a;
      std::uint32_t aOutB = b;
      switch ((aRoute + static_cast<std::uint32_t>(4u)) % 5U) {
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
      pWorkerA[i] = static_cast<std::uint8_t>(aOutA);
      pWorkerB[i] = static_cast<std::uint8_t>(aOutB);
    }
  }

  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kHurricaneBlockBytes) {
    auto storm_block = LoadBlock256Wrapped(pWorkerA, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-3171), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_control = LoadBlock256Wrapped(pSource, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (5724), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_mask_a = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackA, chunk + 46u);
    const auto storm_mask_b = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackB, chunk + 46u + 29u);
    HurricaneMatrix storm(storm_block.data());
    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 0xFFU);
    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 0xFFU);
    storm.InjectAdd(pSalt, kSaltBytes, 9u);
    if ((static_cast<unsigned>(storm_control[13] ^ storm_mask_a[17] ^ storm_mask_b[19] ^ pSalt[2]) % 4u) != 0u) {
    switch (static_cast<unsigned>(storm_control[13] ^ storm_mask_a[17] ^ storm_mask_b[19] ^ pSalt[2]) % 7u) {
      case 0u: storm.ApplyFastOp(HurricaneFastOp::kWeaveColumns, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 1u: storm.ApplyFastOp(HurricaneFastOp::kSwapRows, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 2u: storm.ApplyFastOp(HurricaneFastOp::kRotateColumnDown, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 3u: storm.ApplyFastOp(HurricaneFastOp::kRotateRowRight, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 4u: storm.ApplyFastOp(HurricaneFastOp::kXorRowIntoRow, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 5u: storm.ApplyFastOp(HurricaneFastOp::kAddColumnIntoColumn, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 6u: storm.ApplyFastOp(HurricaneFastOp::kWeaveRows, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
    }
    }
    if ((static_cast<unsigned>(storm_control[53] ^ storm_mask_a[59] ^ storm_mask_b[61] ^ pSalt[3]) % 4u) != 0u) {
    switch (static_cast<unsigned>(storm_control[53] ^ storm_mask_a[59] ^ storm_mask_b[61] ^ pSalt[3]) % 2u) {
      case 0u: storm.ApplySlowOp(HurricaneSlowOp::kFlipDiagB, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
      case 1u: storm.ApplySlowOp(HurricaneSlowOp::kRotateLeft, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
    }
    }
    storm.ApplySlowOp(static_cast<HurricaneSlowOp>((storm_control[79] ^ storm_mask_a[83] ^ storm_mask_b[89] ^ pSalt[4]) % 8U), static_cast<std::uint8_t>(storm_control[89] + storm_mask_a[97] + storm_mask_b[101]), static_cast<std::uint8_t>(storm_block[101] ^ storm_mask_a[103] ^ storm_mask_b[107]));
    storm.Store(pWorkerA + chunk);
    for (std::size_t aLane = 0; aLane < kHurricaneBlockBytes; ++aLane) {
      pWorkerA[chunk + aLane] = static_cast<std::uint8_t>(pWorkerA[chunk + aLane] ^ storm_control[(aLane + 11U) & 255U] ^ storm_mask_a[(aLane + 17U) & 255U] ^ storm_mask_b[(aLane + 29U) & 255U] ^ pSalt[aLane & 31U]);
    }
  }

  TwistCandidate_61681_ApplyStepMixPulse(pSource, pWorkerA, pWorkerB, pSalt, pRound, pBreakerTempA, pBreakerTempB);

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex0 = WrapRange(aReverseIndex + (-7140), aStart, aEnd);
      const int aIndex1 = WrapRange(aReverseIndex + (-2332), aStart, aEnd);
      const int aIndex2 = WrapRange(aReverseIndex + (-6688), aStart, aEnd);
      const int aControlIndex = WrapRange(aReverseIndex + (1550), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorkerB[aIndex2]);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 15u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 14u)));
      const std::uint32_t aRoute = (aSourceCtrl ^ aKeyByte ^ static_cast<std::uint32_t>(3u)) % 6U;
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
  TwistCandidate_61681_FinalWhitening(pSource, pDest, pSalt, pRound, aTwiddleA, aTwiddleB);

}

static void TwistCandidate_61681_PushKeyRound(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (1721)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 29u + (1081)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 1u + (3766)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 15u + static_cast<unsigned int>(6u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(17u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 197u)));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a + b + aSaltByte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(aMixValue);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ aKeyByte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

static void TwistCandidate_61681_PushMaskRoundA(
    unsigned char* pDest,
    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundMaskBuffer, 0, kMaskBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-4295)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 9u + (1110)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-2830)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(107u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(107u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] = static_cast<unsigned char>(pNextRoundMaskBuffer[aMaskIndex] + static_cast<unsigned char>((((a ^ c) + b) ^ aOtherMask) & 0xFFu));
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

static void TwistCandidate_61681_PushMaskRoundB(
    unsigned char* pDest,
    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],
    unsigned int pLength) {
  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  std::memset(pNextRoundMaskBuffer, 0, kMaskBytes);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-5534)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 2u + (-6281)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 14u + (-2468)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(59u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(59u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] ^= static_cast<unsigned char>((((a + c) ^ b) + aOtherMask) & 0xFFu);
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

void TwistCandidate_61681(
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDest,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],
    unsigned int pLength) {
  if (pLength == 0U) {
    return;
  }
  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {
    return;
  }
  unsigned char aSalt[kSaltBytes]{};
  unsigned char aBreakerTempA[kMatrixBlockBytes]{};
  unsigned char aBreakerTempB[kMatrixBlockBytes]{};
  TwistCandidate_61681_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_61681_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_61681_MaskSeedA(pSource, pWorkerA, pMaskStackA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_61681_MaskSeedB(pSource, pWorkerB, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_61681_TwistBlock(aRoundSource, pWorkerA, pWorkerB, aRoundDest, aBreakerTempA, aBreakerTempB, aRound, aSalt, pKeyStack, pMaskStackA, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_61681_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_61681_PushMaskRoundA(aRoundDest, pMaskStackA, pMaskStackB, pNextRoundMaskBufferA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_61681_PushMaskRoundB(aRoundDest, pMaskStackB, pMaskStackA, pNextRoundMaskBufferB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

const RegisteredCandidate kRegisteredCandidates[] = {
  {
    61681,
    "TwistCandidate_61681",
    0,
    0,
    0,
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    "TwistCandidate_61681",
    &TwistCandidate_61681,
    &TwistCandidate_61681_KeySeed,
    &TwistCandidate_61681_SaltSeed,
    &TwistCandidate_61681_MaskSeedA,
    &TwistCandidate_61681_MaskSeedB,
    &TwistCandidate_61681_TwistBlock,
    &TwistCandidate_61681_PushKeyRound,
    &TwistCandidate_61681_PushMaskRoundA,
    &TwistCandidate_61681_PushMaskRoundB,
  }
};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}  // namespace twist
