#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistBreakers.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {

// Candidate 142215: TwistCandidate_142215
// family=ferocious_dual_worker op_budget=8 worker_shapes=1x1/2x4 worker_reverse=(False, True, False, True)/(False, False, True, False) twiddle1=mix/none twiddle2=mix/i_mul twiddle2_source=A tsunami=custom breaker_shapes=aBreakerAB:4m1i1v120 lane_breaker=true braid_breaker=false jump_breaker=false swap_breaker=false mask_template=3 mask_seed_a=off mask_seed_b=sbox_wave lightning=false/1 typhoon=true hurricane=true final_reverse=(True, True, True, False)
// ferocious[wa=1x1; wb=2x4; tw1=mix/none; tw2=mix/i_mul; tw2src=A; saltbox=128:00106773:7fb14455; tsunami=custom; final_whitening=custom; breaker_shapes=aBreakerAB:4m1i1v120; wa_rev=(False, True, False, True); wb_rev=(False, False, True, False); lane=on:2; braid=off:2; jump=off:3; swap=off:2; mask=workerbx3; lightning=off:1; typhoon=on; hurricane=on; final=5/(True, True, True, False); key_rot=11; maskA_seed=off/True; maskB_seed=sbox_wave; maskA_bias=206; maskB_bias=1]
static std::uint32_t TwistCandidate_142215_AdvanceTwiddle32A(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState ^ 0x78BC92D7u;
  aState = AdvanceTwiddle32(
      aState,
      pValue ^ 0xDD19C778u,
      pExtra + 0x36DBC0EEu,
      0xF57E1851u,
      9u);
  aState ^= RotateLeft32(pValue + pExtra + 0xEBC20796u, 11U);
  return static_cast<std::uint32_t>(aState + 0x8DC28A86u);
}

static std::uint32_t TwistCandidate_142215_AdvanceTwiddle32B(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState + 0x36DBC0EEu;
  aState = AdvanceTwiddle32(
      aState ^ RotateLeft32(pExtra, 5U),
      pValue + 0xF57E1851u,
      pExtra ^ 0x78BC92D7u,
      0xDD19C778u,
      10u);
  aState ^= RotateLeft32(pValue ^ pExtra ^ 0x55D65A4Fu, 10U);
  return static_cast<std::uint32_t>(aState ^ 0xC3A5D8BFu);
}

