#ifndef RX_RENDER_FRONTEND_BUFFER_H
#define RX_RENDER_FRONTEND_BUFFER_H
#include "rx/core/vector.h"

#include "rx/render/frontend/resource.h"

namespace rx::render::frontend {

struct interface;

struct buffer : resource {
  struct attribute {
    enum class type {
      k_f32,
      k_u8
    };
    rx_size count;
    rx_size offset;
    type kind;
  };

  enum class element_type {
    k_none,
    k_u8,
    k_u16,
    k_u32
  };

  enum class type {
    k_static,
    k_dynamic
  };

  struct edit {
    rx_size sink;
    rx_size offset;
    rx_size size;
  };

  buffer(interface* _frontend);
  ~buffer();

  // write |_size| bytes from |_data| into vertex store
  template<typename T>
  void write_vertices(const T* _data, rx_size _size);

  // write |_size| bytes from |_data| into element store
  template<typename T>
  void write_elements(const T* _data, rx_size _size);

  // map |_size| bytes of vertices
  rx_byte* map_vertices(rx_size _size);

  // map |_size| bytes of elements
  rx_byte* map_elements(rx_size _size);

  // record attribute of |_count| elements of |_type| starting at |_offset|
  void record_attribute(attribute::type _type, rx_size _count, rx_size _offset);

  // record buffer type |_type|
  void record_type(type _type);

  // record vertex stride |_stride|
  void record_stride(rx_size _stride);

  // record element format |_type|
  void record_element_type(element_type _type);

  // Records an edit to the buffer at byte |_offset| of size |_size|.
  void record_vertices_edit(rx_size _offset, rx_size _size);
  void record_elements_edit(rx_size _offset, rx_size _size);

  const vector<rx_byte>& vertices() const &;
  const vector<rx_byte>& elements() const &;
  const vector<attribute>& attributes() const &;
  rx_size stride() const;
  element_type element_kind() const;
  type kind() const;
  rx_size size() const;
  vector<edit>&& edits();

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

  vector<rx_byte> m_vertices_store;
  vector<rx_byte> m_elements_store;
  vector<attribute> m_attributes;
  vector<edit> m_edits;
  element_type m_element_type;
  type m_type;
  rx_size m_stride;
  int m_recorded;
};

template<typename T>
inline void buffer::write_vertices(const T* _data, rx_size _size) {
  write_vertices_data(reinterpret_cast<const rx_byte*>(_data), _size);
}

template<typename T>
inline void buffer::write_elements(const T* _data, rx_size _size) {
  static_assert((traits::is_same<T, rx_byte> || traits::is_same<T, rx_u16>
    || traits::is_same<T, rx_u32>), "unsupported element type T");

  RX_ASSERT(_size % sizeof(T) == 0, "_size isn't a multiple of T");

  if constexpr (traits::is_same<T, rx_byte>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, rx_u16>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, rx_u32>) {
    write_elements_data(reinterpret_cast<const rx_byte*>(_data), _size);
  }
}

inline void buffer::record_attribute(attribute::type _type, rx_size _count,
  rx_size _offset)
{
  m_recorded |= k_attribute;
  m_attributes.push_back({_count, _offset, _type});
}

inline void buffer::record_type(type _type) {
  RX_ASSERT(!(m_recorded & k_type), "already recorded type");
  m_recorded |= k_type;
  m_type = _type;
}

inline void buffer::record_stride(rx_size _stride) {
  RX_ASSERT(!(m_recorded & k_stride), "already recorded stride");
  m_recorded |= k_stride;
  m_stride = _stride;
}

inline void buffer::record_element_type(element_type _type) {
  RX_ASSERT(!(m_recorded & k_element_type), "already recorded element type");
  m_recorded |= k_element_type;
  m_element_type = _type;
}

inline void buffer::record_vertices_edit(rx_size _offset, rx_size _size) {
  m_edits.emplace_back(0_z, _offset, _size);
}

inline void buffer::record_elements_edit(rx_size _offset, rx_size _size) {
  RX_ASSERT(m_element_type != element_type::k_none,
    "cannot record edit to elements");
  m_edits.emplace_back(1_z, _offset, _size);
}

inline const vector<rx_byte>& buffer::vertices() const & {
  return m_vertices_store;
}

inline const vector<rx_byte>& buffer::elements() const & {
  return m_elements_store;
}

inline const vector<buffer::attribute>& buffer::attributes() const & {
  return m_attributes;
}

inline rx_size buffer::stride() const {
  return m_stride;
}

inline buffer::element_type buffer::element_kind() const {
  return m_element_type;
}

inline buffer::type buffer::kind() const {
  return m_type;
}

inline rx_size buffer::size() const {
  return m_vertices_store.size() + m_elements_store.size();
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_BUFFER_H
