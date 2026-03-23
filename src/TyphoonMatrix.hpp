#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace twist {

enum class TyphoonFastOp : std::uint8_t {
  kRotateRowLeft = 0,
  kRotateRowRight = 1,
  kRotateColumnUp = 2,
  kRotateColumnDown = 3,
  kSwapRows = 4,
  kSwapColumns = 5,
  kXorRowIntoRow = 6,
  kAddRowIntoRow = 7,
  kXorColumnIntoColumn = 8,
  kAddColumnIntoColumn = 9,
  kWeaveRows = 10,
  kWeaveColumns = 11,
};

enum class TyphoonSlowOp : std::uint8_t {
  kFlipHorizontal = 0,
  kFlipVertical = 1,
  kRotateRing = 2,
  kTwistCross = 3,
};

class TyphoonMatrix final {
 public:
  static constexpr std::size_t kWidth = 16;
  static constexpr std::size_t kHeight = 8;
  static constexpr std::size_t kSize = kWidth * kHeight;

  TyphoonMatrix();
  explicit TyphoonMatrix(const std::uint8_t* bytes);

  std::uint8_t* Data();
  const std::uint8_t* Data() const;

  void Clear();
  void Fill(std::uint8_t value);
  void Load(const std::uint8_t* bytes);
  void Store(std::uint8_t* bytes) const;

  std::uint8_t& At(std::size_t row, std::size_t column);
  std::uint8_t At(std::size_t row, std::size_t column) const;

  void RotateRowLeft(std::size_t row, unsigned amount);
  void RotateRowRight(std::size_t row, unsigned amount);
  void RotateColumnUp(std::size_t column, unsigned amount);
  void RotateColumnDown(std::size_t column, unsigned amount);
  void SwapRows(std::size_t row_a, std::size_t row_b);
  void SwapColumns(std::size_t column_a, std::size_t column_b);
  void XorRowIntoRow(std::size_t dst_row, std::size_t src_row);
  void AddRowIntoRow(std::size_t dst_row, std::size_t src_row);
  void XorColumnIntoColumn(std::size_t dst_column, std::size_t src_column);
  void AddColumnIntoColumn(std::size_t dst_column, std::size_t src_column);
  void WeaveRows(std::size_t row_a, std::size_t row_b);
  void WeaveColumns(std::size_t column_a, std::size_t column_b);

  void FlipHorizontal();
  void FlipVertical();
  void RotateRing(unsigned amount);
  void TwistCross(unsigned amount);

  void XorWith(const TyphoonMatrix& other);
  void AddWith(const TyphoonMatrix& other);
  void InjectXor(const std::uint8_t* bytes, std::size_t size, std::size_t start = 0);
  void InjectAdd(const std::uint8_t* bytes, std::size_t size, std::size_t start = 0);

  std::uint8_t FoldXor() const;
  std::uint8_t FoldAdd() const;

  void ApplyFastOp(TyphoonFastOp op, std::uint8_t arg0, std::uint8_t arg1);
  void ApplySlowOp(TyphoonSlowOp op, std::uint8_t arg0, std::uint8_t arg1);

 private:
  static std::size_t NormalizeRow(std::size_t value);
  static std::size_t NormalizeColumn(std::size_t value);
  static unsigned NormalizeRowAmount(unsigned value);
  static unsigned NormalizeColumnAmount(unsigned value);
  std::size_t Offset(std::size_t row, std::size_t column) const;

  std::array<std::uint8_t, kSize> data_{};
};

}  // namespace twist