static void TwistCandidate_142215_ApplySaltMixBox(
    unsigned char (&pSalt)[kSaltBytes],
    std::uint32_t pState,
    std::uint32_t pBias,
    unsigned pRotate) {
  std::uint32_t aTwiddleA = TwistCandidate_142215_AdvanceTwiddle32A(
      pState ^ pBias,
      static_cast<std::uint32_t>(pSalt[(pBias + 3U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 17U));
  std::uint32_t aTwiddleB = TwistCandidate_142215_AdvanceTwiddle32B(
      pState + pBias + static_cast<std::uint32_t>(pRotate),
      static_cast<std::uint32_t>(pSalt[(pBias + 11U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 29U));
  for (std::size_t aIndex = 0; aIndex < kSaltBytes; ++aIndex) {
    const unsigned char aSaltA = pSalt[aIndex];
    const unsigned char aSaltB = pSalt[(aIndex + 7U + (FoldWordToByte(aTwiddleA) & 3U)) & 31U];
    const unsigned char aSaltC = pSalt[(aIndex + 13U + (FoldWordToByte(aTwiddleB) & 3U)) & 31U];
    aTwiddleA = TwistCandidate_142215_AdvanceTwiddle32A(
        aTwiddleA ^ static_cast<std::uint32_t>(aSaltB),
        static_cast<std::uint32_t>(aSaltA) ^ pBias,
        static_cast<std::uint32_t>(aSaltC) ^ static_cast<std::uint32_t>(aIndex * 17U + pRotate));
    aTwiddleB = TwistCandidate_142215_AdvanceTwiddle32B(
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

static void TwistCandidate_142215_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  static constexpr unsigned char aSaltMixBox[128] = {
      0x40U, 0x5AU, 0x25U, 0x05U, 0x43U, 0x4EU, 0xA7U, 0x55U, 0x6DU, 0xE1U, 0xA0U, 0x04U, 0x3CU, 0x80U, 0xFEU, 0x26U,
      0xC3U, 0xEBU, 0x83U, 0x9BU, 0x07U, 0x47U, 0x27U, 0x97U, 0xECU, 0xE2U, 0x7CU, 0xBCU, 0xFAU, 0xF0U, 0x2BU, 0xEFU,
      0xD2U, 0x20U, 0x95U, 0x62U, 0x3DU, 0x87U, 0x98U, 0xD4U, 0x09U, 0x21U, 0xC7U, 0x9AU, 0x32U, 0x79U, 0x1CU, 0x7BU,
      0xBFU, 0xF4U, 0xB5U, 0x93U, 0x75U, 0xF7U, 0x89U, 0xACU, 0x46U, 0x06U, 0x8EU, 0xDAU, 0xFCU, 0x5EU, 0x64U, 0x73U,
      0x94U, 0xD0U, 0x57U, 0x31U, 0xA2U, 0x18U, 0x8FU, 0x10U, 0x36U, 0x77U, 0x69U, 0x1DU, 0x8AU, 0xF8U, 0xB2U, 0x85U,
      0x16U, 0x4AU, 0xB0U, 0x60U, 0xBBU, 0xB4U, 0xD7U, 0xC4U, 0x4CU, 0x2FU, 0xAFU, 0x92U, 0xB6U, 0xF1U, 0x08U, 0x17U,
      0x38U, 0xD8U, 0xCFU, 0xA4U, 0x54U, 0x88U, 0x7EU, 0xC9U, 0xA1U, 0xE8U, 0x91U, 0xE3U, 0x15U, 0x2AU, 0x5DU, 0x1EU,
      0xFFU, 0x1AU, 0xFDU, 0x61U, 0x59U, 0x48U, 0xDFU, 0x5BU, 0x52U, 0x11U, 0xF3U, 0x34U, 0xD9U, 0x78U, 0x39U, 0xD5U
  };
  std::memset(pSalt, 0, kSaltBytes);
  std::uint32_t aSaltAcc = static_cast<std::uint32_t>(0xA5A5A5A5u ^ 99u ^ 1u);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (-4794)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 6u + (6651)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 18u + static_cast<unsigned int>(99u)) & 31U;
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^
        static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aB & 0xFFu), 4u)) ^
        static_cast<std::uint32_t>(aSourceIndex + 99u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aSaltWave, aSaltIndex, aA ^ aSaltWave, aB, static_cast<std::uint32_t>(aSourceIndex), static_cast<std::uint32_t>(1u), 4u, true);
    pSalt[(aSaltIndex + 17U) & 31U] ^= static_cast<unsigned char>(aSaltMixBox[(
        aSaltWave ^ pSalt[(aSaltIndex + 5U) & 31U] ^ static_cast<std::uint32_t>(aSourceIndex)) & 127U]);
    ++aSourceIndex;
  }
  TwistCandidate_142215_ApplySaltMixBox(pSalt, aSaltAcc, static_cast<std::uint32_t>(1u), 4u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] ^ pSalt[(aSaltLane + 7U) & 31U] ^ static_cast<unsigned char>(1u) ^ static_cast<unsigned char>(aSaltLane)) & 127U]);
    pSalt[aSaltLane] = static_cast<unsigned char>(
        FixedMixBoxByte(static_cast<unsigned char>(pSalt[aSaltLane] ^ aSaltWave ^ pSalt[(aSaltLane + 13U) & 31U])) +
        static_cast<unsigned char>(aSaltWave));
  }
  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 31u + (-4640)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 31u + (6497)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = static_cast<unsigned int>((aSourceIndex * 31u + pSalt[(aSourceIndex + 51u) & 31U]) & 31U);
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pSalt[(aSaltIndex + 11U) & 31U]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^ aB ^ aCarry ^ static_cast<std::uint32_t>(51u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aCarry ^ aSaltWave, aSaltIndex, aA ^ aCarry, aB, static_cast<std::uint32_t>(aSourceIndex + aCarry), static_cast<std::uint32_t>(51u), 3u, true);
    pSalt[(aSaltIndex + 23U) & 31U] = static_cast<unsigned char>(pSalt[(aSaltIndex + 23U) & 31U] + static_cast<unsigned char>(aSaltWave));
  }
  TwistCandidate_142215_ApplySaltMixBox(pSalt, aSaltAcc ^ static_cast<std::uint32_t>(51u), static_cast<std::uint32_t>(51u), 3u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] + pSalt[(aSaltLane + 19U) & 31U] + static_cast<unsigned char>(51u + 13u)) & 127U]);
    pSalt[aSaltLane] ^= static_cast<unsigned char>(aSaltWave);
  }
}

