#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace twist {

inline constexpr std::size_t PASSWORD_EXPANDED_SIZE = 7680;
inline constexpr std::size_t BLOCK_SIZE_L1 = 261120U;
inline constexpr std::size_t BLOCK_SIZE_L2 = 522240U;
inline constexpr std::size_t BLOCK_SIZE_L3 = 1044480U;
inline constexpr std::size_t kMatrixBlockBytes = 16;
inline constexpr std::size_t kTyphoonBlockBytes = 128;
inline constexpr std::size_t kHurricaneBlockBytes = 256;
inline constexpr std::size_t kRoundKeyBytes = 32;
inline constexpr std::size_t kRoundKeyStackDepth = 16;
inline constexpr std::size_t kMaskBytes = 8;
inline constexpr std::size_t kMaskStackDepth = 192;
inline constexpr std::size_t kMaskStackTotalBytes = kMaskBytes * kMaskStackDepth;
inline constexpr std::size_t kSaltBytes = 32;

using TwistFunction = void (*)(
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
    unsigned int pLength);

using KeySeedFunction = void (*)(
    unsigned char* pSource,
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned int pLength);

using SaltSeedFunction = void (*)(
    unsigned char* pSource,
    unsigned char (&pSalt)[kSaltBytes],
    unsigned int pLength);

using MaskSeedFunction = void (*)(
    unsigned char* pSource,
    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],
    unsigned int pLength);

using TwistBlockFunction = void (*)(
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
    unsigned int pLength);

using PushKeyRoundFunction = void (*)(
    unsigned char* pDest,
    const unsigned char (&pSalt)[kSaltBytes],
    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],
    unsigned int pLength);

using PushMaskRoundFunction = void (*)(
    unsigned char* pDest,
    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],
    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],
    unsigned int pLength);

inline std::uint32_t RotateLeft32(std::uint32_t value, unsigned int amount) {
  const unsigned int shift = amount & 31U;
  if (shift == 0U) {
    return value;
  }
  return static_cast<std::uint32_t>((value << shift) | (value >> (32U - shift)));
}

inline std::uint8_t RotateLeft8(std::uint8_t value, unsigned int amount) {
  const unsigned int shift = amount & 7U;
  if (shift == 0U) {
    return value;
  }
  return static_cast<std::uint8_t>((value << shift) | (value >> (8U - shift)));
}

inline unsigned char KeySeedMixBoxByte(unsigned char value) {
  static constexpr unsigned char kSBox[256] = {
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
      0x6BU, 0x5FU, 0xB8U, 0x9DU, 0x0BU, 0xF1U, 0xA5U, 0x64U, 0x2BU, 0x80U, 0x76U, 0xB5U, 0x21U, 0x65U, 0xFCU, 0xC8U,
  };
  return kSBox[value];
}

inline unsigned char MaskSeedMixBoxByte(unsigned char value) {
  static constexpr unsigned char kSBox[256] = {
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
      0x20U, 0x73U, 0xC8U, 0xBAU, 0xB1U, 0xBFU, 0x42U, 0x61U, 0x8FU, 0xCAU, 0xFFU, 0x9BU, 0x31U, 0xF6U, 0xEFU, 0x63U,
  };
  return kSBox[value];
}

