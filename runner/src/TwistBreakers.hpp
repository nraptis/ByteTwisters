#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistTypes.hpp"
#include "TyphoonMatrix.hpp"

namespace twist {

template <std::size_t SourceCount>
struct DualWorkerBreakerRecipeBase {
  std::array<int, SourceCount> source_offsets;
  int control_offset;
  int partner_offset;
  std::size_t mask_flat_offset;
  std::uint8_t key_row;
  std::uint8_t key_offset;
  std::uint8_t salt_offset;
  std::uint8_t mixlane_a;
  std::uint8_t mixlane_b;
  std::uint8_t fast_rule;
  std::uint8_t slow_rule;
  std::uint8_t index_mode;
  std::uint8_t emit_mode;
};

using DualWorkerBreakerRecipe2 = DualWorkerBreakerRecipeBase<2>;
using DualWorkerBreakerRecipe3 = DualWorkerBreakerRecipeBase<3>;
using DualWorkerBreakerRecipe4 = DualWorkerBreakerRecipeBase<4>;

inline std::uint8_t FoldWordToByte(std::uint32_t value) {
  return static_cast<std::uint8_t>((value >> 24U) ^ (value >> 16U) ^ (value >> 8U) ^ value);
}

inline std::uint8_t GfDouble(std::uint8_t value) {
  const std::uint8_t carry = static_cast<std::uint8_t>(value & 0x80U);
  std::uint8_t doubled = static_cast<std::uint8_t>(value << 1U);
  if (carry != 0U) {
    doubled ^= 0x1BU;
  }
  return doubled;
}

inline std::uint8_t GfTriple(std::uint8_t value) {
  return static_cast<std::uint8_t>(GfDouble(value) ^ value);
}

inline void MixLightningColumnBytes(
    std::uint8_t& value0,
    std::uint8_t& value1,
    std::uint8_t& value2,
    std::uint8_t& value3) {
  const std::uint8_t mixed0 = static_cast<std::uint8_t>(GfDouble(value0) ^ GfTriple(value1) ^ value2 ^ value3);
  const std::uint8_t mixed1 = static_cast<std::uint8_t>(value0 ^ GfDouble(value1) ^ GfTriple(value2) ^ value3);
  const std::uint8_t mixed2 = static_cast<std::uint8_t>(value0 ^ value1 ^ GfDouble(value2) ^ GfTriple(value3));
  const std::uint8_t mixed3 = static_cast<std::uint8_t>(GfTriple(value0) ^ value1 ^ value2 ^ GfDouble(value3));
  value0 = mixed0;
  value1 = mixed1;
  value2 = mixed2;
  value3 = mixed3;
}

inline void ApplyLightningMixColumns(
    std::array<unsigned char, kMatrixBlockBytes>& bytes,
    const unsigned char (&salt)[kSaltBytes],
    std::uint8_t salt_offset,
    std::uint8_t rule) {
  for (std::size_t column = 0; column < 4U; ++column) {
    std::uint8_t value0 = FixedSBoxByte(static_cast<unsigned char>(
        bytes[column] ^
        salt[(salt_offset + column) & 31U] ^
        static_cast<unsigned char>(rule + column)));
    std::uint8_t value1 = FixedSBoxByte(static_cast<unsigned char>(
        bytes[4U + column] ^
        salt[(salt_offset + column + 5U) & 31U] ^
        static_cast<unsigned char>(rule + 17U)));
    std::uint8_t value2 = FixedSBoxByte(static_cast<unsigned char>(
        bytes[8U + column] ^
        salt[(salt_offset + column + 11U) & 31U] ^
        static_cast<unsigned char>(rule + 29U)));
    std::uint8_t value3 = FixedSBoxByte(static_cast<unsigned char>(
        bytes[12U + column] ^
        salt[(salt_offset + column + 19U) & 31U] ^
        static_cast<unsigned char>(rule + 43U)));
    MixLightningColumnBytes(value0, value1, value2, value3);
    bytes[column] = static_cast<unsigned char>(value0 ^ salt[(salt_offset + column + rule) & 31U]);
    bytes[4U + column] = static_cast<unsigned char>(RotateLeft8(value1, 1U) ^ salt[(salt_offset + column + 7U) & 31U]);
    bytes[8U + column] = static_cast<unsigned char>(RotateLeft8(value2, 3U) ^ salt[(salt_offset + column + 13U) & 31U]);
    bytes[12U + column] = static_cast<unsigned char>(RotateLeft8(value3, 5U) ^ salt[(salt_offset + column + 23U) & 31U]);
  }
}

inline void ApplyLightningMixColumns(
    LightningMatrix& matrix,
    const unsigned char (&salt)[kSaltBytes],
    std::uint8_t salt_offset,
    std::uint8_t rule) {
  std::array<unsigned char, kMatrixBlockBytes> bytes{};
  matrix.Store(bytes.data());
  ApplyLightningMixColumns(bytes, salt, salt_offset, rule);
  matrix.Load(bytes.data());
}

inline std::uint32_t AdvanceTwiddle32(
    std::uint32_t state,
    std::uint32_t value,
    std::uint32_t extra,
    std::uint32_t constant,
    unsigned rotate) {
  std::uint32_t mixed = state ^ (value + 0x9E3779B9u) ^ RotateLeft32(extra + constant, (rotate + 7U) & 31U);
  mixed *= 0x85EBCA6Bu;
  mixed ^= mixed >> 13U;
  mixed = RotateLeft32(mixed, rotate & 31U);
  mixed *= 0xC2B2AE35u;
  mixed ^= mixed >> 16U;
  return mixed;
}

inline void ApplySaltSBoxLayer(
    unsigned char (&salt)[kSaltBytes],
    std::uint32_t state,
    std::uint32_t bias,
    unsigned rotate) {
  for (std::size_t index = 0; index < kSaltBytes; ++index) {
    state = AdvanceTwiddle32(
        state ^ static_cast<std::uint32_t>(salt[(index + 11U) & 31U]),
        static_cast<std::uint32_t>(salt[index]) ^ bias,
        static_cast<std::uint32_t>(salt[(index + 7U) & 31U]) ^ static_cast<std::uint32_t>(index * 17U),
        0x9E3779B9u + static_cast<std::uint32_t>(index * 0x45D9F3Bu),
        5U + ((rotate + static_cast<unsigned>(index)) & 7U));
    const unsigned char wave = FixedSBoxByte(static_cast<unsigned char>(
        FoldWordToByte(state) ^
        salt[index] ^
        salt[(index + 13U) & 31U] ^
        static_cast<unsigned char>(bias + static_cast<std::uint32_t>(index * 29U))));
    salt[index] = static_cast<unsigned char>(
        RotateLeft8(static_cast<std::uint8_t>(salt[index] + wave), (rotate + static_cast<unsigned>(index)) & 7U) ^
        FixedSBoxByte(static_cast<unsigned char>(wave + salt[(index + 3U) & 31U])));
  }
}

inline std::uint32_t AdvanceSaltSeedAccumulator(
    unsigned char (&salt)[kSaltBytes],
    std::uint32_t accumulator,
    unsigned int salt_index,
    std::uint32_t value_a,
    std::uint32_t value_b,
    std::uint32_t lane,
    std::uint32_t bias,
    unsigned rotate,
    bool xor_mode) {
  const unsigned int lane_index = salt_index & 31U;
  const std::uint32_t rotated_b =
      static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(value_b & 0xFFU), rotate));
  const std::uint32_t nonlinear =
      static_cast<std::uint32_t>(FixedSBoxByte(static_cast<unsigned char>((value_a + bias + lane) ^ rotated_b)));
  accumulator = AdvanceTwiddle32(
      accumulator ^ static_cast<std::uint32_t>(salt[(lane_index + 11U) & 31U]),
      nonlinear ^ value_a,
      value_b ^ lane,
      bias * 0x45D9F3Bu + 0x27D4EB2Du,
      11U + (rotate & 7U));
  const unsigned char folded = FoldWordToByte(accumulator);
  const unsigned char mix = FixedSBoxByte(static_cast<unsigned char>(
      folded ^ static_cast<unsigned char>(nonlinear) ^ salt[lane_index] ^ salt[(lane_index + 7U) & 31U]));
  if (xor_mode) {
    salt[lane_index] ^= mix;
  } else {
    salt[lane_index] = static_cast<unsigned char>(salt[lane_index] + mix);
  }
  salt[(lane_index + 11U) & 31U] ^= FixedSBoxByte(static_cast<unsigned char>(mix + folded + (lane & 0xFFU)));
  return accumulator;
}