static void TwistCandidate_142215_KeySeed(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 7u + (5048)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-3632)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pKeyStack[aKeyPlaneIndex][aKeyIndex]);
    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);
    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 37u) ^ aLane ^ static_cast<std::uint32_t>(aKeyIndex * 13u) ^ static_cast<std::uint32_t>(aKeyPlaneIndex * 29u)) & 0xFFu);
    const std::uint32_t aPreMix = static_cast<std::uint32_t>((((a + 99u) ^ ((b << 4u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu));
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

static void TwistCandidate_142215_MaskSeedA(
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
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 203u ^ 206u);
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aReverseIndex * 25u + (1422)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aReverseIndex * 13u + (4151)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aReverseIndex * (25u + 13u) + (3088)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aReverseIndex) * 25u * 8U + static_cast<std::size_t>(aReverseIndex) * 13u + 203u + static_cast<std::size_t>(aTurbulence * ((24u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(203u), 6u, false);
  }
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 41u + (1422)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aSourceIndex * 42u + (4151)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aSourceIndex * (41u + 42u) + (3088)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 41u * 8U + static_cast<std::size_t>(aSourceIndex) * 42u * 3U + 31u + static_cast<std::size_t>(aTurbulence * ((24u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStackA[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), 1u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>(31u), 1u, false);
  }
}

static void TwistCandidate_142215_MaskSeedB(
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
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 197u ^ 1u);
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-2395)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 20u + (3734)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * (15u + 20u) + (-4697)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const int aWorkerIndex = WrapRange(static_cast<int>(aReverseIndex * 28u + (-780)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t aWorkerByte = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;
    const std::uint32_t aTurbulence = static_cast<std::uint32_t>(aMixBox[(aWorkerByte ^ a ^ static_cast<std::uint32_t>(36u)) & 0xFFu]);
    const std::uint32_t aSeedA = static_cast<std::uint32_t>((a ^ aTurbulence) & 0xFFu);
    const std::uint32_t aSeedB = static_cast<std::uint32_t>((b + aTurbulence + aWorkerByte) & 0xFFu);
    const std::uint32_t aSeedC = static_cast<std::uint32_t>((c ^ static_cast<std::uint32_t>(aMixBox[(aWorkerByte + c + static_cast<std::uint32_t>(36u)) & 0xFFu])) & 0xFFu);
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 15u * 8U + static_cast<std::size_t>(aSourceIndex) * 20u + 197u + static_cast<std::size_t>(aTurbulence * ((28u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackB, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(197u), 3u, false);
    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + 36u) * ((28u & 15U) + 1U))) % (kMaskStackTotalBytes * 8U);
    UpdateMaskSeedBit(pMaskStackB, aTurbulenceBit, aMaskSeedState ^ aTurbulence ^ aWorkerByte, 4u, false);
  }
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 24u + (-2395)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aSourceIndex * 21u + (3734)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aReverseIndex * (24u + 21u) + (-4697)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const int aWorkerIndex = WrapRange(static_cast<int>(aReverseIndex * 28u + (-780)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t aWorkerByte = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;
    const std::uint32_t aTurbulence = static_cast<std::uint32_t>(aMixBox[(aWorkerByte ^ a ^ static_cast<std::uint32_t>(36u)) & 0xFFu]);
    const std::uint32_t aSeedA = static_cast<std::uint32_t>((a ^ aTurbulence) & 0xFFu);
    const std::uint32_t aSeedB = static_cast<std::uint32_t>((b + aTurbulence + aWorkerByte) & 0xFFu);
    const std::uint32_t aSeedC = static_cast<std::uint32_t>((c ^ static_cast<std::uint32_t>(aMixBox[(aWorkerByte + c + static_cast<std::uint32_t>(36u)) & 0xFFu])) & 0xFFu);
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 24u * 8U + static_cast<std::size_t>(aSourceIndex) * 21u * 3U + 83u + static_cast<std::size_t>(aTurbulence * ((28u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStackB[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackB, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), 7u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>(83u), 7u, false);
    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + 36u) * ((28u & 31U) + 1U))) % (kMaskStackTotalBytes * 8U);
    UpdateMaskSeedBit(pMaskStackB, aTurbulenceBit, RotateLeft32(aMaskSeedState ^ aTurbulence ^ aCarry, 7U), 7u, true);
  }
}

static void TwistCandidate_142215_TsunamiBreaker(
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
      0x88U, 0xF3U, 0x0AU, 0x63U, 0x70U, 0x08U, 0xC1U, 0x3DU, 0x60U, 0x96U, 0xE8U, 0xC2U, 0x81U, 0x42U, 0xA7U, 0xB1U,
      0xB4U, 0x36U, 0x6DU, 0xAAU, 0xC9U, 0x6FU, 0xB0U, 0xDEU, 0x20U, 0x3CU, 0x19U, 0x5FU, 0xE2U, 0x71U, 0xFEU, 0x3EU,
      0x15U, 0xB7U, 0xA2U, 0x13U, 0x85U, 0x55U, 0x6EU, 0x9FU, 0x10U, 0x83U, 0xDDU, 0x66U, 0xE3U, 0xE9U, 0xE7U, 0x03U,
      0x45U, 0xB2U, 0x5BU, 0x77U, 0x62U, 0xECU, 0x1CU, 0xCEU, 0x14U, 0xB8U, 0x9BU, 0xEAU, 0x8BU, 0x97U, 0xBAU, 0xF0U,
      0xF9U, 0x1AU, 0x59U, 0x04U, 0x1BU, 0xA8U, 0xACU, 0xC4U, 0xCBU, 0xEEU, 0xC6U, 0xFDU, 0x23U, 0x95U, 0x4EU, 0xBEU,
      0x22U, 0x46U, 0x05U, 0x93U, 0xDAU, 0x0BU, 0xA0U, 0x90U, 0xEBU, 0x8AU, 0xBFU, 0xF7U, 0x00U, 0x0CU, 0x09U, 0x38U,
      0x5DU, 0x8DU, 0x86U, 0x6CU, 0xF8U, 0xA9U, 0xB3U, 0xE0U, 0x48U, 0x8CU, 0x16U, 0xC0U, 0xC3U, 0x7CU, 0xDBU, 0x32U,
      0xE1U, 0xD7U, 0x5EU, 0x0DU, 0xD1U, 0x12U, 0x9AU, 0x79U, 0x06U, 0x3AU, 0xD5U, 0xE4U, 0x2EU, 0x87U, 0x4FU, 0x44U
  };
  std::uint32_t aAccA = AdvanceTwiddle32(0xAE531D32u ^ static_cast<std::uint32_t>(pRound), pTwiddleA, pTwiddleB, 0x369A6978u, 10u);
  std::uint32_t aAccB = AdvanceTwiddle32(0x0E5D7AEAu ^ static_cast<std::uint32_t>(pRound), pTwiddleB, pTwiddleA, 0xE425C69Fu, 3u);
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (3384), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aSourceIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-5741), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-1757), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (6851), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::uint32_t aSourceA = static_cast<std::uint32_t>(pSource[aSourceIndexA]);
    const std::uint32_t aSourceB = static_cast<std::uint32_t>(pSource[aSourceIndexB]);
    const std::uint32_t aTapA = static_cast<std::uint32_t>(pWorkerA[aWorkerIndexA]);
    const std::uint32_t aTapB = static_cast<std::uint32_t>(pWorkerB[aWorkerIndexB]);
    const std::uint32_t aSaltA = static_cast<std::uint32_t>(pSalt[(i + 21u) & 31U]);
    const std::uint32_t aSaltB = static_cast<std::uint32_t>(pSalt[(i + 20u) & 31U]);
    const std::uint32_t aBoxA = static_cast<std::uint32_t>(aMixBox[(aSourceA ^ aSaltA ^ aTapB ^ FoldWordToByte(aAccA)) & 127U]);
    const std::uint32_t aBoxB = static_cast<std::uint32_t>(aMixBox[(aSourceB + aSaltB + aTapA + FoldWordToByte(aAccB)) & 127U]);
    aAccA = AdvanceTwiddle32(aAccA ^ pTwiddleA, aTapA ^ aBoxA, aSourceB ^ aSaltB, 0x369A6978u, 10u);
    aAccB = AdvanceTwiddle32(aAccB ^ pTwiddleB, aTapB ^ aBoxB, aSourceA ^ aSaltA, 0xE425C69Fu, 3u);
    const unsigned char aWaveA = aMixBox[(FoldWordToByte(aAccA) ^ aSourceA ^ aSaltB ^ static_cast<unsigned char>(i)) & 127U];
    const unsigned char aWaveB = aMixBox[(FoldWordToByte(aAccB) ^ aSourceB ^ aSaltA ^ static_cast<unsigned char>(i >> 1U)) & 127U];
    switch ((static_cast<unsigned>(aBoxA ^ aBoxB ^ static_cast<std::uint32_t>(pRound)) + 3u) & 3U) {
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
    if (((i + 1U) % 24u) == 0U) {
      const std::uint32_t aCarry = aAccA;
      aAccA = aAccB ^ RotateLeft32(aCarry, 3U);
      aAccB = aCarry + RotateLeft32(aAccB, 5U);
    }
  }
}

