#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistBreakers.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {

// Candidate 186612: TwistCandidate_186612
// family=ferocious_dual_worker op_budget=7 worker_shapes=1x3/1x2 worker_reverse=(True, True, False, True)/(True, False, False, True) twiddle1=xor/i_mul twiddle2=xor/none twiddle2_source=A tsunami=custom breaker_shapes=aBreakerAB:4m1i1v302,aBreakerCD:4m3i2v302 lane_breaker=true braid_breaker=false jump_breaker=false swap_breaker=true mask_template=0 mask_seed_a=sbox_wave mask_seed_b=off lightning=false/1 typhoon=false hurricane=false final_reverse=(False, True, False, False)
// ferocious[wa=1x3; wb=1x2; tw1=xor/i_mul; tw2=xor/none; tw2src=A; saltbox=128:000fae37:666ae724; tsunami=custom; final_whitening=custom; breaker_shapes=aBreakerAB:4m1i1v302,aBreakerCD:4m3i2v302; wa_rev=(True, True, False, True); wb_rev=(True, False, False, True); lane=on:3; braid=off:0; jump=off:0; swap=on:1; mask=workerax0; lightning=off:1; typhoon=off; hurricane=off; final=1/(False, True, False, False); key_rot=5; maskA_seed=sbox_wave/True; maskB_seed=off; maskA_bias=119; maskB_bias=22]
static std::uint32_t TwistCandidate_186612_AdvanceTwiddle32A(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState ^ 0x7DC44C82u;
  aState = AdvanceTwiddle32(
      aState,
      pValue ^ 0xD862D12Eu,
      pExtra + 0xEBBEED1Au,
      0x281B326Eu,
      10u);
  aState ^= RotateLeft32(pValue + pExtra + 0x33DC3C34u, 12U);
  return static_cast<std::uint32_t>(aState + 0x55DF7EECu);
}

static std::uint32_t TwistCandidate_186612_AdvanceTwiddle32B(
    std::uint32_t pState,
    std::uint32_t pValue,
    std::uint32_t pExtra) {
  std::uint32_t aState = pState + 0xEBBEED1Au;
  aState = AdvanceTwiddle32(
      aState ^ RotateLeft32(pExtra, 5U),
      pValue + 0x281B326Eu,
      pExtra ^ 0x7DC44C82u,
      0xD862D12Eu,
      11u);
  aState ^= RotateLeft32(pValue ^ pExtra ^ 0x56271DB0u, 11U);
  return static_cast<std::uint32_t>(aState ^ 0xC3A5DF74u);
}