inline void UpdateMaskSeedBit(
    unsigned char (&mask_stack)[kMaskStackDepth][kMaskBytes],
    std::size_t flat_bit_index,
    std::uint32_t state,
    unsigned rotate,
    bool xor_mode) {
  const std::size_t total_bits = kMaskStackTotalBytes * 8U;
  const std::size_t bit_index = flat_bit_index % total_bits;
  const std::size_t byte_index = (bit_index >> 3U) % kMaskStackTotalBytes;
  const std::size_t row = (byte_index / kMaskBytes) % kMaskStackDepth;
  const std::size_t column = byte_index % kMaskBytes;
  const unsigned char bit_mask = static_cast<unsigned char>(1U << (bit_index & 7U));
  const unsigned char fold = MaskSeedMixBoxByte(static_cast<unsigned char>(
      FoldWordToByte(state) ^
      mask_stack[row][column] ^
      static_cast<unsigned char>(byte_index + (bit_index & 7U))));
  if (xor_mode) {
    mask_stack[row][column] ^= static_cast<unsigned char>(bit_mask ^ (fold & RotateLeft8(0xA5U, rotate)));
  } else {
    mask_stack[row][column] = static_cast<unsigned char>(
        mask_stack[row][column] +
        static_cast<unsigned char>((fold & bit_mask) ^ RotateLeft8(bit_mask, rotate)));
  }
}