static void TwistCandidate_142215_FinalWhitening(
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
      0x20U, 0x67U, 0xC4U, 0xC5U, 0x1CU, 0xD6U, 0xBCU, 0xDCU, 0xD2U, 0x92U, 0x39U, 0xAEU, 0x66U, 0xD0U, 0xEEU, 0xA5U,
      0x4DU, 0x68U, 0xB6U, 0x3BU, 0xC8U, 0x37U, 0x1EU, 0x78U, 0x9AU, 0x5AU, 0x1DU, 0xE9U, 0xDEU, 0xD7U, 0xBBU, 0xA3U,
      0x6CU, 0x42U, 0x47U, 0x70U, 0x88U, 0x32U, 0x0DU, 0xF1U, 0x44U, 0x51U, 0xF3U, 0x8CU, 0x76U, 0xB0U, 0x73U, 0x38U,
      0x27U, 0x8FU, 0x35U, 0x89U, 0x6DU, 0x19U, 0x95U, 0xCFU, 0x0BU, 0x22U, 0xB4U, 0xA6U, 0x83U, 0xE3U, 0xEAU, 0x86U,
      0xE1U, 0x5FU, 0x61U, 0x8AU, 0x3DU, 0xD4U, 0x50U, 0x7CU, 0xE0U, 0xA9U, 0x2BU, 0x62U, 0x03U, 0x52U, 0x2EU, 0xB8U,
      0x00U, 0x59U, 0x17U, 0x4EU, 0x99U, 0xAAU, 0xF6U, 0x2AU, 0xE8U, 0x97U, 0x7EU, 0xA7U, 0xEFU, 0x43U, 0xEBU, 0x4AU,
      0x16U, 0xD5U, 0x84U, 0x5DU, 0x09U, 0x4FU, 0xC0U, 0xB3U, 0x24U, 0xAFU, 0x28U, 0xFCU, 0x94U, 0x25U, 0x7BU, 0x11U,
      0xB2U, 0xFDU, 0xDBU, 0x54U, 0xFBU, 0x3FU, 0xF2U, 0x0FU, 0x01U, 0xB9U, 0x10U, 0xD9U, 0x6BU, 0xDDU, 0x7DU, 0xCCU
  };
  std::uint32_t aAcc = AdvanceTwiddle32(pTwiddleA ^ static_cast<std::uint32_t>(pRound), pTwiddleB, 0xA0FBCB33u, 0x6309046Au, 4u);
  // Final whitening
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (6008), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aNeighborIndex = (i + 17u) % PASSWORD_EXPANDED_SIZE;
    const std::uint32_t aSourceByte = static_cast<std::uint32_t>(pSource[aSourceIndex]);
    const std::uint32_t aDestByte = static_cast<std::uint32_t>(pDest[i]);
    const std::uint32_t aNeighborByte = static_cast<std::uint32_t>(pDest[aNeighborIndex]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[(i + 10u) & 31U]);
    const std::uint32_t aMix = static_cast<std::uint32_t>(aMixBox[(aDestByte ^ aSourceByte ^ aSaltByte ^ FoldWordToByte(aAcc)) & 127U]);
    aAcc = AdvanceTwiddle32(aAcc, aDestByte ^ aMix, aSourceByte ^ aNeighborByte ^ aSaltByte, 0x6309046Au, 4u);
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

