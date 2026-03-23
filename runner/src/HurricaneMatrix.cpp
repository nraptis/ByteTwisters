#include "HurricaneMatrix.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace twist {

HurricaneMatrix::HurricaneMatrix() = default;

HurricaneMatrix::HurricaneMatrix(const std::uint8_t* bytes) {
  Load(bytes);
}

std::uint8_t* HurricaneMatrix::Data() {
  return data_.data();
}

const std::uint8_t* HurricaneMatrix::Data() const {
  return data_.data();
}

void HurricaneMatrix::Clear() {
  data_.fill(0U);
}

void HurricaneMatrix::Fill(std::uint8_t value) {
  data_.fill(value);
}

void HurricaneMatrix::Load(const std::uint8_t* bytes) {
  if (bytes == nullptr) {
    Clear();
    return;
  }
  std::memcpy(data_.data(), bytes, kSize);
}

void HurricaneMatrix::Store(std::uint8_t* bytes) const {
  if (bytes == nullptr) {
    return;
  }
  std::memcpy(bytes, data_.data(), kSize);
}

std::uint8_t& HurricaneMatrix::At(std::size_t row, std::size_t column) {
  return data_[Offset(row, column)];
}

std::uint8_t HurricaneMatrix::At(std::size_t row, std::size_t column) const {
  return data_[Offset(row, column)];
}

void HurricaneMatrix::RotateRowLeft(std::size_t row, unsigned amount) {
  row = NormalizeIndex(row);
  const unsigned shift = NormalizeAmount(amount);
  if (shift == 0U) {
    return;
  }
  std::array<std::uint8_t, kWidth> copy{};
  for (std::size_t col = 0; col < kWidth; ++col) {
    copy[col] = At(row, col);
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(row, col) = copy[(col + shift) % kWidth];
  }
}

void HurricaneMatrix::RotateRowRight(std::size_t row, unsigned amount) {
  RotateRowLeft(row, (kWidth - NormalizeAmount(amount)) % kWidth);
}

void HurricaneMatrix::RotateColumnUp(std::size_t column, unsigned amount) {
  column = NormalizeIndex(column);
  const unsigned shift = NormalizeAmount(amount);
  if (shift == 0U) {
    return;
  }
  std::array<std::uint8_t, kHeight> copy{};
  for (std::size_t row = 0; row < kHeight; ++row) {
    copy[row] = At(row, column);
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, column) = copy[(row + shift) % kHeight];
  }
}

void HurricaneMatrix::RotateColumnDown(std::size_t column, unsigned amount) {
  RotateColumnUp(column, (kHeight - NormalizeAmount(amount)) % kHeight);
}

void HurricaneMatrix::SwapRows(std::size_t row_a, std::size_t row_b) {
  row_a = NormalizeIndex(row_a);
  row_b = NormalizeIndex(row_b);
  if (row_a == row_b) {
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    std::swap(At(row_a, col), At(row_b, col));
  }
}

void HurricaneMatrix::SwapColumns(std::size_t column_a, std::size_t column_b) {
  column_a = NormalizeIndex(column_a);
  column_b = NormalizeIndex(column_b);
  if (column_a == column_b) {
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    std::swap(At(row, column_a), At(row, column_b));
  }
}

void HurricaneMatrix::XorRowIntoRow(std::size_t dst_row, std::size_t src_row) {
  dst_row = NormalizeIndex(dst_row);
  src_row = NormalizeIndex(src_row);
  if (dst_row == src_row) {
    RotateRowLeft(dst_row, 1U);
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(dst_row, col) = static_cast<std::uint8_t>(At(dst_row, col) ^ At(src_row, col));
  }
}

void HurricaneMatrix::AddRowIntoRow(std::size_t dst_row, std::size_t src_row) {
  dst_row = NormalizeIndex(dst_row);
  src_row = NormalizeIndex(src_row);
  if (dst_row == src_row) {
    RotateRowRight(dst_row, 1U);
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(dst_row, col) = static_cast<std::uint8_t>(At(dst_row, col) + At(src_row, col));
  }
}

void HurricaneMatrix::XorColumnIntoColumn(std::size_t dst_column, std::size_t src_column) {
  dst_column = NormalizeIndex(dst_column);
  src_column = NormalizeIndex(src_column);
  if (dst_column == src_column) {
    RotateColumnUp(dst_column, 1U);
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, dst_column) = static_cast<std::uint8_t>(At(row, dst_column) ^ At(row, src_column));
  }
}

