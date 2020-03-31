#ifndef RX_IMAGE_MATRIX_H
#define RX_IMAGE_MATRIX_H
#include "rx/core/vector.h"
#include "rx/math/vec2.h"

namespace rx::image {

struct matrix
{
  constexpr matrix(memory::allocator& _allocator);
  constexpr matrix();

  matrix(memory::allocator& _allocator, const math::vec2z& _dimensions, rx_size _channels);
  matrix(const math::vec2z& _dimensions, rx_size _channels);

  matrix(matrix&& matrix_);
  matrix(const matrix& _matrix);

  bool resize(const math::vec2z& _dimensions, rx_size _channels);

  matrix& operator=(matrix&& matrix_);
  matrix& operator=(const matrix& _matrix);

  matrix scaled(const math::vec2z& _dimensions) const;

  const vector<rx_f32>& data() const &;

  const rx_f32& operator[](rx_size _index) const;
  rx_f32& operator[](rx_size _index);

  const rx_f32* operator()(rx_size _x, rx_size _y) const;
  rx_f32* operator()(rx_size _x, rx_size _y);

  const math::vec2z& dimensions() const &;
  rx_size channels() const;

  constexpr memory::allocator& allocator() const;

private:
  ref<memory::allocator> m_allocator;
  vector<rx_f32> m_data;
  math::vec2z m_dimensions;
  rx_size m_channels;
};

inline constexpr matrix::matrix(memory::allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{allocator()}
  , m_channels{0}
{
}

inline constexpr matrix::matrix()
  : matrix{memory::system_allocator::instance()}
{
}

inline matrix::matrix(memory::allocator& _allocator, const math::vec2z& _dimensions, rx_size _channels)
  : m_allocator{_allocator}
  , m_data{allocator()}
{
  RX_ASSERT(resize(_dimensions, _channels), "out of memory");
}

inline matrix::matrix(const math::vec2z& _dimensions, rx_size _channels)
  : matrix{memory::system_allocator::instance(), _dimensions, _channels}
{
}

inline matrix::matrix(matrix&& matrix_)
  : m_allocator{matrix_.m_allocator}
  , m_data{utility::move(matrix_.m_data)}
  , m_dimensions{utility::exchange(matrix_.m_dimensions, math::vec2z{})}
  , m_channels{utility::exchange(matrix_.m_channels, 0)}
{
}

inline matrix::matrix(const matrix& _matrix)
  : m_allocator{_matrix.m_allocator}
  , m_data{_matrix.m_data}
  , m_dimensions{_matrix.m_dimensions}
  , m_channels{_matrix.m_channels}
{
}

inline bool matrix::resize(const math::vec2z& _dimensions, rx_size _channels) {
  m_dimensions = _dimensions;
  m_channels = _channels;
  return m_data.resize(m_dimensions.area() * m_channels, utility::uninitialized{});
}

inline matrix& matrix::operator=(matrix&& matrix_) {
  RX_ASSERT(this != &matrix_, "self move");

  m_allocator = matrix_.m_allocator;
  m_data = utility::move(matrix_.m_data);
  m_dimensions = utility::exchange(matrix_.m_dimensions, math::vec2z{});
  m_channels = utility::exchange(matrix_.m_channels, 0);

  return *this;
}

inline matrix& matrix::operator=(const matrix& _matrix) {
  RX_ASSERT(this != &_matrix, "self assignment");

  m_data = _matrix.m_data;
  m_dimensions = _matrix.m_dimensions;
  m_channels = _matrix.m_channels;

  return *this;
}

inline const vector<rx_f32>& matrix::data() const & {
  return m_data;
}

inline const rx_f32& matrix::operator[](rx_size _index) const {
  return m_data[_index];
}

inline rx_f32& matrix::operator[](rx_size _index) {
  return m_data[_index];
}

inline const rx_f32* matrix::operator()(rx_size _x, rx_size _y) const {
  RX_ASSERT(!m_data.is_empty(), "empty matrix");
  return m_data.data() + (m_dimensions.w * _y + _x) * m_channels;
}

inline rx_f32* matrix::operator()(rx_size _x, rx_size _y) {
  RX_ASSERT(!m_data.is_empty(), "empty matrix");
  return m_data.data() + (m_dimensions.w * _y + _x) * m_channels;
}

inline const math::vec2z& matrix::dimensions() const & {
  return m_dimensions;
}

inline rx_size matrix::channels() const {
  return m_channels;
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& matrix::allocator() const {
  return m_allocator;
}

} // namespace rx::image

#endif // RX_IMAGE_MATRIX_H
