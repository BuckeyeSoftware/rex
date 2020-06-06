#ifndef RX_IMAGE_MATRIX_H
#define RX_IMAGE_MATRIX_H
#include "rx/core/vector.h"
#include "rx/math/vec2.h"

namespace Rx::Image {

struct Matrix
{
  constexpr Matrix(Memory::Allocator& _allocator);
  constexpr Matrix();

  Matrix(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions, Size _channels);
  Matrix(const Math::Vec2z& _dimensions, Size _channels);

  Matrix(Matrix&& matrix_);
  Matrix(const Matrix& _matrix);

  bool resize(const Math::Vec2z& _dimensions, Size _channels);

  Matrix& operator=(Matrix&& matrix_);
  Matrix& operator=(const Matrix& _matrix);

  Matrix scaled(const Math::Vec2z& _dimensions) const;

  const Vector<Float32>& data() const &;

  const Float32& operator[](Size _index) const;
  Float32& operator[](Size _index);

  const Float32* operator()(Size _x, Size _y) const;
  Float32* operator()(Size _x, Size _y);

  const Math::Vec2z& dimensions() const &;
  Size channels() const;

  constexpr Memory::Allocator& allocator() const;

private:
  Ref<Memory::Allocator> m_allocator;
  Vector<Float32> m_data;
  Math::Vec2z m_dimensions;
  Size m_channels;
};

inline constexpr Matrix::Matrix(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{allocator()}
  , m_channels{0}
{
}

inline constexpr Matrix::Matrix()
  : Matrix{Memory::SystemAllocator::instance()}
{
}

inline Matrix::Matrix(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions, Size _channels)
  : m_allocator{_allocator}
  , m_data{allocator()}
{
  RX_ASSERT(resize(_dimensions, _channels), "out of memory");
}

inline Matrix::Matrix(const Math::Vec2z& _dimensions, Size _channels)
  : Matrix{Memory::SystemAllocator::instance(), _dimensions, _channels}
{
}

inline Matrix::Matrix(Matrix&& matrix_)
  : m_allocator{matrix_.m_allocator}
  , m_data{Utility::move(matrix_.m_data)}
  , m_dimensions{Utility::exchange(matrix_.m_dimensions, Math::Vec2z{})}
  , m_channels{Utility::exchange(matrix_.m_channels, 0)}
{
}

inline Matrix::Matrix(const Matrix& _matrix)
  : m_allocator{_matrix.m_allocator}
  , m_data{_matrix.m_data}
  , m_dimensions{_matrix.m_dimensions}
  , m_channels{_matrix.m_channels}
{
}

inline bool Matrix::resize(const Math::Vec2z& _dimensions, Size _channels) {
  m_dimensions = _dimensions;
  m_channels = _channels;
  return m_data.resize(m_dimensions.area() * m_channels, Utility::UninitializedTag{});
}

inline Matrix& Matrix::operator=(Matrix&& matrix_) {
  RX_ASSERT(this != &matrix_, "self move");

  m_allocator = matrix_.m_allocator;
  m_data = Utility::move(matrix_.m_data);
  m_dimensions = Utility::exchange(matrix_.m_dimensions, Math::Vec2z{});
  m_channels = Utility::exchange(matrix_.m_channels, 0);

  return *this;
}

inline Matrix& Matrix::operator=(const Matrix& _matrix) {
  RX_ASSERT(this != &_matrix, "self assignment");

  m_data = _matrix.m_data;
  m_dimensions = _matrix.m_dimensions;
  m_channels = _matrix.m_channels;

  return *this;
}

inline const Vector<Float32>& Matrix::data() const & {
  return m_data;
}

inline const Float32& Matrix::operator[](Size _index) const {
  return m_data[_index];
}

inline Float32& Matrix::operator[](Size _index) {
  return m_data[_index];
}

inline const Float32* Matrix::operator()(Size _x, Size _y) const {
  RX_ASSERT(!m_data.is_empty(), "empty matrix");
  return m_data.data() + (m_dimensions.w * _y + _x) * m_channels;
}

inline Float32* Matrix::operator()(Size _x, Size _y) {
  RX_ASSERT(!m_data.is_empty(), "empty matrix");
  return m_data.data() + (m_dimensions.w * _y + _x) * m_channels;
}

inline const Math::Vec2z& Matrix::dimensions() const & {
  return m_dimensions;
}

inline Size Matrix::channels() const {
  return m_channels;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Matrix::allocator() const {
  return m_allocator;
}

} // namespace rx::image

#endif // RX_IMAGE_MATRIX_H