void HurricaneMatrix::AddColumnIntoColumn(std::size_t dst_column, std::size_t src_column) {
  dst_column = NormalizeIndex(dst_column);
  src_column = NormalizeIndex(src_column);
  if (dst_column == src_column) {
    RotateColumnDown(dst_column, 1U);
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, dst_column) = static_cast<std::uint8_t>(At(row, dst_column) + At(row, src_column));
  }
}

void HurricaneMatrix::WeaveRows(std::size_t row_a, std::size_t row_b) {
  row_a = NormalizeIndex(row_a);
  row_b = NormalizeIndex(row_b);
  if (row_a == row_b) {
    return;
  }
  for (std::size_t col = 1; col < kWidth; col += 2U) {
    std::swap(At(row_a, col), At(row_b, col));
  }
}

void HurricaneMatrix::WeaveColumns(std::size_t column_a, std::size_t column_b) {
  column_a = NormalizeIndex(column_a);
  column_b = NormalizeIndex(column_b);
  if (column_a == column_b) {
    return;
  }
  for (std::size_t row = 1; row < kHeight; row += 2U) {
    std::swap(At(row, column_a), At(row, column_b));
  }
}

void HurricaneMatrix::RotateRight() {
  std::array<std::uint8_t, kSize> original = data_;
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = 0; col < kWidth; ++col) {
      At(row, col) = original[(kHeight - 1U - col) * kWidth + row];
    }
  }
}

void HurricaneMatrix::RotateLeft() {
  std::array<std::uint8_t, kSize> original = data_;
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = 0; col < kWidth; ++col) {
      At(row, col) = original[col * kWidth + (kWidth - 1U - row)];
    }
  }
}

void HurricaneMatrix::FlipHorizontal() {
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = 0; col < kWidth / 2U; ++col) {
      std::swap(At(row, col), At(row, kWidth - 1U - col));
    }
  }
}

void HurricaneMatrix::FlipVertical() {
  for (std::size_t row = 0; row < kHeight / 2U; ++row) {
    SwapRows(row, kHeight - 1U - row);
  }
}

void HurricaneMatrix::Transpose() {
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = row + 1U; col < kWidth; ++col) {
      std::swap(At(row, col), At(col, row));
    }
  }
}

void HurricaneMatrix::FlipDiagB() {
  std::array<std::uint8_t, kSize> original = data_;
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = 0; col < kWidth; ++col) {
      At(row, col) = original[(kHeight - 1U - col) * kWidth + (kWidth - 1U - row)];
    }
  }
}

void HurricaneMatrix::RotateRing(unsigned amount) {
  const unsigned shift = amount % (kWidth * 4U - 4U);
  if (shift == 0U) {
    return;
  }
  for (std::size_t layer = 0; layer < kWidth / 2U; ++layer) {
    const std::size_t min = layer;
    const std::size_t max = kWidth - 1U - layer;
    if (min >= max) {
      break;
    }
    std::vector<std::size_t> ring_indices;
    for (std::size_t col = min; col <= max; ++col) ring_indices.push_back(Offset(min, col));
    for (std::size_t row = min + 1U; row <= max; ++row) ring_indices.push_back(Offset(row, max));
    for (std::size_t col = max; col-- > min;) ring_indices.push_back(Offset(max, col));
    for (std::size_t row = max; row-- > min + 1U;) ring_indices.push_back(Offset(row, min));
    if (ring_indices.empty()) {
      continue;
    }
    std::vector<std::uint8_t> copy(ring_indices.size());
    for (std::size_t i = 0; i < ring_indices.size(); ++i) {
      copy[i] = data_[ring_indices[i]];
    }
    const unsigned layer_shift = amount % ring_indices.size();
    for (std::size_t i = 0; i < ring_indices.size(); ++i) {
      data_[ring_indices[(i + layer_shift) % ring_indices.size()]] = copy[i];
    }
  }
}

void HurricaneMatrix::TwistCross(unsigned amount) {
  const std::size_t center_a = 7U;
  const std::size_t center_b = 8U;
  const unsigned shift = NormalizeAmount(amount);
  for (std::size_t i = 0; i < kWidth; ++i) {
    At(center_a, i) = static_cast<std::uint8_t>(At(center_a, i) ^ At(i, (i + shift) % kWidth));
    At(center_b, i) = static_cast<std::uint8_t>(At(center_b, i) + At((i + shift) % kHeight, i));
    At(i, center_a) = static_cast<std::uint8_t>(At(i, center_a) ^ static_cast<std::uint8_t>(FoldAdd() + i));
    At(i, center_b) = static_cast<std::uint8_t>(At(i, center_b) + static_cast<std::uint8_t>(FoldXor() + i * 3U));
  }
}

