#ifndef RX_RENDER_FRONTEND_BUFFER_H
#define RX_RENDER_FRONTEND_BUFFER_H
#include "rx/core/vector.h"

#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;

struct Buffer : Resource {
  struct Attribute {
    enum class Type {
      k_f32,
      k_u8
    };
    Size count;
    Size offset;
    Type type;
  };

  enum class ElementType {
    k_none,
    k_u8,
    k_u16,
    k_u32
  };

  enum class Type {
    k_static,
    k_dynamic
  };

  struct Edit {
    Size sink;
    Size offset;
    Size size;
  };

  Buffer(Context* _frontend);
  ~Buffer();

  // write |_size| bytes from |_data| into vertex store
  template<typename T>
  void write_vertices(const T* _data, Size _size);

  // write |_size| bytes from |_data| into element store
  template<typename T>
  void write_elements(const T* _data, Size _size);

  // map |_size| bytes of vertices
  Byte* map_vertices(Size _size);

  // map |_size| bytes of elements
  Byte* map_elements(Size _size);

  // record attribute of |_count| elements of |_type| starting at |_offset|
  void record_attribute(Attribute::Type _type, Size _count, Size _offset);

  // record buffer Type |_type|
  void record_type(Type _type);

  // record vertex stride |_stride|
  void record_stride(Size _stride);

  // record element format |_type|
  void record_element_type(ElementType _type);

  // Records an edit to the buffer at byte |_offset| of size |_size|.
  void record_vertices_edit(Size _offset, Size _size);
  void record_elements_edit(Size _offset, Size _size);

  const Vector<Byte>& vertices() const &;
  const Vector<Byte>& elements() const &;
  const Vector<Attribute>& attributes() const &;
  Size stride() const;
  ElementType element_type() const;
  Type type() const;
  Size size() const;
  Vector<Edit>&& edits();

  void validate() const;

private:
  void write_vertices_data(const Byte* _data, Size _size);
  void write_elements_data(const Byte* _data, Size _size);

  enum {
    k_stride       = 1 << 0,
    k_type         = 1 << 1,
    k_element_type = 1 << 2,
    k_attribute    = 1 << 3
  };

  Vector<Byte> m_vertices_store;
  Vector<Byte> m_elements_store;
  Vector<Attribute> m_attributes;
  Vector<Edit> m_edits;
  ElementType m_element_type;
  Type m_type;
  Size m_stride;
  int m_recorded;
};

template<typename T>
inline void Buffer::write_vertices(const T* _data, Size _size) {
  write_vertices_data(reinterpret_cast<const Byte*>(_data), _size);
}

template<typename T>
inline void Buffer::write_elements(const T* _data, Size _size) {
  static_assert((traits::is_same<T, Byte> || traits::is_same<T, Uint16>
                 || traits::is_same<T, Uint32>), "unsupported element Type T");

  RX_ASSERT(_size % sizeof(T) == 0, "_size isn't a multiple of T");

  if constexpr (traits::is_same<T, Byte>) {
    write_elements_data(reinterpret_cast<const Byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, Uint16>) {
    write_elements_data(reinterpret_cast<const Byte*>(_data), _size);
  } else if constexpr (traits::is_same<T, Uint32>) {
    write_elements_data(reinterpret_cast<const Byte*>(_data), _size);
  }
}

inline void Buffer::record_attribute(Attribute::Type _type, Size _count,
  Size _offset)
{
  m_recorded |= k_attribute;
  m_attributes.push_back({_count, _offset, _type});
}

inline void Buffer::record_type(Type _type) {
  RX_ASSERT(!(m_recorded & k_type), "already recorded Type");
  m_recorded |= k_type;
  m_type = _type;
}

inline void Buffer::record_stride(Size _stride) {
  RX_ASSERT(!(m_recorded & k_stride), "already recorded stride");
  m_recorded |= k_stride;
  m_stride = _stride;
}

inline void Buffer::record_element_type(ElementType _type) {
  RX_ASSERT(!(m_recorded & k_element_type), "already recorded element Type");
  m_recorded |= k_element_type;
  m_element_type = _type;
}

inline void Buffer::record_vertices_edit(Size _offset, Size _size) {
  m_edits.emplace_back(0_z, _offset, _size);
}

inline void Buffer::record_elements_edit(Size _offset, Size _size) {
  RX_ASSERT(m_element_type != ElementType::k_none,
    "cannot record edit to elements");
  m_edits.emplace_back(1_z, _offset, _size);
}

inline const Vector<Byte>& Buffer::vertices() const & {
  return m_vertices_store;
}

inline const Vector<Byte>& Buffer::elements() const & {
  return m_elements_store;
}

inline const Vector<Buffer::Attribute>& Buffer::attributes() const & {
  return m_attributes;
}

inline Size Buffer::stride() const {
  return m_stride;
}

inline Buffer::ElementType Buffer::element_type() const {
  return m_element_type;
}

inline Buffer::Type Buffer::type() const {
  return m_type;
}

inline Size Buffer::size() const {
  return m_vertices_store.size() + m_elements_store.size();
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_BUFFER_H