static void TwistCandidate_142215_ApplyDualWorkerMatrixBreaker(
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
  std::uint32_t aSequenceA = TwistCandidate_142215_AdvanceTwiddle32A(
      0x176472E3u ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pRecipe.fast_rule) ^ static_cast<std::uint32_t>(pRecipe.key_row),
      static_cast<std::uint32_t>(pRecipe.slow_rule) ^ static_cast<std::uint32_t>(pRecipe.key_offset));
  std::uint32_t aSequenceB = TwistCandidate_142215_AdvanceTwiddle32B(
      0x072C89C9u ^ static_cast<std::uint32_t>(pRound),
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
    aSequenceA = TwistCandidate_142215_AdvanceTwiddle32A(
        aSequenceA ^ aSelector,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ aKey[1] ^ aPartner[2]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk));
    aSequenceB = TwistCandidate_142215_AdvanceTwiddle32B(
        aSequenceB ^ aSlowSelector,
        static_cast<std::uint32_t>(pBreakerTempB[3] ^ aMaskA[4] ^ aMaskB[5]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk >> 4U));
  }
}

static void TwistCandidate_142215_ApplyStepMixPulse(
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
  std::uint32_t aPulseA = TwistCandidate_142215_AdvanceTwiddle32A(
      0x35297BDDu ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pSalt[29U]),
      0xF79C3BE3u ^ static_cast<std::uint32_t>(pSalt[19U]));
  std::uint32_t aPulseB = TwistCandidate_142215_AdvanceTwiddle32B(
      0xF4EF3373u + static_cast<std::uint32_t>(pRound * 17U),
      static_cast<std::uint32_t>(pSalt[19U]),
      0xD33BF87Bu ^ static_cast<std::uint32_t>(pSalt[29U]));
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
          pSalt[(aLane + 29U + (aPulseByteB & 3U)) & 31U] ^
          aPulseByteA ^
          static_cast<unsigned char>(pRound + static_cast<unsigned int>(aLane))));
      const unsigned char aMix = FixedMixBoxByte(static_cast<unsigned char>(
          aWave +
          aWorkerBlockA[(aLane + 5U + (aPulseByteB & 1U)) & 15U] +
          RotateLeft8(aWorkerBlockB[(aLane + 9U + (aPulseByteA & 1U)) & 15U], 1U + ((aPulseByteA >> 5U) & 3U)) +
          pSalt[(aLane + 19U + (aPulseByteA & 3U)) & 31U]));
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
    aPulseA = TwistCandidate_142215_AdvanceTwiddle32A(
        aPulseA ^ aPulseFold,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ pBreakerTempB[5]),
        static_cast<std::uint32_t>(chunk + static_cast<std::size_t>(aPulseByteB)));
    aPulseB = TwistCandidate_142215_AdvanceTwiddle32B(
        aPulseB ^ RotateLeft32(aPulseFold, 7U),
        static_cast<std::uint32_t>(pBreakerTempB[3] + pBreakerTempA[9]),
        static_cast<std::uint32_t>((chunk >> 4U) + static_cast<std::size_t>(aPulseByteA)));
  }
}