inline std::uint32_t AdvanceMaskSeedBitstream(
    unsigned char (&mask_stack)[kMaskStackDepth][kMaskBytes],
    std::uint32_t state,
    std::size_t flat_bit_offset,
    std::uint32_t value_a,
    std::uint32_t value_b,
    std::uint32_t value_c,
    std::uint32_t lane,
    std::uint32_t bias,
    unsigned rotate,
    bool xor_mode) {
  const std::uint32_t rotated_b =
      static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(value_b & 0xFFU), rotate));
  const std::uint32_t nonlinear = static_cast<std::uint32_t>(MaskSeedMixBoxByte(static_cast<unsigned char>(
      value_a ^ rotated_b ^ value_c ^ bias ^ lane)));
  state = AdvanceTwiddle32(
      state ^ value_c,
      nonlinear ^ value_a,
      value_b ^ lane,
      0xA24BAEDCu + bias * 0x45D9F3Bu,
      5U + (rotate & 7U));
  const std::size_t total_bits = kMaskStackTotalBytes * 8U;
  const std::size_t primary_bit = (
      flat_bit_offset +
      static_cast<std::size_t>((state ^ (state >> 11U)) & 0x3FFFU) +
      static_cast<std::size_t>(lane * ((bias & 7U) + 1U))) % total_bits;
  UpdateMaskSeedBit(mask_stack, primary_bit, state ^ nonlinear, rotate, xor_mode);
  const std::size_t secondary_bit = (
      primary_bit +
      1U +
      static_cast<std::size_t>((state >> 17U) & 0x1FFU) +
      static_cast<std::size_t>(value_c & 7U)) % total_bits;
  UpdateMaskSeedBit(mask_stack, secondary_bit, RotateLeft32(state ^ nonlinear, 7U), rotate + 3U, true);
  return state;
}

inline void ApplyTsunamiBreaker(
    unsigned char* worker_a,
    unsigned char* worker_b,
    const unsigned char (&salt)[kSaltBytes],
    unsigned int round,
    std::uint32_t seed_a,
    std::uint32_t seed_b) {
  if (worker_a == nullptr || worker_b == nullptr) {
    return;
  }

  std::uint32_t acc_a = AdvanceTwiddle32(0x85EBCA6Bu ^ static_cast<std::uint32_t>(round), seed_a, seed_b, 0xC2B2AE35u, 7U);
  std::uint32_t acc_b = AdvanceTwiddle32(0xC2B2AE35u ^ static_cast<std::uint32_t>(round), seed_b, seed_a, 0x165667B1u, 11U);

  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    const std::uint32_t cross_a = static_cast<std::uint32_t>(worker_b[(i + 13U) % PASSWORD_EXPANDED_SIZE]) ^
                                  static_cast<std::uint32_t>(salt[i & 31U]);
    const std::uint32_t cross_b = static_cast<std::uint32_t>(worker_a[(i + 7U) % PASSWORD_EXPANDED_SIZE]) ^
                                  static_cast<std::uint32_t>(salt[(i + 13U) & 31U]);
    acc_a = AdvanceTwiddle32(acc_a, static_cast<std::uint32_t>(worker_a[i]) ^ cross_a, seed_a ^ static_cast<std::uint32_t>(i * 17U), 0x85EBCA6Bu, 7U);
    acc_b = AdvanceTwiddle32(acc_b, static_cast<std::uint32_t>(worker_b[i]) ^ cross_b, seed_b ^ static_cast<std::uint32_t>(i * 29U), 0xC2B2AE35u, 5U);
    const unsigned char wave_a = static_cast<unsigned char>(FoldWordToByte(acc_a) ^ salt[(i + 5U) & 31U]);
    const unsigned char wave_b = static_cast<unsigned char>(FoldWordToByte(acc_b) ^ salt[(i + 17U) & 31U]);
    worker_a[i] ^= wave_b;
    worker_b[i] = static_cast<unsigned char>(worker_b[i] + wave_a);
    if ((i & 0x0FU) == 0U) {
      const std::uint32_t temp = acc_a;
      acc_a = acc_b ^ (temp >> 5U);
      acc_b = temp + RotateLeft32(acc_b, 3U);
    }
  }
}

