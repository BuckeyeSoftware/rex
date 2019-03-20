#ifndef RX_RENDER_BUFFER_H
#define RX_RENDER_BUFFER_H

#include <rx/core/array.h>
#include <rx/render/resource.h>

namespace rx::render {

struct frontend;
struct buffer : resource {
  struct attribute {
    enum class category {
      k_f32,
      k_u8
    };
    rx_size count;
    rx_size offset;
    category type;
  };

  enum class element_category {
    k_none,
    k_u8,
    k_u16,
    k_u32
  };

  enum class category {
    k_static,
    k_dynamic
  };

  buffer(frontend* _frontend);
  ~buffer();

  // write |_size| bytes from |_data| into vertex store
  template<typename T>
  void write_vertices(const T* _data, rx_size _size);

  // write |_size| bytes from |_data| into element store
  template<typename T>
  void write_elements(const T* _data, rx_size _size);

  // record attribute of |_count| elements of |_type| starting at |_offset|
  void record_attribute(attribute::category _type, rx_size _count, rx_size _offset);

  // record vertex stride |_stride|
  void record_stride(rx_size _stride);

  // record element format |_type|
  void record_element_type(element_category _type);

  // record buffer type |_type|
  void record_type(category _type);

  // flush vertex and element store
  void flush();

  const array<rx_byte>& vertices() const &;
  const array<rx_byte>& elements() const &;
  const array<attribute>& attributes() const &;
  rx_size stride() const;
  element_category element_type() const;
  category type() const;

  void validate() const;

private:
  void write_vertices_data(const rx_byte* _data, rx_size _size);
  void write_elements_data(const rx_byte* _data, rx_size _size);

  enum {
    k_stride       = 1 << 0,
    k_type         = 1 << 1,
    k_element_type = 1 << 2,
    k_attribute    = 1 << 3
  };

  array<rx_byte> m_vertices_store;
  array<rx_byte> m_elements_store;
  array<attribute> m_attributes;
  element_category m_element_type;
  category m_type;
  rx_size m_stride;
  int m_recorded;
};

template<typename T>
inline void buffer::write_vertices(const T* _data, rx_size _size) {
  write_vertices_data(reinterpret_cast<const rx_byte*>(_data), _size);
}

template<typename T>
inline void buffer::write_elements(const T* _data, rx_size _size) {
  static_assert((traits::is_same<T, rx_byte> || traits::is_same<T, rx_u16> || traits::is_same<T, rx_u32>),
    "unsupported element type T");

  RX_ASSERT(_size % sizeof(T) == 0, "_size isn't a multiple of T");

  if constexpr (traits::is_same<T, rx_byte>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, rx_u16>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, rx_u32>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  }
}

inline void buffer::record_attribute(attribute::category _type, rx_size _count, rx_size _offset) {
  m_recorded |= k_attribute;
  m_attributes.push_back({_count, _offset, _type});
}

inline void buffer::record_type(category _type) {
  RX_ASSERT(!(m_recorded & k_type), "already recorded type");
  m_recorded |= k_type;
  m_type = _type;
}

inline void buffer::record_stride(rx_size _stride) {
  RX_ASSERT(!(m_recorded & k_stride), "already recorded stride");
  m_recorded |= k_stride;
  m_stride = _stride;
}

inline void buffer::record_element_type(element_category _type) {
  RX_ASSERT(!(m_recorded & k_element_type), "already recorded element type");
  m_recorded |= k_element_type;
  m_element_type = _type;
}

inline const array<rx_byte>& buffer::vertices() const & {
  return m_vertices_store;
}

inline const array<rx_byte>& buffer::elements() const & {
  return m_elements_store;
}

inline const array<buffer::attribute>& buffer::attributes() const & {
  return m_attributes;
}

inline rx_size buffer::stride() const {
  return m_stride;
}

inline buffer::element_category buffer::element_type() const {
  return m_element_type;
}

inline buffer::category buffer::type() const {
  return m_type;
}

} // namespace rx::render

#endif
