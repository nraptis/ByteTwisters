#include "TyphoonMatrix.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace twist {

TyphoonMatrix::TyphoonMatrix() = default;

TyphoonMatrix::TyphoonMatrix(const std::uint8_t* bytes) {
  Load(bytes);
}

std::uint8_t* TyphoonMatrix::Data() {
  return data_.data();
}

const std::uint8_t* TyphoonMatrix::Data() const {
  return data_.data();
}

void TyphoonMatrix::Clear() {
  data_.fill(0U);
}

void TyphoonMatrix::Fill(std::uint8_t value) {
  data_.fill(value);
}

void TyphoonMatrix::Load(const std::uint8_t* bytes) {
  if (bytes == nullptr) {
    Clear();
    return;
  }
  std::memcpy(data_.data(), bytes, kSize);
}

void TyphoonMatrix::Store(std::uint8_t* bytes) const {
  if (bytes == nullptr) {
    return;
  }
  std::memcpy(bytes, data_.data(), kSize);
}

std::uint8_t& TyphoonMatrix::At(std::size_t row, std::size_t column) {
  return data_[Offset(row, column)];
}

std::uint8_t TyphoonMatrix::At(std::size_t row, std::size_t column) const {
  return data_[Offset(row, column)];
}

void TyphoonMatrix::RotateRowLeft(std::size_t row, unsigned amount) {
  row = NormalizeRow(row);
  const unsigned shift = NormalizeColumnAmount(amount);
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

void TyphoonMatrix::RotateRowRight(std::size_t row, unsigned amount) {
  RotateRowLeft(row, (kWidth - NormalizeColumnAmount(amount)) % kWidth);
}

void TyphoonMatrix::RotateColumnUp(std::size_t column, unsigned amount) {
  column = NormalizeColumn(column);
  const unsigned shift = NormalizeRowAmount(amount);
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

void TyphoonMatrix::RotateColumnDown(std::size_t column, unsigned amount) {
  RotateColumnUp(column, (kHeight - NormalizeRowAmount(amount)) % kHeight);
}

void TyphoonMatrix::SwapRows(std::size_t row_a, std::size_t row_b) {
  row_a = NormalizeRow(row_a);
  row_b = NormalizeRow(row_b);
  if (row_a == row_b) {
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    std::swap(At(row_a, col), At(row_b, col));
  }
}

void TyphoonMatrix::SwapColumns(std::size_t column_a, std::size_t column_b) {
  column_a = NormalizeColumn(column_a);
  column_b = NormalizeColumn(column_b);
  if (column_a == column_b) {
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    std::swap(At(row, column_a), At(row, column_b));
  }
}

void TyphoonMatrix::XorRowIntoRow(std::size_t dst_row, std::size_t src_row) {
  dst_row = NormalizeRow(dst_row);
  src_row = NormalizeRow(src_row);
  if (dst_row == src_row) {
    RotateRowLeft(dst_row, 1U);
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(dst_row, col) = static_cast<std::uint8_t>(At(dst_row, col) ^ At(src_row, col));
  }
}

void TyphoonMatrix::AddRowIntoRow(std::size_t dst_row, std::size_t src_row) {
  dst_row = NormalizeRow(dst_row);
  src_row = NormalizeRow(src_row);
  if (dst_row == src_row) {
    RotateRowRight(dst_row, 1U);
    return;
  }
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(dst_row, col) = static_cast<std::uint8_t>(At(dst_row, col) + At(src_row, col));
  }
}

void TyphoonMatrix::XorColumnIntoColumn(std::size_t dst_column, std::size_t src_column) {
  dst_column = NormalizeColumn(dst_column);
  src_column = NormalizeColumn(src_column);
  if (dst_column == src_column) {
    RotateColumnUp(dst_column, 1U);
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, dst_column) = static_cast<std::uint8_t>(At(row, dst_column) ^ At(row, src_column));
  }
}

void TyphoonMatrix::AddColumnIntoColumn(std::size_t dst_column, std::size_t src_column) {
  dst_column = NormalizeColumn(dst_column);
  src_column = NormalizeColumn(src_column);
  if (dst_column == src_column) {
    RotateColumnDown(dst_column, 1U);
    return;
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, dst_column) = static_cast<std::uint8_t>(At(row, dst_column) + At(row, src_column));
  }
}