template <std::size_t SourceCount>
inline unsigned char FoldRecipeSourceLane(
    const std::array<std::array<unsigned char, kMatrixBlockBytes>, SourceCount>& source_blocks,
    std::size_t lane,
    const DualWorkerBreakerRecipeBase<SourceCount>& recipe) {
  unsigned char mixed = source_blocks[0][lane & 15U];
  if constexpr (SourceCount > 1U) {
    mixed ^= RotateLeft8(source_blocks[1][(lane + recipe.mixlane_a) & 15U], 1U + (recipe.index_mode & 1U));
  }
  if constexpr (SourceCount > 2U) {
    mixed = FixedSBoxByte(static_cast<unsigned char>(
        mixed +
        source_blocks[2][(lane + recipe.mixlane_b) & 15U] +
        recipe.fast_rule));
  }
  if constexpr (SourceCount > 3U) {
    mixed ^= FixedSBoxByte(static_cast<unsigned char>(
        source_blocks[3][(lane + recipe.mixlane_a + recipe.mixlane_b) & 15U] ^
        recipe.slow_rule));
  }
  return mixed;
}

template <std::size_t SourceCount>
inline void InjectRecipeSourceMatrices(
    const DualWorkerBreakerRecipeBase<SourceCount>& recipe,
    const std::array<std::array<unsigned char, kMatrixBlockBytes>, SourceCount>& source_blocks,
    LightningMatrix& lane_a,
    LightningMatrix& lane_b,
    const unsigned char (&salt)[kSaltBytes]) {
  LightningMatrix source0(source_blocks[0].data());
  ApplyLightningMixColumns(source0, salt, recipe.salt_offset, recipe.fast_rule);
  lane_a.XorWith(source0);

  if constexpr (SourceCount > 1U) {
    LightningMatrix source1(source_blocks[1].data());
    ApplyLightningMixColumns(source1, salt, static_cast<std::uint8_t>(recipe.salt_offset + 3U), recipe.slow_rule);
    lane_b.AddWith(source1);
  }
  if constexpr (SourceCount > 2U) {
    LightningMatrix source2(source_blocks[2].data());
    ApplyLightningMixColumns(
        source2,
        salt,
        static_cast<std::uint8_t>(recipe.salt_offset + 7U),
        static_cast<std::uint8_t>(recipe.fast_rule ^ recipe.slow_rule));
    lane_a.AddWith(source2);
    lane_b.XorWith(source2);
  }
  if constexpr (SourceCount > 3U) {
    LightningMatrix source3(source_blocks[3].data());
    ApplyLightningMixColumns(
        source3,
        salt,
        static_cast<std::uint8_t>(recipe.salt_offset + 13U),
        static_cast<std::uint8_t>(recipe.fast_rule + recipe.slow_rule + recipe.index_mode));
    lane_a.XorWith(source3);
    lane_b.AddWith(source3);
  }
}