static void TwistCandidate_142215_TwistBlock(
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
  std::uint32_t aTwiddleA = static_cast<std::uint32_t>(0xA511E9B3u ^ 0xD0B5618Bu ^ (pRound * 43u));
  aTwiddleA ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (5048), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleA ^= static_cast<std::uint32_t>(pSalt[(pRound + 0u) & 31U]) << 16U;
  std::uint32_t aTwiddleB = static_cast<std::uint32_t>(0xB44B1D93u ^ 0xD0B5618Bu ^ (pRound * 43u));
  aTwiddleB ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-3632), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleB ^= static_cast<std::uint32_t>(pSalt[(pRound + 11u) & 31U]) << 16U;
  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(i + (-353), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(aReverseIndex + (3706), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerA[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 29u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 149u + 19u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (3706 + 19), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackA[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackB[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 1u + 3U) & 15U), static_cast<std::size_t>(i + 18u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 2u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = 0U;
      const std::uint32_t aRoute = ((aSourceCtrl >> 7u) ^ aWorkerCtrl ^ aKeyByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = static_cast<std::uint32_t>(a ^ aKeyByte); break;
        case 2U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (aKeyByte & (~aMaskByte & 0xFFu))); break;
        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (aKeyByte & 0x0Fu)); break;
      }
      pWorkerA[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = AdvanceTwiddle32(aTwiddleA ^ static_cast<std::uint32_t>(pSalt[2u]), static_cast<std::uint32_t>((aValue) ^ RotateLeft32(static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 7U)), static_cast<std::uint32_t>(206u) + static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 0x165667B1u, 10u);
    }
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(i + (-1194), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(i + (6716), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerB[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 4u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 22u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 14u + 4u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (6716 + 4), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackB[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackA[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 4u + 3U) & 15U), static_cast<std::size_t>(i + 27u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 20u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = static_cast<std::uint32_t>(((aLaneIndex * static_cast<std::uint32_t>(5u)) ^ static_cast<std::uint32_t>(117u)) & 0xFFu);
      const int aIndex2 = WrapRange(i + (4490), aStart, aEnd);
      const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::uint32_t aRoute = ((aSourceCtrl >> 4u) ^ aWorkerCtrl ^ aKeyByte ^ aMaskByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = b; break;
        case 2U: aValue = static_cast<std::uint32_t>(a ^ b); break;
        default: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;
      }
      pWorkerB[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = AdvanceTwiddle32(aTwiddleA ^ static_cast<std::uint32_t>(pSalt[20u]), static_cast<std::uint32_t>((aValue) ^ RotateLeft32(static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 7U)), static_cast<std::uint32_t>(2u) + static_cast<std::uint32_t>(aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias), 0x165667B1u, 10u);
    }
  }

  TwistCandidate_142215_TsunamiBreaker(pSource, pWorkerA, pWorkerB, pSalt, pRound, aTwiddleA, aTwiddleB);

  {
    const DualWorkerBreakerRecipe4 aBreakerAB{
        {-353, -5695, 3911, 6206},
        3706,
        8,
        static_cast<std::size_t>(83u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(29u),
        static_cast<std::uint8_t>(21u),
        static_cast<std::uint8_t>(13u),
        static_cast<std::uint8_t>(9u),
        static_cast<std::uint8_t>(81u),
        static_cast<std::uint8_t>(101u),
        static_cast<std::uint8_t>(8u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(2u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(1u),
    };
    TwistCandidate_142215_ApplyDualWorkerMatrixBreaker(aBreakerAB, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound, pBreakerTempA, pBreakerTempB);
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex0 = WrapRange(i + (-3190), aStart, aEnd);
      const int aIndex1 = WrapRange(i + (2523), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (-4304), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pWorkerB[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 14u + 604u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatOther = WrapRange(static_cast<int>(aMaskFlat) + (1 + 3), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackB[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackA[(static_cast<std::size_t>(aMaskFlatOther) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatOther) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 5U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aRoute = (c ^ aMaskByte ^ aMaskByteB ^ static_cast<std::uint32_t>(1u)) % 5U;
      std::uint32_t aOutA = a;
      std::uint32_t aOutB = b;
      switch ((aRoute + static_cast<std::uint32_t>(3u)) % 5U) {
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
    auto storm_block = LoadBlock128Wrapped(pWorkerB, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-6269), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_control = LoadBlock128Wrapped(pWorkerA, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (2679), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_mask_a = LoadMaskStackBlockWrapped<kTyphoonBlockBytes>(pMaskStackA, chunk + 658u);
    const auto storm_mask_b = LoadMaskStackBlockWrapped<kTyphoonBlockBytes>(pMaskStackB, chunk + 658u + 23u);
    TyphoonMatrix storm(storm_block.data());
    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 127U);
    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 127U);
    storm.InjectAdd(pSalt, kSaltBytes, 23u);
    if ((static_cast<unsigned>(storm_control[3] ^ storm_mask_a[11] ^ storm_mask_b[13] ^ pSalt[0]) % 3u) != 0u) {
    switch (static_cast<unsigned>(storm_control[3] ^ storm_mask_a[11] ^ storm_mask_b[13] ^ pSalt[0]) % 6u) {
      case 0u: storm.ApplyFastOp(TyphoonFastOp::kRotateColumnDown, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
      case 1u: storm.ApplyFastOp(TyphoonFastOp::kAddColumnIntoColumn, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
      case 2u: storm.ApplyFastOp(TyphoonFastOp::kAddRowIntoRow, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
      case 3u: storm.ApplyFastOp(TyphoonFastOp::kSwapRows, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
      case 4u: storm.ApplyFastOp(TyphoonFastOp::kSwapColumns, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
      case 5u: storm.ApplyFastOp(TyphoonFastOp::kXorColumnIntoColumn, static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29]), static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])); break;
    }
    }
    if ((static_cast<unsigned>(storm_control[37] ^ storm_mask_a[41] ^ storm_mask_b[43] ^ pSalt[1]) % 2u) != 0u) {
    switch (static_cast<unsigned>(storm_control[37] ^ storm_mask_a[41] ^ storm_mask_b[43] ^ pSalt[1]) % 2u) {
      case 0u: storm.ApplySlowOp(TyphoonSlowOp::kFlipHorizontal, static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53]), static_cast<std::uint8_t>(storm_block[53] ^ storm_mask_a[59] ^ storm_mask_b[61])); break;
      case 1u: storm.ApplySlowOp(TyphoonSlowOp::kTwistCross, static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53]), static_cast<std::uint8_t>(storm_block[53] ^ storm_mask_a[59] ^ storm_mask_b[61])); break;
    }
    }
    storm.ApplySlowOp(static_cast<TyphoonSlowOp>((storm_control[61] ^ storm_mask_a[67] ^ storm_mask_b[71] ^ pSalt[2]) % 4U), static_cast<std::uint8_t>(storm_control[71] + storm_mask_a[73] + storm_mask_b[79]), static_cast<std::uint8_t>(storm_block[79] ^ storm_mask_a[83] ^ storm_mask_b[89]));
    storm.Store(pWorkerB + chunk);
    for (std::size_t aLane = 0; aLane < kTyphoonBlockBytes; ++aLane) {
      pWorkerB[chunk + aLane] = static_cast<std::uint8_t>(pWorkerB[chunk + aLane] ^ storm_control[(aLane + 7U) & 127U] ^ storm_mask_a[(aLane + 13U) & 127U] ^ storm_mask_b[(aLane + 27U) & 127U] ^ pSalt[(aLane + 19U) & 31U]);
    }
  }

  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kHurricaneBlockBytes) {
    auto storm_block = LoadBlock256Wrapped(pWorkerA, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (-6548), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_control = LoadBlock256Wrapped(pSource, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + (1181), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));
    const auto storm_mask_a = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackA, chunk + 1075u);
    const auto storm_mask_b = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackB, chunk + 1075u + 29u);
    HurricaneMatrix storm(storm_block.data());
    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 0xFFU);
    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 0xFFU);
    storm.InjectAdd(pSalt, kSaltBytes, 23u);
    switch (static_cast<unsigned>(storm_control[13] ^ storm_mask_a[17] ^ storm_mask_b[19] ^ pSalt[2]) % 5u) {
      case 0u: storm.ApplyFastOp(HurricaneFastOp::kRotateRowRight, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 1u: storm.ApplyFastOp(HurricaneFastOp::kRotateColumnDown, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 2u: storm.ApplyFastOp(HurricaneFastOp::kSwapColumns, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 3u: storm.ApplyFastOp(HurricaneFastOp::kRotateRowLeft, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
      case 4u: storm.ApplyFastOp(HurricaneFastOp::kAddColumnIntoColumn, static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37]), static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])); break;
    }
    switch (static_cast<unsigned>(storm_control[53] ^ storm_mask_a[59] ^ storm_mask_b[61] ^ pSalt[3]) % 5u) {
      case 0u: storm.ApplySlowOp(HurricaneSlowOp::kRotateLeft, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
      case 1u: storm.ApplySlowOp(HurricaneSlowOp::kFlipHorizontal, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
      case 2u: storm.ApplySlowOp(HurricaneSlowOp::kFlipVertical, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
      case 3u: storm.ApplySlowOp(HurricaneSlowOp::kRotateRight, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
      case 4u: storm.ApplySlowOp(HurricaneSlowOp::kTranspose, static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71]), static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])); break;
    }
    storm.ApplySlowOp(static_cast<HurricaneSlowOp>((storm_control[79] ^ storm_mask_a[83] ^ storm_mask_b[89] ^ pSalt[4]) % 8U), static_cast<std::uint8_t>(storm_control[89] + storm_mask_a[97] + storm_mask_b[101]), static_cast<std::uint8_t>(storm_block[101] ^ storm_mask_a[103] ^ storm_mask_b[107]));
    storm.Store(pWorkerA + chunk);
    for (std::size_t aLane = 0; aLane < kHurricaneBlockBytes; ++aLane) {
      pWorkerA[chunk + aLane] = static_cast<std::uint8_t>(pWorkerA[chunk + aLane] + storm_control[(aLane + 11U) & 255U] + storm_mask_a[(aLane + 17U) & 255U] + storm_mask_b[(aLane + 29U) & 255U] + pSalt[aLane & 31U]);
    }
  }

  TwistCandidate_142215_ApplyStepMixPulse(pSource, pWorkerA, pWorkerB, pSalt, pRound, pBreakerTempA, pBreakerTempB);

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex0 = WrapRange(aReverseIndex + (129), aStart, aEnd);
      const int aIndex1 = WrapRange(aReverseIndex + (4331), aStart, aEnd);
      const int aIndex2 = WrapRange(aReverseIndex + (3154), aStart, aEnd);
      const int aControlIndex = WrapRange(i + (2395), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorkerB[aIndex2]);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 1u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 1u)));
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
  TwistCandidate_142215_FinalWhitening(pSource, pDest, pSalt, pRound, aTwiddleA, aTwiddleB);

}

static void TwistCandidate_142215_PushKeyRound(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 28u + (1486)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 19u + (5767)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 19u + (-3432)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 28u + static_cast<unsigned int>(11u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(5u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 1u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 1u)));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a + b + aSaltByte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(aMixValue);
    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ aKeyByte) & 0xFFu);
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