static void TwistCandidate_186612_ApplySaltMixBox(
    unsigned char (&pSalt)[kSaltBytes],
    std::uint32_t pState,
    std::uint32_t pBias,
    unsigned pRotate) {
  std::uint32_t aTwiddleA = TwistCandidate_186612_AdvanceTwiddle32A(
      pState ^ pBias,
      static_cast<std::uint32_t>(pSalt[(pBias + 3U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 17U));
  std::uint32_t aTwiddleB = TwistCandidate_186612_AdvanceTwiddle32B(
      pState + pBias + static_cast<std::uint32_t>(pRotate),
      static_cast<std::uint32_t>(pSalt[(pBias + 11U) & 31U]),
      static_cast<std::uint32_t>(pRotate + 29U));
  for (std::size_t aIndex = 0; aIndex < kSaltBytes; ++aIndex) {
    const unsigned char aSaltA = pSalt[aIndex];
    const unsigned char aSaltB = pSalt[(aIndex + 7U + (FoldWordToByte(aTwiddleA) & 3U)) & 31U];
    const unsigned char aSaltC = pSalt[(aIndex + 13U + (FoldWordToByte(aTwiddleB) & 3U)) & 31U];
    aTwiddleA = TwistCandidate_186612_AdvanceTwiddle32A(
        aTwiddleA ^ static_cast<std::uint32_t>(aSaltB),
        static_cast<std::uint32_t>(aSaltA) ^ pBias,
        static_cast<std::uint32_t>(aSaltC) ^ static_cast<std::uint32_t>(aIndex * 17U + pRotate));
    aTwiddleB = TwistCandidate_186612_AdvanceTwiddle32B(
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

static void TwistCandidate_186612_SaltSeed(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {
    return;
  }
  static constexpr unsigned char aSaltMixBox[128] = {
      0xE9U, 0x2AU, 0x23U, 0x3BU, 0x50U, 0x0DU, 0xFCU, 0x27U, 0xDAU, 0x3FU, 0xD4U, 0xF5U, 0x54U, 0x9FU, 0xCCU, 0xC7U,
      0xABU, 0x49U, 0x42U, 0xE3U, 0x8EU, 0x5CU, 0x60U, 0xB3U, 0xDCU, 0x7AU, 0x1EU, 0xBDU, 0x46U, 0xB6U, 0x78U, 0xD2U,
      0x6FU, 0xBCU, 0x6CU, 0x61U, 0xA2U, 0x7FU, 0xECU, 0x0BU, 0xD3U, 0xDBU, 0xADU, 0x38U, 0x33U, 0xE7U, 0x1BU, 0x03U,
      0xA5U, 0x8FU, 0xA0U, 0xC5U, 0x02U, 0x56U, 0x9AU, 0x97U, 0x4FU, 0x7EU, 0xEFU, 0x80U, 0x92U, 0x77U, 0xFBU, 0xBEU,
      0xF2U, 0x07U, 0x13U, 0xEEU, 0xA8U, 0xE6U, 0x00U, 0x53U, 0x8CU, 0x66U, 0x5FU, 0x36U, 0xD7U, 0x18U, 0x4AU, 0x2FU,
      0x57U, 0xEAU, 0xC1U, 0xB1U, 0x5AU, 0x28U, 0x4EU, 0xEDU, 0x5BU, 0x9DU, 0xA3U, 0x39U, 0x73U, 0x12U, 0x74U, 0x17U,
      0xC0U, 0xF9U, 0x10U, 0x71U, 0xA6U, 0x1CU, 0xE4U, 0x4CU, 0x51U, 0xAAU, 0xE0U, 0xE5U, 0xD5U, 0x52U, 0xDDU, 0x08U,
      0xA7U, 0x47U, 0x2CU, 0x59U, 0x96U, 0x0EU, 0xB8U, 0x24U, 0xAEU, 0xF8U, 0x0CU, 0x86U, 0x87U, 0x20U, 0x44U, 0xC4U
  };
  std::memset(pSalt, 0, kSaltBytes);
  std::uint32_t aSaltAcc = static_cast<std::uint32_t>(0xA5A5A5A5u ^ 45u ^ 251u);
  unsigned int aSourceIndex = 0U;
  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 29u + (-6465)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 7u + (2556)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = (aSourceIndex * 29u + static_cast<unsigned int>(45u)) & 31U;
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^
        static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aB & 0xFFu), 5u)) ^
        static_cast<std::uint32_t>(aSourceIndex + 45u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aSaltWave, aSaltIndex, aA ^ aSaltWave, aB, static_cast<std::uint32_t>(aSourceIndex), static_cast<std::uint32_t>(251u), 5u, false);
    pSalt[(aSaltIndex + 17U) & 31U] ^= static_cast<unsigned char>(aSaltMixBox[(
        aSaltWave ^ pSalt[(aSaltIndex + 5U) & 31U] ^ static_cast<std::uint32_t>(aSourceIndex)) & 127U]);
    ++aSourceIndex;
  }
  TwistCandidate_186612_ApplySaltMixBox(pSalt, aSaltAcc, static_cast<std::uint32_t>(251u), 5u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] ^ pSalt[(aSaltLane + 7U) & 31U] ^ static_cast<unsigned char>(251u) ^ static_cast<unsigned char>(aSaltLane)) & 127U]);
    pSalt[aSaltLane] = static_cast<unsigned char>(
        FixedMixBoxByte(static_cast<unsigned char>(pSalt[aSaltLane] ^ aSaltWave ^ pSalt[(aSaltLane + 13U) & 31U])) +
        static_cast<unsigned char>(aSaltWave));
  }
  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 26u + (-5128)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 40u + (1219)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aSaltIndex = static_cast<unsigned int>((aSourceIndex * 26u + pSalt[(aSourceIndex + 177u) & 31U]) & 31U);
    const std::uint32_t aA = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t aB = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pSalt[(aSaltIndex + 11U) & 31U]);
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        aA ^ aB ^ aCarry ^ static_cast<std::uint32_t>(177u)) & 127U]);
    aSaltAcc = AdvanceSaltSeedAccumulator(pSalt, aSaltAcc ^ aCarry ^ aSaltWave, aSaltIndex, aA ^ aCarry, aB, static_cast<std::uint32_t>(aSourceIndex + aCarry), static_cast<std::uint32_t>(177u), 4u, false);
    pSalt[(aSaltIndex + 23U) & 31U] = static_cast<unsigned char>(pSalt[(aSaltIndex + 23U) & 31U] + static_cast<unsigned char>(aSaltWave));
  }
  TwistCandidate_186612_ApplySaltMixBox(pSalt, aSaltAcc ^ static_cast<std::uint32_t>(177u), static_cast<std::uint32_t>(177u), 4u);
  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {
    const std::uint32_t aSaltWave = static_cast<std::uint32_t>(aSaltMixBox[(
        pSalt[aSaltLane] + pSalt[(aSaltLane + 19U) & 31U] + static_cast<unsigned char>(177u + 13u)) & 127U]);
    pSalt[aSaltLane] ^= static_cast<unsigned char>(aSaltWave);
  }
}