template <std::size_t SourceCount>
inline void ApplyDualWorkerMatrixBreakerImpl(
    const DualWorkerBreakerRecipeBase<SourceCount>& recipe,
    const unsigned char* source,
    unsigned char* worker_a,
    unsigned char* worker_b,
    const unsigned char (&salt)[kSaltBytes],
    unsigned char (&key_stack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&mask_stack_a)[kMaskStackDepth][kMaskBytes],
    unsigned char (&mask_stack_b)[kMaskStackDepth][kMaskBytes],
    unsigned int round) {
  if (source == nullptr || worker_a == nullptr || worker_b == nullptr) {
    return;
  }

  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMatrixBlockBytes) {
    std::array<std::array<unsigned char, kMatrixBlockBytes>, SourceCount> source_blocks{};
    for (std::size_t source_index_id = 0; source_index_id < SourceCount; ++source_index_id) {
      const int source_offset = recipe.source_offsets[source_index_id];
      const std::size_t source_index = static_cast<std::size_t>(WrapRange(
          static_cast<int>(chunk) +
              source_offset +
              static_cast<int>(source_index_id * static_cast<std::size_t>((recipe.index_mode & 3U) * 17U)),
          0,
          static_cast<int>(PASSWORD_EXPANDED_SIZE)));
      source_blocks[source_index_id] =
          LoadBlockWrapped<kMatrixBlockBytes>(source, PASSWORD_EXPANDED_SIZE, source_index);
    }
    const std::size_t control_index = static_cast<std::size_t>(
        WrapRange(static_cast<int>(chunk) + recipe.control_offset, 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));
    const std::size_t partner_index = static_cast<std::size_t>(
        WrapRange(static_cast<int>(chunk) + recipe.partner_offset, 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));

    const auto control = LoadBlockWrapped<kMatrixBlockBytes>(source, PASSWORD_EXPANDED_SIZE, control_index);
    const auto partner = LoadBlockWrapped<kMatrixBlockBytes>(worker_b, PASSWORD_EXPANDED_SIZE, partner_index);
    const auto block_a = LoadBlockWrapped<kMatrixBlockBytes>(worker_a, PASSWORD_EXPANDED_SIZE, chunk);
    const auto block_b = LoadBlockWrapped<kMatrixBlockBytes>(worker_b, PASSWORD_EXPANDED_SIZE, chunk);
    const auto mask_a = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(
        mask_stack_a,
        chunk + recipe.mask_flat_offset + static_cast<std::size_t>((recipe.index_mode & 3U) * 11U));
    const auto mask_b = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(
        mask_stack_b,
        chunk + recipe.mask_flat_offset + static_cast<std::size_t>((recipe.index_mode & 3U) * 11U));
    const auto key = LoadKeyStackBlock16Wrapped(
        key_stack,
        static_cast<std::size_t>((round + recipe.key_row) & 15U),
        chunk + recipe.key_offset);

    LightningMatrix lane_a(block_a.data());
    LightningMatrix lane_b(block_b.data());
    LightningMatrix partner_matrix(partner.data());
    LightningMatrix key_matrix(key.data());

    InjectRecipeSourceMatrices(recipe, source_blocks, lane_a, lane_b, salt);
    lane_a.AddWith(key_matrix);
    lane_b.XorWith(partner_matrix);
    lane_a.InjectXor(mask_a.data(), mask_a.size(), control[recipe.mixlane_a & 15U] & 15U);
    lane_a.InjectAdd(mask_b.data(), mask_b.size(), control[(recipe.mixlane_b + 5U) & 15U] & 15U);
    lane_b.InjectAdd(mask_b.data(), mask_b.size(), control[recipe.mixlane_b & 15U] & 15U);
    lane_b.InjectXor(mask_a.data(), mask_a.size(), control[(recipe.mixlane_a + 7U) & 15U] & 15U);
    lane_a.InjectAdd(salt, kSaltBytes, recipe.salt_offset);
    lane_b.InjectXor(salt, kSaltBytes, static_cast<std::size_t>(recipe.salt_offset + 7U));

    std::uint32_t source_fold = FoldBytesXor(source_blocks[0].data(), source_blocks[0].size());
    if constexpr (SourceCount > 1U) {
      source_fold ^= FoldBytesAdd(source_blocks[1].data(), source_blocks[1].size());
    }
    if constexpr (SourceCount > 2U) {
      source_fold ^= FoldBytesXor(source_blocks[2].data(), source_blocks[2].size()) << 1U;
    }
    if constexpr (SourceCount > 3U) {
      source_fold ^= FoldBytesAdd(source_blocks[3].data(), source_blocks[3].size()) << 2U;
    }
    const std::uint32_t selector =
        FoldBytesXor(control.data(), control.size()) ^
        FoldBytesAdd(mask_a.data(), mask_a.size()) ^
        FoldBytesXor(mask_b.data(), mask_b.size()) ^
        FoldBytesXor(key.data(), key.size()) ^
        FoldBytesAdd(partner.data(), partner.size()) ^
        source_fold ^
        static_cast<std::uint32_t>(recipe.fast_rule) ^
        static_cast<std::uint32_t>(round * 17U);
    lane_a.ApplyFastOp(
        static_cast<LightningFastOp>(selector % 12U),
        static_cast<std::uint8_t>(control[0] ^ key[recipe.mixlane_a & 15U] ^ FoldRecipeSourceLane(source_blocks, 0U, recipe)),
        static_cast<std::uint8_t>(mask_a[1] + mask_b[2] + recipe.mixlane_b + FoldRecipeSourceLane(source_blocks, 1U, recipe)));
    lane_b.ApplyFastOp(
        static_cast<LightningFastOp>((selector >> 3U) % 12U),
        static_cast<std::uint8_t>(control[2] + key[(recipe.mixlane_b + 3U) & 15U] + FoldRecipeSourceLane(source_blocks, 2U, recipe)),
        static_cast<std::uint8_t>(mask_b[3] ^ mask_a[4] ^ recipe.mixlane_a ^ FoldRecipeSourceLane(source_blocks, 3U, recipe)));

    const std::uint32_t slow_selector =
        selector ^
        FoldBytesAdd(partner.data(), partner.size()) ^
        (source_fold >> 7U) ^
        static_cast<std::uint32_t>(recipe.slow_rule);
    lane_a.ApplySlowOp(
        static_cast<LightningSlowOp>(slow_selector % 8U),
        static_cast<std::uint8_t>(control[4] + mask_a[5] + mask_b[6] + FoldRecipeSourceLane(source_blocks, 4U, recipe)),
        static_cast<std::uint8_t>(key[6] ^ salt[(recipe.salt_offset + 5U) & 31U] ^ FoldRecipeSourceLane(source_blocks, 5U, recipe)));
    lane_b.ApplySlowOp(
        static_cast<LightningSlowOp>((slow_selector >> 5U) % 8U),
        static_cast<std::uint8_t>(control[7] ^ mask_b[8] ^ mask_a[9] ^ FoldRecipeSourceLane(source_blocks, 6U, recipe)),
        static_cast<std::uint8_t>(key[9] + salt[(recipe.salt_offset + 11U) & 31U] + FoldRecipeSourceLane(source_blocks, 7U, recipe)));

    ApplyLightningMixColumns(
        lane_a,
        salt,
        static_cast<std::uint8_t>(recipe.salt_offset + FoldRecipeSourceLane(source_blocks, 8U, recipe)),
        static_cast<std::uint8_t>(recipe.fast_rule ^ FoldWordToByte(selector)));
    ApplyLightningMixColumns(
        lane_b,
        salt,
        static_cast<std::uint8_t>(recipe.salt_offset + 11U + FoldRecipeSourceLane(source_blocks, 9U, recipe)),
        static_cast<std::uint8_t>(recipe.slow_rule ^ FoldWordToByte(slow_selector)));

    std::array<unsigned char, kMatrixBlockBytes> store_a{};
    std::array<unsigned char, kMatrixBlockBytes> store_b{};
    std::array<unsigned char, kMatrixBlockBytes> emit_a{};
    std::array<unsigned char, kMatrixBlockBytes> emit_b{};
    lane_a.Store(store_a.data());
    lane_b.Store(store_b.data());

    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {
      const unsigned char source_feedback = FoldRecipeSourceLane(source_blocks, lane, recipe);
      const unsigned char feedback = FixedSBoxByte(static_cast<unsigned char>(
          source_feedback ^
          control[(lane + recipe.mixlane_b) & 15U] ^
          key[(lane + 3U) & 15U] ^
          mask_a[(lane + 5U) & 15U] ^
          mask_b[(lane + 9U) & 15U] ^
          partner[(lane + recipe.mixlane_a + recipe.index_mode) & 15U] ^
          salt[(lane + recipe.salt_offset) & 31U]));
      if (recipe.emit_mode == 0U) {
        emit_a[lane] = static_cast<unsigned char>(
            store_a[lane] ^
            RotateLeft8(store_b[(lane + recipe.mixlane_a + recipe.index_mode) & 15U], 1U) ^
            feedback);
        emit_b[lane] = static_cast<unsigned char>(
            store_b[lane] +
            RotateLeft8(store_a[(lane + recipe.mixlane_b + (recipe.index_mode & 3U)) & 15U], 3U) +
            feedback);
      } else {
        emit_a[lane] = static_cast<unsigned char>(
            store_a[lane] +
            RotateLeft8(store_b[(lane + recipe.mixlane_a + recipe.index_mode) & 15U], 1U) +
            feedback);
        emit_b[lane] = static_cast<unsigned char>(
            store_b[lane] ^
            RotateLeft8(store_a[(lane + recipe.mixlane_b + (recipe.index_mode & 3U)) & 15U], 3U) ^
            feedback);
      }
    }

    StoreBlockContiguous<kMatrixBlockBytes>(worker_a, PASSWORD_EXPANDED_SIZE, chunk, emit_a);
    StoreBlockContiguous<kMatrixBlockBytes>(worker_b, PASSWORD_EXPANDED_SIZE, chunk, emit_b);
  }
}