void HurricaneMatrix::XorWith(const HurricaneMatrix& other) {
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] ^ other.data_[i]);
  }
}

void HurricaneMatrix::AddWith(const HurricaneMatrix& other) {
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] + other.data_[i]);
  }
}

void HurricaneMatrix::InjectXor(const std::uint8_t* bytes, std::size_t size, std::size_t start) {
  if (bytes == nullptr || size == 0U) {
    return;
  }
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] ^ bytes[(start + i) % size]);
  }
}

void HurricaneMatrix::InjectAdd(const std::uint8_t* bytes, std::size_t size, std::size_t start) {
  if (bytes == nullptr || size == 0U) {
    return;
  }
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] + bytes[(start + i) % size]);
  }
}

std::uint8_t HurricaneMatrix::FoldXor() const {
  std::uint8_t value = 0U;
  for (std::uint8_t byte : data_) {
    value = static_cast<std::uint8_t>(value ^ byte);
  }
  return value;
}

std::uint8_t HurricaneMatrix::FoldAdd() const {
  std::uint8_t value = 0U;
  for (std::uint8_t byte : data_) {
    value = static_cast<std::uint8_t>(value + byte);
  }
  return value;
}

void HurricaneMatrix::ApplyFastOp(HurricaneFastOp op, std::uint8_t arg0, std::uint8_t arg1) {
  switch (op) {
    case HurricaneFastOp::kRotateRowLeft:
      RotateRowLeft(arg0, arg1);
      break;
    case HurricaneFastOp::kRotateRowRight:
      RotateRowRight(arg0, arg1);
      break;
    case HurricaneFastOp::kRotateColumnUp:
      RotateColumnUp(arg0, arg1);
      break;
    case HurricaneFastOp::kRotateColumnDown:
      RotateColumnDown(arg0, arg1);
      break;
    case HurricaneFastOp::kSwapRows:
      SwapRows(arg0, arg1);
      break;
    case HurricaneFastOp::kSwapColumns:
      SwapColumns(arg0, arg1);
      break;
    case HurricaneFastOp::kXorRowIntoRow:
      XorRowIntoRow(arg0, arg1);
      break;
    case HurricaneFastOp::kAddRowIntoRow:
      AddRowIntoRow(arg0, arg1);
      break;
    case HurricaneFastOp::kXorColumnIntoColumn:
      XorColumnIntoColumn(arg0, arg1);
      break;
    case HurricaneFastOp::kAddColumnIntoColumn:
      AddColumnIntoColumn(arg0, arg1);
      break;
    case HurricaneFastOp::kWeaveRows:
      WeaveRows(arg0, arg1);
      break;
    case HurricaneFastOp::kWeaveColumns:
      WeaveColumns(arg0, arg1);
      break;
  }
}

void HurricaneMatrix::ApplySlowOp(HurricaneSlowOp op, std::uint8_t arg0, std::uint8_t arg1) {
  static_cast<void>(arg1);
  switch (op) {
    case HurricaneSlowOp::kRotateRight:
      RotateRight();
      break;
    case HurricaneSlowOp::kRotateLeft:
      RotateLeft();
      break;
    case HurricaneSlowOp::kFlipHorizontal:
      FlipHorizontal();
      break;
    case HurricaneSlowOp::kFlipVertical:
      FlipVertical();
      break;
    case HurricaneSlowOp::kTranspose:
      Transpose();
      break;
    case HurricaneSlowOp::kFlipDiagB:
      FlipDiagB();
      break;
    case HurricaneSlowOp::kRotateRing:
      RotateRing(arg0);
      break;
    case HurricaneSlowOp::kTwistCross:
      TwistCross(arg0);
      break;
  }
}

std::size_t HurricaneMatrix::NormalizeIndex(std::size_t value) {
  return value % kWidth;
}

unsigned HurricaneMatrix::NormalizeAmount(unsigned value) {
  return value % kWidth;
}

std::size_t HurricaneMatrix::Offset(std::size_t row, std::size_t column) const {
  return (NormalizeIndex(row) * kWidth) + NormalizeIndex(column);
}

}  // namespace twist