void TyphoonMatrix::WeaveRows(std::size_t row_a, std::size_t row_b) {
  row_a = NormalizeRow(row_a);
  row_b = NormalizeRow(row_b);
  if (row_a == row_b) {
    return;
  }
  for (std::size_t col = 1; col < kWidth; col += 2U) {
    std::swap(At(row_a, col), At(row_b, col));
  }
}

void TyphoonMatrix::WeaveColumns(std::size_t column_a, std::size_t column_b) {
  column_a = NormalizeColumn(column_a);
  column_b = NormalizeColumn(column_b);
  if (column_a == column_b) {
    return;
  }
  for (std::size_t row = 1; row < kHeight; row += 2U) {
    std::swap(At(row, column_a), At(row, column_b));
  }
}

void TyphoonMatrix::FlipHorizontal() {
  for (std::size_t row = 0; row < kHeight; ++row) {
    for (std::size_t col = 0; col < kWidth / 2U; ++col) {
      std::swap(At(row, col), At(row, kWidth - 1U - col));
    }
  }
}

void TyphoonMatrix::FlipVertical() {
  for (std::size_t row = 0; row < kHeight / 2U; ++row) {
    SwapRows(row, kHeight - 1U - row);
  }
}

void TyphoonMatrix::RotateRing(unsigned amount) {
  const std::size_t max_layers = std::min(kWidth, kHeight) / 2U;
  for (std::size_t layer = 0; layer < max_layers; ++layer) {
    const std::size_t min_row = layer;
    const std::size_t max_row = kHeight - 1U - layer;
    const std::size_t min_col = layer;
    const std::size_t max_col = kWidth - 1U - layer;
    if (min_row >= max_row || min_col >= max_col) {
      break;
    }
    std::vector<std::size_t> ring_indices;
    for (std::size_t col = min_col; col <= max_col; ++col) {
      ring_indices.push_back(Offset(min_row, col));
    }
    for (std::size_t row = min_row + 1U; row <= max_row; ++row) {
      ring_indices.push_back(Offset(row, max_col));
    }
    for (std::size_t col = max_col; col-- > min_col;) {
      ring_indices.push_back(Offset(max_row, col));
    }
    for (std::size_t row = max_row; row-- > min_row + 1U;) {
      ring_indices.push_back(Offset(row, min_col));
    }
    if (ring_indices.empty()) {
      continue;
    }
    std::vector<std::uint8_t> copy(ring_indices.size());
    for (std::size_t i = 0; i < ring_indices.size(); ++i) {
      copy[i] = data_[ring_indices[i]];
    }
    const unsigned shift = amount % ring_indices.size();
    if (shift == 0U) {
      continue;
    }
    for (std::size_t i = 0; i < ring_indices.size(); ++i) {
      data_[ring_indices[(i + shift) % ring_indices.size()]] = copy[i];
    }
  }
}

void TyphoonMatrix::TwistCross(unsigned amount) {
  const std::size_t row_a = 3U;
  const std::size_t row_b = 4U;
  const std::size_t col_a = 7U;
  const std::size_t col_b = 8U;
  const unsigned row_shift = NormalizeRowAmount(amount);
  const unsigned col_shift = NormalizeColumnAmount(amount);
  for (std::size_t col = 0; col < kWidth; ++col) {
    At(row_a, col) = static_cast<std::uint8_t>(
        At(row_a, col) ^ At((col + row_shift) % kHeight, (col + col_shift) % kWidth));
    At(row_b, col) = static_cast<std::uint8_t>(
        At(row_b, col) + At((col * 3U + row_shift) % kHeight, col));
  }
  for (std::size_t row = 0; row < kHeight; ++row) {
    At(row, col_a) = static_cast<std::uint8_t>(
        At(row, col_a) ^ At(row, (row + col_shift) % kWidth) ^ static_cast<std::uint8_t>(FoldAdd() + row * 5U));
    At(row, col_b) = static_cast<std::uint8_t>(
        At(row, col_b) + At(row, (row * 3U + col_shift) % kWidth) + static_cast<std::uint8_t>(FoldXor() + row * 7U));
  }
}