static void TwistCandidate_186612_KeySeed(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 16u + (-1137)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 28u + (4712)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pKeyStack[aKeyPlaneIndex][aKeyIndex]);
    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);
    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 37u) ^ aLane ^ static_cast<std::uint32_t>(aKeyIndex * 13u) ^ static_cast<std::uint32_t>(aKeyPlaneIndex * 29u)) & 0xFFu);
    const std::uint32_t aPreMix = static_cast<std::uint32_t>((((a + 45u) ^ ((b << 5u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu));
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

static void TwistCandidate_186612_MaskSeedA(
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
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 94u ^ 119u);
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 17u + (409)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 24u + (2295)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aReverseIndex * (17u + 24u) + (3363)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const int aWorkerIndex = WrapRange(static_cast<int>(aReverseIndex * 12u + (5491)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t aWorkerByte = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;
    const std::uint32_t aTurbulence = static_cast<std::uint32_t>(aMixBox[(aWorkerByte ^ a ^ static_cast<std::uint32_t>(212u)) & 0xFFu]);
    const std::uint32_t aSeedA = static_cast<std::uint32_t>((a ^ aTurbulence) & 0xFFu);
    const std::uint32_t aSeedB = static_cast<std::uint32_t>((b + aTurbulence + aWorkerByte) & 0xFFu);
    const std::uint32_t aSeedC = static_cast<std::uint32_t>((c ^ static_cast<std::uint32_t>(aMixBox[(aWorkerByte + c + static_cast<std::uint32_t>(212u)) & 0xFFu])) & 0xFFu);
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 17u * 8U + static_cast<std::size_t>(aSourceIndex) * 24u + 94u + static_cast<std::size_t>(aTurbulence * ((12u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(94u), 2u, true);
    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + 212u) * ((12u & 15U) + 1U))) % (kMaskStackTotalBytes * 8U);
    UpdateMaskSeedBit(pMaskStackA, aTurbulenceBit, aMaskSeedState ^ aTurbulence ^ aWorkerByte, 6u, true);
  }
  for (int aLoopIndex = 0; aLoopIndex != static_cast<int>(PASSWORD_EXPANDED_SIZE); aLoopIndex += 1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 61u + (409)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aSourceIndex * 55u + (2295)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aSourceIndex * (61u + 55u) + (3363)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const int aWorkerIndex = WrapRange(static_cast<int>(aReverseIndex * 12u + (5491)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t aWorkerByte = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;
    const std::uint32_t aTurbulence = static_cast<std::uint32_t>(aMixBox[(aWorkerByte ^ a ^ static_cast<std::uint32_t>(212u)) & 0xFFu]);
    const std::uint32_t aSeedA = static_cast<std::uint32_t>((a ^ aTurbulence) & 0xFFu);
    const std::uint32_t aSeedB = static_cast<std::uint32_t>((b + aTurbulence + aWorkerByte) & 0xFFu);
    const std::uint32_t aSeedC = static_cast<std::uint32_t>((c ^ static_cast<std::uint32_t>(aMixBox[(aWorkerByte + c + static_cast<std::uint32_t>(212u)) & 0xFFu])) & 0xFFu);
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 61u * 8U + static_cast<std::size_t>(aSourceIndex) * 55u * 3U + 123u + static_cast<std::size_t>(aTurbulence * ((12u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStackA[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackA, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), 2u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>(123u), 2u, true);
    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + 212u) * ((12u & 31U) + 1U))) % (kMaskStackTotalBytes * 8U);
    UpdateMaskSeedBit(pMaskStackA, aTurbulenceBit, RotateLeft32(aMaskSeedState ^ aTurbulence ^ aCarry, 7U), 2u, true);
  }
}

static void TwistCandidate_186612_MaskSeedB(
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
  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ 170u ^ 22u);
  for (int aLoopIndex = 0; aLoopIndex != static_cast<int>(PASSWORD_EXPANDED_SIZE); aLoopIndex += 1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex0 = WrapRange(static_cast<int>(aReverseIndex * 15u + (4671)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 21u + (7335)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aReverseIndex * (15u + 21u) + (-2462)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aReverseIndex) * 15u * 8U + static_cast<std::size_t>(aSourceIndex) * 21u + 170u + static_cast<std::size_t>(aTurbulence * ((53u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackB, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>(170u), 6u, false);
  }
  for (int aLoopIndex = static_cast<int>(PASSWORD_EXPANDED_SIZE) - 1; aLoopIndex != -1; aLoopIndex += -1) {
    const unsigned int aSourceIndex = static_cast<unsigned int>(aLoopIndex);
    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);
    const int aIndex3 = WrapRange(static_cast<int>(aSourceIndex * 48u + (4671)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex4 = WrapRange(static_cast<int>(aReverseIndex * 46u + (7335)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex5 = WrapRange(static_cast<int>(aReverseIndex * (48u + 46u) + (-2462)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);
    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);
    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);
    const std::uint32_t aWorkerByte = 0U;
    const std::uint32_t aTurbulence = 0U;
    const std::uint32_t aSeedA = a;
    const std::uint32_t aSeedB = b;
    const std::uint32_t aSeedC = c;
    const std::size_t aFlatBit = (static_cast<std::size_t>(aSourceIndex) * 48u * 8U + static_cast<std::size_t>(aReverseIndex) * 46u * 3U + 74u + static_cast<std::size_t>(aTurbulence * ((53u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);
    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;
    const std::uint32_t aCarry = static_cast<std::uint32_t>(pMaskStackB[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);
    aMaskSeedState = AdvanceMaskSeedBitstream(pMaskStackB, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), 1u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>(74u), 1u, false);
  }
}

static void TwistCandidate_186612_TsunamiBreaker(
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
      0xFBU, 0x58U, 0x8AU, 0x98U, 0xB8U, 0x7FU, 0xEAU, 0x38U, 0xB7U, 0x54U, 0x7BU, 0xF5U, 0x48U, 0xE3U, 0x3EU, 0xA9U,
      0x73U, 0x1BU, 0xA2U, 0xCEU, 0x0CU, 0x5BU, 0x08U, 0x0BU, 0xEEU, 0xDAU, 0x25U, 0x4FU, 0xD0U, 0x4AU, 0xB2U, 0x4BU,
      0x41U, 0x1DU, 0xC2U, 0xADU, 0x83U, 0x3AU, 0xC8U, 0x0FU, 0x43U, 0x77U, 0x5CU, 0xF4U, 0x86U, 0xD3U, 0x39U, 0x9EU,
      0xF2U, 0x32U, 0x1AU, 0x27U, 0x9FU, 0x9BU, 0x80U, 0x01U, 0x12U, 0xF7U, 0x7EU, 0xD8U, 0xABU, 0x0DU, 0xE8U, 0x4EU,
      0x66U, 0x7CU, 0x94U, 0xF6U, 0xDDU, 0x55U, 0x6BU, 0x8BU, 0x6FU, 0xAFU, 0x6AU, 0x2EU, 0x8CU, 0xDFU, 0x67U, 0xE6U,
      0x06U, 0x07U, 0x76U, 0x02U, 0xE7U, 0xD4U, 0x9CU, 0xD2U, 0x5FU, 0x87U, 0x85U, 0xB4U, 0x40U, 0x91U, 0x8EU, 0x0EU,
      0x23U, 0x8FU, 0xBAU, 0x28U, 0xEDU, 0x61U, 0xFAU, 0xCDU, 0x57U, 0x2BU, 0xA6U, 0x7DU, 0x59U, 0x50U, 0x42U, 0x90U,
      0xD5U, 0x78U, 0xD6U, 0xDEU, 0x1EU, 0x84U, 0xDBU, 0x3BU, 0x5EU, 0x81U, 0x16U, 0xE5U, 0x10U, 0x2DU, 0x05U, 0x97U
  };
  std::uint32_t aAccA = AdvanceTwiddle32(0x776B2BBBu ^ static_cast<std::uint32_t>(pRound), pTwiddleA, pTwiddleB, 0xCF0D4B1Cu, 3u);
  std::uint32_t aAccB = AdvanceTwiddle32(0x986B72ECu ^ static_cast<std::uint32_t>(pRound), pTwiddleB, pTwiddleA, 0xA6954F4Cu, 4u);
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-6813), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aSourceIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (4050), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-7258), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aWorkerIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (-2284), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::uint32_t aSourceA = static_cast<std::uint32_t>(pSource[aSourceIndexA]);
    const std::uint32_t aSourceB = static_cast<std::uint32_t>(pSource[aSourceIndexB]);
    const std::uint32_t aTapA = static_cast<std::uint32_t>(pWorkerA[aWorkerIndexA]);
    const std::uint32_t aTapB = static_cast<std::uint32_t>(pWorkerB[aWorkerIndexB]);
    const std::uint32_t aSaltA = static_cast<std::uint32_t>(pSalt[(i + 27u) & 31U]);
    const std::uint32_t aSaltB = static_cast<std::uint32_t>(pSalt[(i + 18u) & 31U]);
    const std::uint32_t aBoxA = static_cast<std::uint32_t>(aMixBox[(aSourceA ^ aSaltA ^ aTapB ^ FoldWordToByte(aAccA)) & 127U]);
    const std::uint32_t aBoxB = static_cast<std::uint32_t>(aMixBox[(aSourceB + aSaltB + aTapA + FoldWordToByte(aAccB)) & 127U]);
    aAccA = AdvanceTwiddle32(aAccA ^ pTwiddleA, aTapA ^ aBoxA, aSourceB ^ aSaltB, 0xCF0D4B1Cu, 3u);
    aAccB = AdvanceTwiddle32(aAccB ^ pTwiddleB, aTapB ^ aBoxB, aSourceA ^ aSaltA, 0xA6954F4Cu, 4u);
    const unsigned char aWaveA = aMixBox[(FoldWordToByte(aAccA) ^ aSourceA ^ aSaltB ^ static_cast<unsigned char>(i)) & 127U];
    const unsigned char aWaveB = aMixBox[(FoldWordToByte(aAccB) ^ aSourceB ^ aSaltA ^ static_cast<unsigned char>(i >> 1U)) & 127U];
    switch ((static_cast<unsigned>(aBoxA ^ aBoxB ^ static_cast<std::uint32_t>(pRound)) + 0u) & 3U) {
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

static void TwistCandidate_186612_FinalWhitening(
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
      0x99U, 0x82U, 0xD1U, 0x96U, 0x38U, 0xBDU, 0x5BU, 0x23U, 0x6DU, 0xAAU, 0x7AU, 0xECU, 0xD0U, 0x45U, 0xA7U, 0x81U,
      0xDFU, 0xC5U, 0xB0U, 0x30U, 0xB8U, 0x17U, 0x2CU, 0x1DU, 0x57U, 0xE3U, 0x28U, 0xDEU, 0x56U, 0x29U, 0x60U, 0xEFU,
      0x79U, 0x71U, 0x0BU, 0x49U, 0xE7U, 0x33U, 0xE1U, 0xEBU, 0x0EU, 0x0FU, 0x1FU, 0xA8U, 0xA6U, 0x09U, 0xF4U, 0xB1U,
      0x88U, 0xF7U, 0x4EU, 0x01U, 0x0CU, 0x8DU, 0x89U, 0xABU, 0x5FU, 0x69U, 0x63U, 0x6FU, 0xCAU, 0x05U, 0x1CU, 0x02U,
      0xC2U, 0xC1U, 0x26U, 0x84U, 0x2FU, 0x12U, 0xFEU, 0xFCU, 0x90U, 0xE5U, 0xFDU, 0x78U, 0x65U, 0xA4U, 0x8BU, 0xA1U,
      0x46U, 0x2AU, 0x44U, 0x7BU, 0xADU, 0x85U, 0xEAU, 0x91U, 0x4FU, 0x48U, 0x22U, 0xBEU, 0x7EU, 0xF3U, 0x94U, 0xE2U,
      0xE6U, 0xD3U, 0xCDU, 0x58U, 0x74U, 0xD5U, 0x4BU, 0x3EU, 0xD2U, 0x66U, 0x9AU, 0xE4U, 0x31U, 0x8EU, 0x3AU, 0x32U,
      0x62U, 0xCCU, 0xD7U, 0xAEU, 0x93U, 0x7FU, 0x04U, 0xBFU, 0x83U, 0xF0U, 0x9CU, 0x77U, 0xA0U, 0x2BU, 0x50U, 0x13U
  };
  std::uint32_t aAcc = AdvanceTwiddle32(pTwiddleA ^ static_cast<std::uint32_t>(pRound), pTwiddleB, 0x2030765Eu, 0x7B6AC9FEu, 7u);
  // Final whitening
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::size_t aSourceIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + (3331), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t aNeighborIndex = (i + 5u) % PASSWORD_EXPANDED_SIZE;
    const std::uint32_t aSourceByte = static_cast<std::uint32_t>(pSource[aSourceIndex]);
    const std::uint32_t aDestByte = static_cast<std::uint32_t>(pDest[i]);
    const std::uint32_t aNeighborByte = static_cast<std::uint32_t>(pDest[aNeighborIndex]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[(i + 18u) & 31U]);
    const std::uint32_t aMix = static_cast<std::uint32_t>(aMixBox[(aDestByte ^ aSourceByte ^ aSaltByte ^ FoldWordToByte(aAcc)) & 127U]);
    aAcc = AdvanceTwiddle32(aAcc, aDestByte ^ aMix, aSourceByte ^ aNeighborByte ^ aSaltByte, 0x7B6AC9FEu, 7u);
    const unsigned char aWave = aMixBox[(FoldWordToByte(aAcc) + aSourceByte + aNeighborByte + aSaltByte) & 127U];
    switch ((static_cast<unsigned>(aMix) + 2u) % 3U) {
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

static void TwistCandidate_186612_ApplyDualWorkerMatrixBreaker(
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
  std::uint32_t aSequenceA = TwistCandidate_186612_AdvanceTwiddle32A(
      0x2C472D38u ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pRecipe.fast_rule) ^ static_cast<std::uint32_t>(pRecipe.key_row),
      static_cast<std::uint32_t>(pRecipe.slow_rule) ^ static_cast<std::uint32_t>(pRecipe.key_offset));
  std::uint32_t aSequenceB = TwistCandidate_186612_AdvanceTwiddle32B(
      0xE45C0B58u ^ static_cast<std::uint32_t>(pRound),
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
    aSequenceA = TwistCandidate_186612_AdvanceTwiddle32A(
        aSequenceA ^ aSelector,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ aKey[1] ^ aPartner[2]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk));
    aSequenceB = TwistCandidate_186612_AdvanceTwiddle32B(
        aSequenceB ^ aSlowSelector,
        static_cast<std::uint32_t>(pBreakerTempB[3] ^ aMaskA[4] ^ aMaskB[5]),
        aSourceFold ^ static_cast<std::uint32_t>(chunk >> 4U));
  }
}

static void TwistCandidate_186612_ApplyStepMixPulse(
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
  std::uint32_t aPulseA = TwistCandidate_186612_AdvanceTwiddle32A(
      0xCEFA49CAu ^ static_cast<std::uint32_t>(pRound),
      static_cast<std::uint32_t>(pSalt[10U]),
      0x0C4A49FBu ^ static_cast<std::uint32_t>(pSalt[28U]));
  std::uint32_t aPulseB = TwistCandidate_186612_AdvanceTwiddle32B(
      0x40649C9Cu + static_cast<std::uint32_t>(pRound * 17U),
      static_cast<std::uint32_t>(pSalt[28U]),
      0x67B09FCDu ^ static_cast<std::uint32_t>(pSalt[10U]));
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
          pSalt[(aLane + 10U + (aPulseByteB & 3U)) & 31U] ^
          aPulseByteA ^
          static_cast<unsigned char>(pRound + static_cast<unsigned int>(aLane))));
      const unsigned char aMix = FixedMixBoxByte(static_cast<unsigned char>(
          aWave +
          aWorkerBlockA[(aLane + 5U + (aPulseByteB & 1U)) & 15U] +
          RotateLeft8(aWorkerBlockB[(aLane + 9U + (aPulseByteA & 1U)) & 15U], 1U + ((aPulseByteA >> 5U) & 3U)) +
          pSalt[(aLane + 28U + (aPulseByteA & 3U)) & 31U]));
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
    aPulseA = TwistCandidate_186612_AdvanceTwiddle32A(
        aPulseA ^ aPulseFold,
        static_cast<std::uint32_t>(pBreakerTempA[0] ^ pBreakerTempB[5]),
        static_cast<std::uint32_t>(chunk + static_cast<std::size_t>(aPulseByteB)));
    aPulseB = TwistCandidate_186612_AdvanceTwiddle32B(
        aPulseB ^ RotateLeft32(aPulseFold, 7U),
        static_cast<std::uint32_t>(pBreakerTempB[3] + pBreakerTempA[9]),
        static_cast<std::uint32_t>((chunk >> 4U) + static_cast<std::size_t>(aPulseByteA)));
  }
}

static void TwistCandidate_186612_TwistBlock(
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
  std::uint32_t aTwiddleA = static_cast<std::uint32_t>(0xA511E9B3u ^ 0xFFE0821Du ^ (pRound * 24u));
  aTwiddleA ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (-1137), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleA ^= static_cast<std::uint32_t>(pSalt[(pRound + 0u) & 31U]) << 16U;
  std::uint32_t aTwiddleB = static_cast<std::uint32_t>(0xB44B1D93u ^ 0xFFE0821Du ^ (pRound * 24u));
  aTwiddleB ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + (4712), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;
  aTwiddleB ^= static_cast<std::uint32_t>(pSalt[(pRound + 11u) & 31U]) << 16U;
  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(aReverseIndex + (7375), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(aReverseIndex + (3312), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerA[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 12u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 16u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 103u + 16u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (3312 + 16), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackA[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackB[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 12u + 3U) & 15U), static_cast<std::size_t>(i + 26u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 15u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = static_cast<std::uint32_t>(((aLaneIndex * static_cast<std::uint32_t>(29u)) ^ static_cast<std::uint32_t>(68u)) & 0xFFu);
      const std::uint32_t aRoute = ((aSourceCtrl >> 1u) ^ aWorkerCtrl ^ aKeyByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = static_cast<std::uint32_t>(a ^ aKeyByte); break;
        case 2U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (aKeyByte & (~aMaskByte & 0xFFu))); break;
        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (aKeyByte & 0x0Fu)); break;
      }
      pWorkerA[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = AdvanceTwiddle32(aTwiddleA, static_cast<std::uint32_t>(aValue), static_cast<std::uint32_t>((aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias) ^ static_cast<std::uint32_t>(139u)), 0x85EBCA6Bu, 4u);
    }
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex1 = WrapRange(aReverseIndex + (1274), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex1]);
      const int aControlIndex = WrapRange(aReverseIndex + (-358), aStart, aEnd);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>(pWorkerB[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 3u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 27u)));
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 96u + 24u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + (-358 + 24), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackB[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackA[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 3u + 3U) & 15U), static_cast<std::size_t>(i + 15u)));
      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + 29u) & 31U]);
      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);
      const std::uint32_t aPositionBias = 0U;
      const std::uint32_t aRoute = ((aSourceCtrl >> 3u) ^ aWorkerCtrl ^ aMaskByte) & 3U;
      std::uint32_t aValue = a;
      switch (aRoute) {
        case 0U: aValue = a; break;
        case 1U: aValue = static_cast<std::uint32_t>(a ^ aMaskByte); break;
        case 2U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (aMaskByte & (~aMaskByte & 0xFFu))); break;
        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (aMaskByte & 0x0Fu)); break;
      }
      pWorkerB[i] = static_cast<std::uint8_t>(aValue);
      aTwiddleA = AdvanceTwiddle32(aTwiddleA, static_cast<std::uint32_t>(aValue), static_cast<std::uint32_t>((aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias) ^ static_cast<std::uint32_t>(71u)), 0x85EBCA6Bu, 7u);
    }
  }

  TwistCandidate_186612_TsunamiBreaker(pSource, pWorkerA, pWorkerB, pSalt, pRound, aTwiddleA, aTwiddleB);

  {
    const DualWorkerBreakerRecipe4 aBreakerAB{
        {7375, -1423, 4236, -5441},
        3312,
        4,
        static_cast<std::size_t>(375u),
        static_cast<std::uint8_t>(12u),
        static_cast<std::uint8_t>(16u),
        static_cast<std::uint8_t>(17u),
        static_cast<std::uint8_t>(7u),
        static_cast<std::uint8_t>(7u),
        static_cast<std::uint8_t>(107u),
        static_cast<std::uint8_t>(103u),
        static_cast<std::uint8_t>(8u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(2u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(1u),
    };
    TwistCandidate_186612_ApplyDualWorkerMatrixBreaker(aBreakerAB, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound, pBreakerTempA, pBreakerTempB);
  }

  {
    const DualWorkerBreakerRecipe4 aBreakerCD{
        {1274, 6132, -2250, 3647},
        6228,
        5901,
        static_cast<std::size_t>(102u),
        static_cast<std::uint8_t>(6u),
        static_cast<std::uint8_t>(27u),
        static_cast<std::uint8_t>(6u),
        static_cast<std::uint8_t>(8u),
        static_cast<std::uint8_t>(9u),
        static_cast<std::uint8_t>(169u),
        static_cast<std::uint8_t>(55u),
        static_cast<std::uint8_t>(10u),
        static_cast<std::uint8_t>(1u),
        static_cast<std::uint8_t>(3u),
        static_cast<std::uint8_t>(0u),
        static_cast<std::uint8_t>(2u),
        static_cast<std::uint8_t>(2u),
        static_cast<std::uint8_t>(3u),
    };
    TwistCandidate_186612_ApplyDualWorkerMatrixBreaker(aBreakerCD, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound, pBreakerTempA, pBreakerTempB);
  }

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aIndex0 = WrapRange(i + (-1794), aStart, aEnd);
      const int aIndex1 = WrapRange(i + (4616), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (-858), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pWorkerA[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerB[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);
      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * 17u + 1038u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;
      const int aMaskFlatOther = WrapRange(static_cast<int>(aMaskFlat) + (3 + 0), 0, static_cast<int>(kMaskStackTotalBytes));
      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>(pMaskStackA[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);
      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>(pMaskStackB[(static_cast<std::size_t>(aMaskFlatOther) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatOther) % kMaskBytes]);
      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 5U) ^ (aMaskByteB << 1U)) & 0xFFu);
      const std::uint32_t aRoute = (c ^ aMaskByte ^ aMaskByteB ^ static_cast<std::uint32_t>(3u)) % 5U;
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
      pWorkerA[i] = static_cast<std::uint8_t>(aOutA);
      pWorkerB[i] = static_cast<std::uint8_t>(aOutB);
    }
  }

  TwistCandidate_186612_ApplyStepMixPulse(pSource, pWorkerA, pWorkerB, pSalt, pRound, pBreakerTempA, pBreakerTempB);

  {
    const int aStart = 0;
    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);
    for (int i = aStart; i < aEnd; ++i) {
      const int aReverseIndex = aEnd - 1 - i;
      const int aIndex0 = WrapRange(i + (955), aStart, aEnd);
      const int aIndex1 = WrapRange(aReverseIndex + (-5876), aStart, aEnd);
      const int aIndex2 = WrapRange(i + (-5821), aStart, aEnd);
      const int aControlIndex = WrapRange(i + (-1372), aStart, aEnd);
      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);
      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);
      const std::uint32_t c = static_cast<std::uint32_t>(pWorkerB[aIndex2]);
      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);
      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
          pKeyStack,
          static_cast<std::size_t>((pRound + 9u + static_cast<unsigned int>(i)) & 15U),
          static_cast<std::size_t>(i + 6u)));
      const std::uint32_t aRoute = (aSourceCtrl ^ aKeyByte ^ static_cast<std::uint32_t>(1u)) % 6U;
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
  TwistCandidate_186612_FinalWhitening(pSource, pDest, pSalt, pRound, aTwiddleA, aTwiddleB);

}

