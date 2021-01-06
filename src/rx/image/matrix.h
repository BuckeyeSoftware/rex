#ifndef RX_IMAGE_MATRIX_H
#define RX_IMAGE_MATRIX_H
#include "rx/core/linear_buffer.h"

#include "rx/math/vec4.h"
#include "rx/math/vec2.h"

#include "rx/core/string.h"

namespace Rx::Image {

// RGBA_F32
struct Matrix {
  RX_MARK_NO_COPY(Matrix);

  constexpr Matrix() = default;

  static Optional<Matrix> create_from_bytes(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions, const Byte* _data);
  static Optional<Matrix> create(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions);

  Matrix(Matrix&& matrix_);
  Matrix& operator=(Matrix&& matrix_);

  Math::Vec4f* data();
  const Math::Vec4f* data() const;

  Math::Vec4f& operator()(Size _x, Size _y);
  const Math::Vec4f& operator()(Size _x, Size _y) const;

  const Math::Vec2z& dimensions() const;
  Size samples() const;

  constexpr Memory::Allocator& allocator() const;

private:
  LinearBuffer m_data;
  Math::Vec2z m_dimensions;
};

inline Matrix::Matrix(Matrix&& matrix_)
  : m_data{Utility::move(matrix_.m_data)}
  , m_dimensions{Utility::exchange(matrix_.m_dimensions, Math::Vec2z{})}
{
}

inline Matrix& Matrix::operator=(Matrix&& matrix_) {
  if (&matrix_ == this) {
    return *this;
  }

  m_data = Utility::move(matrix_.m_data);
  m_dimensions = Utility::exchange(matrix_.m_dimensions, Math::Vec2z{});

  return *this;
}

inline Math::Vec4f* Matrix::data() {
  return reinterpret_cast<Math::Vec4f*>(m_data.data());
}

inline const Math::Vec4f* Matrix::data() const {
  return reinterpret_cast<const Math::Vec4f*>(m_data.data());
}

inline Math::Vec4f& Matrix::operator()(Size _x, Size _y) {
  const auto index = (m_dimensions.w * _y + _x) * sizeof(Math::Vec4f);
  RX_ASSERT(m_data.in_range(index), "out of bounds %zu >= %zu", index, m_data.size());
  return *reinterpret_cast<Math::Vec4f*>(m_data.data() + index);
}

inline const Math::Vec4f& Matrix::operator()(Size _x, Size _y) const {
  const auto index = (m_dimensions.w * _y + _x) * sizeof(Math::Vec4f);
  RX_ASSERT(m_data.in_range(index), "out of bounds %zu >= %zu", index, m_data.size());
  return *reinterpret_cast<const Math::Vec4f*>(m_data.data() + index);
}

inline const Math::Vec2z& Matrix::dimensions() const {
  return m_dimensions;
}

inline Size Matrix::samples() const {
  return m_dimensions.area();
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Matrix::allocator() const {
  return m_data.allocator();
}

} // namespace Rx::Image

#endif // RX_IMAGE_MATRIX_H