inline unsigned char FixedMixBoxByte(unsigned char value) {
  static constexpr unsigned char kSBox[256] = {
      0x63U, 0x7CU, 0x77U, 0x7BU, 0xF2U, 0x6BU, 0x6FU, 0xC5U, 0x30U, 0x01U, 0x67U, 0x2BU, 0xFEU, 0xD7U, 0xABU, 0x76U,
      0xCAU, 0x82U, 0xC9U, 0x7DU, 0xFAU, 0x59U, 0x47U, 0xF0U, 0xADU, 0xD4U, 0xA2U, 0xAFU, 0x9CU, 0xA4U, 0x72U, 0xC0U,
      0xB7U, 0xFDU, 0x93U, 0x26U, 0x36U, 0x3FU, 0xF7U, 0xCCU, 0x34U, 0xA5U, 0xE5U, 0xF1U, 0x71U, 0xD8U, 0x31U, 0x15U,
      0x04U, 0xC7U, 0x23U, 0xC3U, 0x18U, 0x96U, 0x05U, 0x9AU, 0x07U, 0x12U, 0x80U, 0xE2U, 0xEBU, 0x27U, 0xB2U, 0x75U,
      0x09U, 0x83U, 0x2CU, 0x1AU, 0x1BU, 0x6EU, 0x5AU, 0xA0U, 0x52U, 0x3BU, 0xD6U, 0xB3U, 0x29U, 0xE3U, 0x2FU, 0x84U,
      0x53U, 0xD1U, 0x00U, 0xEDU, 0x20U, 0xFCU, 0xB1U, 0x5BU, 0x6AU, 0xCBU, 0xBEU, 0x39U, 0x4AU, 0x4CU, 0x58U, 0xCFU,
      0xD0U, 0xEFU, 0xAAU, 0xFBU, 0x43U, 0x4DU, 0x33U, 0x85U, 0x45U, 0xF9U, 0x02U, 0x7FU, 0x50U, 0x3CU, 0x9FU, 0xA8U,
      0x51U, 0xA3U, 0x40U, 0x8FU, 0x92U, 0x9DU, 0x38U, 0xF5U, 0xBCU, 0xB6U, 0xDAU, 0x21U, 0x10U, 0xFFU, 0xF3U, 0xD2U,
      0xCDU, 0x0CU, 0x13U, 0xECU, 0x5FU, 0x97U, 0x44U, 0x17U, 0xC4U, 0xA7U, 0x7EU, 0x3DU, 0x64U, 0x5DU, 0x19U, 0x73U,
      0x60U, 0x81U, 0x4FU, 0xDCU, 0x22U, 0x2AU, 0x90U, 0x88U, 0x46U, 0xEEU, 0xB8U, 0x14U, 0xDEU, 0x5EU, 0x0BU, 0xDBU,
      0xE0U, 0x32U, 0x3AU, 0x0AU, 0x49U, 0x06U, 0x24U, 0x5CU, 0xC2U, 0xD3U, 0xACU, 0x62U, 0x91U, 0x95U, 0xE4U, 0x79U,
      0xE7U, 0xC8U, 0x37U, 0x6DU, 0x8DU, 0xD5U, 0x4EU, 0xA9U, 0x6CU, 0x56U, 0xF4U, 0xEAU, 0x65U, 0x7AU, 0xAEU, 0x08U,
      0xBAU, 0x78U, 0x25U, 0x2EU, 0x1CU, 0xA6U, 0xB4U, 0xC6U, 0xE8U, 0xDDU, 0x74U, 0x1FU, 0x4BU, 0xBDU, 0x8BU, 0x8AU,
      0x70U, 0x3EU, 0xB5U, 0x66U, 0x48U, 0x03U, 0xF6U, 0x0EU, 0x61U, 0x35U, 0x57U, 0xB9U, 0x86U, 0xC1U, 0x1DU, 0x9EU,
      0xE1U, 0xF8U, 0x98U, 0x11U, 0x69U, 0xD9U, 0x8EU, 0x94U, 0x9BU, 0x1EU, 0x87U, 0xE9U, 0xCEU, 0x55U, 0x28U, 0xDFU,
      0x8CU, 0xA1U, 0x89U, 0x0DU, 0xBFU, 0xE6U, 0x42U, 0x68U, 0x41U, 0x99U, 0x2DU, 0x0FU, 0xB0U, 0x54U, 0xBBU, 0x16U,
  };
  return kSBox[value];
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

inline unsigned char MaskStackByte(
    const unsigned char pMaskStack[kMaskStackDepth][kMaskBytes],
    std::size_t row,
    std::size_t column) {
  if (pMaskStack == nullptr) {
    return 0U;
  }
  return pMaskStack[row % kMaskStackDepth][column % kMaskBytes];
}

inline std::array<unsigned char, kMatrixBlockBytes> LoadKeyStackBlock16Wrapped(
    const unsigned char pKeyStack[kRoundKeyStackDepth][kRoundKeyBytes],
    std::size_t row,
    std::size_t index) {
  std::array<unsigned char, kMatrixBlockBytes> bytes{};
  for (std::size_t lane = 0; lane < bytes.size(); ++lane) {
    bytes[lane] = KeyStackByte(pKeyStack, row, index + lane);
  }
  return bytes;
}

template <std::size_t BlockBytes>
inline std::array<unsigned char, BlockBytes> LoadMaskStackBlockWrapped(
    const unsigned char pMaskStack[kMaskStackDepth][kMaskBytes],
    std::size_t flat_index) {
  std::array<unsigned char, BlockBytes> bytes{};
  if (pMaskStack == nullptr) {
    return bytes;
  }
  flat_index %= kMaskStackTotalBytes;
  for (std::size_t lane = 0; lane < BlockBytes; ++lane) {
    const std::size_t cursor = (flat_index + lane) % kMaskStackTotalBytes;
    bytes[lane] = pMaskStack[(cursor / kMaskBytes) % kMaskStackDepth][cursor % kMaskBytes];
  }
  return bytes;
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

inline void RotateMaskStack(
    unsigned char pMaskStack[kMaskStackDepth][kMaskBytes],
    const unsigned char next_round_mask[kMaskBytes]) {
  if (pMaskStack == nullptr || next_round_mask == nullptr) {
    return;
  }
  for (std::size_t row = 0; row + 1U < kMaskStackDepth; ++row) {
    std::memcpy(pMaskStack[row], pMaskStack[row + 1U], kMaskBytes);
  }
  std::memcpy(pMaskStack[kMaskStackDepth - 1U], next_round_mask, kMaskBytes);
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

template <std::size_t BlockBytes>
inline std::array<unsigned char, BlockBytes> LoadBlockWrapped(
    const unsigned char* data,
    std::size_t total_bytes,
    std::size_t index) {
  std::array<unsigned char, BlockBytes> bytes{};
  if (data == nullptr || total_bytes == 0U) {
    return bytes;
  }
  index %= total_bytes;
  for (std::size_t lane = 0; lane < bytes.size(); ++lane) {
    bytes[lane] = data[(index + lane) % total_bytes];
  }
  return bytes;
}

inline std::array<unsigned char, kMatrixBlockBytes> LoadBlock16Wrapped(
    const unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index) {
  return LoadBlockWrapped<kMatrixBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index);
}

inline std::array<unsigned char, kHurricaneBlockBytes> LoadBlock256Wrapped(
    const unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index) {
  return LoadBlockWrapped<kHurricaneBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index);
}

inline std::array<unsigned char, kTyphoonBlockBytes> LoadBlock128Wrapped(
    const unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index) {
  return LoadBlockWrapped<kTyphoonBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index);
}