static void TwistCandidate_142215_PushMaskRoundA(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 10u + (-5828)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 30u + (-1185)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 7u + (-5262)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(206u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(206u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] ^= static_cast<unsigned char>((((a + c) ^ b) + aOtherMask) & 0xFFu);
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

static void TwistCandidate_142215_PushMaskRoundB(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 4u + (-3867)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 26u + (2337)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 27u + (-2632)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(1u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(1u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] = static_cast<unsigned char>(pNextRoundMaskBuffer[aMaskIndex] + static_cast<unsigned char>((((a ^ c) + b) ^ aOtherMask) & 0xFFu));
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

void TwistCandidate_142215(
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
  TwistCandidate_142215_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_142215_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_142215_MaskSeedA(pSource, pWorkerA, pMaskStackA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_142215_MaskSeedB(pSource, pWorkerB, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_142215_TwistBlock(aRoundSource, pWorkerA, pWorkerB, aRoundDest, aBreakerTempA, aBreakerTempB, aRound, aSalt, pKeyStack, pMaskStackA, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_142215_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_142215_PushMaskRoundA(aRoundDest, pMaskStackA, pMaskStackB, pNextRoundMaskBufferA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_142215_PushMaskRoundB(aRoundDest, pMaskStackB, pMaskStackA, pNextRoundMaskBufferB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

const RegisteredCandidate kRegisteredCandidates[] = {
  {
    142215,
    "TwistCandidate_142215",
    0,
    0,
    0,
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    "TwistCandidate_142215",
    &TwistCandidate_142215,
    &TwistCandidate_142215_KeySeed,
    &TwistCandidate_142215_SaltSeed,
    &TwistCandidate_142215_MaskSeedA,
    &TwistCandidate_142215_MaskSeedB,
    &TwistCandidate_142215_TwistBlock,
    &TwistCandidate_142215_PushKeyRound,
    &TwistCandidate_142215_PushMaskRoundA,
    &TwistCandidate_142215_PushMaskRoundB,
  }
};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}  // namespace twist