void TyphoonMatrix::XorWith(const TyphoonMatrix& other) {
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] ^ other.data_[i]);
  }
}

void TyphoonMatrix::AddWith(const TyphoonMatrix& other) {
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] + other.data_[i]);
  }
}

void TyphoonMatrix::InjectXor(const std::uint8_t* bytes, std::size_t size, std::size_t start) {
  if (bytes == nullptr || size == 0U) {
    return;
  }
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] ^ bytes[(start + i) % size]);
  }
}

void TyphoonMatrix::InjectAdd(const std::uint8_t* bytes, std::size_t size, std::size_t start) {
  if (bytes == nullptr || size == 0U) {
    return;
  }
  for (std::size_t i = 0; i < kSize; ++i) {
    data_[i] = static_cast<std::uint8_t>(data_[i] + bytes[(start + i) % size]);
  }
}

std::uint8_t TyphoonMatrix::FoldXor() const {
  std::uint8_t value = 0U;
  for (std::uint8_t byte : data_) {
    value = static_cast<std::uint8_t>(value ^ byte);
  }
  return value;
}

std::uint8_t TyphoonMatrix::FoldAdd() const {
  std::uint8_t value = 0U;
  for (std::uint8_t byte : data_) {
    value = static_cast<std::uint8_t>(value + byte);
  }
  return value;
}

void TyphoonMatrix::ApplyFastOp(TyphoonFastOp op, std::uint8_t arg0, std::uint8_t arg1) {
  switch (op) {
    case TyphoonFastOp::kRotateRowLeft:
      RotateRowLeft(arg0, arg1);
      break;
    case TyphoonFastOp::kRotateRowRight:
      RotateRowRight(arg0, arg1);
      break;
    case TyphoonFastOp::kRotateColumnUp:
      RotateColumnUp(arg0, arg1);
      break;
    case TyphoonFastOp::kRotateColumnDown:
      RotateColumnDown(arg0, arg1);
      break;
    case TyphoonFastOp::kSwapRows:
      SwapRows(arg0, arg1);
      break;
    case TyphoonFastOp::kSwapColumns:
      SwapColumns(arg0, arg1);
      break;
    case TyphoonFastOp::kXorRowIntoRow:
      XorRowIntoRow(arg0, arg1);
      break;
    case TyphoonFastOp::kAddRowIntoRow:
      AddRowIntoRow(arg0, arg1);
      break;
    case TyphoonFastOp::kXorColumnIntoColumn:
      XorColumnIntoColumn(arg0, arg1);
      break;
    case TyphoonFastOp::kAddColumnIntoColumn:
      AddColumnIntoColumn(arg0, arg1);
      break;
    case TyphoonFastOp::kWeaveRows:
      WeaveRows(arg0, arg1);
      break;
    case TyphoonFastOp::kWeaveColumns:
      WeaveColumns(arg0, arg1);
      break;
  }
}

void TyphoonMatrix::ApplySlowOp(TyphoonSlowOp op, std::uint8_t arg0, std::uint8_t arg1) {
  static_cast<void>(arg1);
  switch (op) {
    case TyphoonSlowOp::kFlipHorizontal:
      FlipHorizontal();
      break;
    case TyphoonSlowOp::kFlipVertical:
      FlipVertical();
      break;
    case TyphoonSlowOp::kRotateRing:
      RotateRing(arg0);
      break;
    case TyphoonSlowOp::kTwistCross:
      TwistCross(arg0);
      break;
  }
}

std::size_t TyphoonMatrix::NormalizeRow(std::size_t value) {
  return value % kHeight;
}

std::size_t TyphoonMatrix::NormalizeColumn(std::size_t value) {
  return value % kWidth;
}

unsigned TyphoonMatrix::NormalizeRowAmount(unsigned value) {
  return value % kHeight;
}

unsigned TyphoonMatrix::NormalizeColumnAmount(unsigned value) {
  return value % kWidth;
}

std::size_t TyphoonMatrix::Offset(std::size_t row, std::size_t column) const {
  return (NormalizeRow(row) * kWidth) + NormalizeColumn(column);
}

}  // namespace twist