template <std::size_t BlockBytes>
inline void StoreBlockContiguous(
    unsigned char* data,
    std::size_t total_bytes,
    std::size_t index,
    const std::array<unsigned char, BlockBytes>& bytes) {
  if (data == nullptr || total_bytes == 0U || index >= total_bytes) {
    return;
  }
  const std::size_t copy_count = (index + BlockBytes <= total_bytes) ? BlockBytes : (total_bytes - index);
  std::memcpy(data + index, bytes.data(), copy_count);
}

inline void StoreBlock16Contiguous(
    unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    const std::array<unsigned char, kMatrixBlockBytes>& bytes) {
  StoreBlockContiguous<kMatrixBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index, bytes);
}

inline void StoreBlock256Contiguous(
    unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    const std::array<unsigned char, kHurricaneBlockBytes>& bytes) {
  StoreBlockContiguous<kHurricaneBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index, bytes);
}

inline void StoreBlock128Contiguous(
    unsigned char data[PASSWORD_EXPANDED_SIZE],
    std::size_t index,
    const std::array<unsigned char, kTyphoonBlockBytes>& bytes) {
  StoreBlockContiguous<kTyphoonBlockBytes>(data, PASSWORD_EXPANDED_SIZE, index, bytes);
}

inline std::uint32_t FoldBytesXor(const unsigned char* data, std::size_t size) {
  std::uint32_t value = 0U;
  if (data == nullptr) {
    return value;
  }
  for (std::size_t i = 0; i < size; ++i) {
    value ^= static_cast<std::uint32_t>(data[i]) << ((i & 3U) * 8U);
  }
  return value;
}

inline std::uint32_t FoldBytesAdd(const unsigned char* data, std::size_t size) {
  std::uint32_t value = 0U;
  if (data == nullptr) {
    return value;
  }
  for (std::size_t i = 0; i < size; ++i) {
    value = static_cast<std::uint32_t>(value + data[i] + static_cast<unsigned int>(i * 17U));
  }
  return value;
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
  KeySeedFunction key_seed;
  SaltSeedFunction salt_seed;
  MaskSeedFunction mask_seed_a;
  MaskSeedFunction mask_seed_b;
  TwistBlockFunction twist_block;
  PushKeyRoundFunction push_key_round;
  PushMaskRoundFunction push_mask_round_a;
  PushMaskRoundFunction push_mask_round_b;
};

extern const RegisteredCandidate kRegisteredCandidates[];
extern const std::size_t kRegisteredCandidateCount;
extern const RegisteredCandidate kBaselineCandidates[];
extern const std::size_t kBaselineCandidateCount;

}  // namespace twist