inline void ApplyDualWorkerMatrixBreaker(
    const DualWorkerBreakerRecipe2& recipe,
    const unsigned char* source,
    unsigned char* worker_a,
    unsigned char* worker_b,
    const unsigned char (&salt)[kSaltBytes],
    unsigned char (&key_stack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&mask_stack_a)[kMaskStackDepth][kMaskBytes],
    unsigned char (&mask_stack_b)[kMaskStackDepth][kMaskBytes],
    unsigned int round) {
  ApplyDualWorkerMatrixBreakerImpl(recipe, source, worker_a, worker_b, salt, key_stack, mask_stack_a, mask_stack_b, round);
}

inline void ApplyDualWorkerMatrixBreaker(
    const DualWorkerBreakerRecipe3& recipe,
    const unsigned char* source,
    unsigned char* worker_a,
    unsigned char* worker_b,
    const unsigned char (&salt)[kSaltBytes],
    unsigned char (&key_stack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&mask_stack_a)[kMaskStackDepth][kMaskBytes],
    unsigned char (&mask_stack_b)[kMaskStackDepth][kMaskBytes],
    unsigned int round) {
  ApplyDualWorkerMatrixBreakerImpl(recipe, source, worker_a, worker_b, salt, key_stack, mask_stack_a, mask_stack_b, round);
}