static void TwistCandidate_186612_PushKeyRound(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 18u + (3365)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 17u + (-7021)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 15u + (-6376)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aKeyIndex = (aSourceIndex * 18u + static_cast<unsigned int>(5u)) & 31U;
    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>(9u)) & 31U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);
    const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(
        pKeyStack,
        static_cast<std::size_t>((aSourceIndex + 11u) & 15U),
        static_cast<std::size_t>(aSourceIndex + 251u)));
    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a ^ b ^ aSaltByte) & 0xFFu);
    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(aMixValue));
    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ aKeyByte) & 0xFFu));
    ++aSourceIndex;
  }
  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);
}

static void TwistCandidate_186612_PushMaskRoundA(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-3995)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-5813)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 1u + (6463)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(119u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(119u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] = static_cast<unsigned char>(pNextRoundMaskBuffer[aMaskIndex] + static_cast<unsigned char>((((a ^ c) + b) ^ aOtherMask) & 0xFFu));
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

static void TwistCandidate_186612_PushMaskRoundB(
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
    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * 23u + (-4761)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * 12u + (-2784)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * 25u + (-2589)), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));
    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>(22u)) & 7U;
    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>(22u) + 3U) & 7U;
    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);
    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);
    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);
    const std::uint32_t aOtherMask = static_cast<std::uint32_t>(pMaskStackOther[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);
    pNextRoundMaskBuffer[aMaskIndex] = static_cast<unsigned char>(pNextRoundMaskBuffer[aMaskIndex] + static_cast<unsigned char>((((a ^ c) + b) ^ aOtherMask) & 0xFFu));
    ++aSourceIndex;
  }
  RotateMaskStack(pMaskStackSelf, pNextRoundMaskBuffer);
}

void TwistCandidate_186612(
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
  TwistCandidate_186612_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_186612_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_186612_MaskSeedA(pSource, pWorkerA, pMaskStackA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  TwistCandidate_186612_MaskSeedB(pSource, pWorkerB, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  unsigned int aRound = 0U;
  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U)
        ? pSource
        : (pDest + offset - PASSWORD_EXPANDED_SIZE);
    unsigned char* aRoundDest = pDest + offset;
    TwistCandidate_186612_TwistBlock(aRoundSource, pWorkerA, pWorkerB, aRoundDest, aBreakerTempA, aBreakerTempB, aRound, aSalt, pKeyStack, pMaskStackA, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_186612_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_186612_PushMaskRoundA(aRoundDest, pMaskStackA, pMaskStackB, pNextRoundMaskBufferA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
    TwistCandidate_186612_PushMaskRoundB(aRoundDest, pMaskStackB, pMaskStackA, pNextRoundMaskBufferB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));
  }
}

const RegisteredCandidate kRegisteredCandidates[] = {
  {
    186612,
    "TwistCandidate_186612",
    0,
    0,
    0,
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},
    "TwistCandidate_186612",
    &TwistCandidate_186612,
    &TwistCandidate_186612_KeySeed,
    &TwistCandidate_186612_SaltSeed,
    &TwistCandidate_186612_MaskSeedA,
    &TwistCandidate_186612_MaskSeedB,
    &TwistCandidate_186612_TwistBlock,
    &TwistCandidate_186612_PushKeyRound,
    &TwistCandidate_186612_PushMaskRoundA,
    &TwistCandidate_186612_PushMaskRoundB,
  }
};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}  // namespace twist