inline void ApplyDualWorkerMatrixBreaker(
    const DualWorkerBreakerRecipe4& recipe,
    const unsigned char* source,
    unsigned char* worker_a,
    unsigned char* worker_b,
    const unsigned char (&salt)[kSaltBytes],
    unsigned char (&key_stack)[kRoundKeyStackDepth][kRoundKeyBytes],
    unsigned char (&mask_stack_a)[kMaskStackDepth][kMaskBytes],
    unsigned char (&mask_stack_b)[kMaskStackDepth][kMaskBytes],
    unsigned int round) {
  ApplyDualWorkerMatrixBreakerImpl(recipe, source, worker_a, worker_b, salt, key_stack, mask_stack_a, mask_stack_b, round);
}

inline void ApplyFinalWhitening(
    unsigned char* dest,
    const unsigned char (&salt)[kSaltBytes],
    std::uint32_t twiddle_a,
    std::uint32_t twiddle_b,
    unsigned int round) {
  if (dest == nullptr) {
    return;
  }

  std::uint32_t acc = AdvanceTwiddle32(
      twiddle_a ^ static_cast<std::uint32_t>(round),
      twiddle_b,
      static_cast<std::uint32_t>(round * 31U),
      0x27D4EB2Du,
      11U);
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {
    acc = AdvanceTwiddle32(
        acc,
        static_cast<std::uint32_t>(dest[i]) ^ static_cast<std::uint32_t>(salt[i & 31U]),
        twiddle_b ^ static_cast<std::uint32_t>(i * 13U),
        0x165667B1u,
        9U);
    const unsigned char wave = FixedSBoxByte(static_cast<unsigned char>(
        FoldWordToByte(acc) ^ dest[i] ^ salt[(i + ((acc >> 27U) & 31U)) & 31U]));
    dest[i] ^= wave;
  }
}

}  // namespace twist
