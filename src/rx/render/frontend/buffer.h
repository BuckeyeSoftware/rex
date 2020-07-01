#ifndef RX_RENDER_FRONTEND_BUFFER_H
#define RX_RENDER_FRONTEND_BUFFER_H
#include "rx/core/vector.h"

#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;

struct Buffer : Resource {
  struct Attribute {
    enum class Type {
      // Scalars
      k_f32,    // 1 x Float32

      // Vectors
      k_vec2f,  // 2 x Float32
      k_vec3f,  // 3 x Float32
      k_vec4f,  // 4 x Float32
      k_vec4b,  // 4 x Byte

      // Matrices
      k_mat4x4f // 4x4 x Float32
    };

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

  // write |_size| bytes from |_data| into instance store
  template<typename T>
  void write_instances(const T* _data, Size _size);

  // map |_size| bytes of vertices
  Byte* map_vertices(Size _size);

  // map |_size| bytes of elements
  Byte* map_elements(Size _size);

  // map |_size| bytes of instances
  Byte* map_instances(Size _size);

  // record vertex attribute of |_type| starting at |_offset|
  void record_vertex_attribute(Attribute::Type _type, Size _offset);

  // record instance attribute of |_type| starting at |_offset|
  void record_instance_attribute(Attribute::Type _type, Size _offset);

  // record buffer Type |_type|
  void record_type(Type _type);

  // record if instanced.
  void record_instanced(bool _instanced);

  // record vertex stride |_stride|
  void record_vertex_stride(Size _stride);

  // record instance stride |_stride|
  void record_instance_stride(Size _stride);

  // record element format |_type|
  void record_element_type(ElementType _type);

  // Records an edit to the buffer at byte |_offset| of size |_size|.
  void record_vertices_edit(Size _offset, Size _size);
  void record_elements_edit(Size _offset, Size _size);
  void record_instances_edit(Size _offset, Size _size);

  const Vector<Byte>& vertices() const &;
  const Vector<Byte>& elements() const &;
  const Vector<Byte>& instances() const &;

  const Vector<Attribute>& vertex_attributes() const &;
  const Vector<Attribute>& instance_attributes() const &;

  Size vertex_stride() const;
  Size instance_stride() const;

  ElementType element_type() const;
  Size element_size() const;

  Type type() const;
  Size size() const;

  Vector<Edit>&& edits();

  void validate() const;

  bool is_instanced() const;
  bool is_indexed() const;

private:
  void write_vertices_data(const Byte* _data, Size _size);
  void write_elements_data(const Byte* _data, Size _size);
  void write_instances_data(const Byte* _data, Size _size);

  enum {
    k_vertex_stride       = 1 << 0,
    k_instance_stride     = 1 << 1,
    k_instanced           = 1 << 2,
    k_type                = 1 << 3,
    k_element_type        = 1 << 4,
    k_vertex_attribute    = 1 << 5,
    k_instance_attribute  = 1 << 6
  };

  Vector<Byte> m_vertices_store;
  Vector<Byte> m_elements_store;
  Vector<Byte> m_instances_store;

  Vector<Attribute> m_vertex_attributes;
  Vector<Attribute> m_instance_attributes;

  Vector<Edit> m_edits;

  ElementType m_element_type;

  Type m_type;
  bool m_instanced;

  Size m_vertex_stride;
  Size m_instance_stride;

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

inline void Buffer::record_vertex_attribute(Attribute::Type _type, Size _offset) {
  m_recorded |= k_vertex_attribute;
  m_vertex_attributes.emplace_back(_offset, _type);
}

inline void Buffer::record_instance_attribute(Attribute::Type _type, Size _offset) {
  RX_ASSERT(m_recorded & k_instanced, "not an instanced buffer");
  m_recorded |= k_instance_attribute;
  m_instance_attributes.emplace_back(_offset, _type);
}

inline void Buffer::record_type(Type _type) {
  RX_ASSERT(!(m_recorded & k_type), "already recorded type");
  m_recorded |= k_type;
  m_type = _type;
}

inline void Buffer::record_instanced(bool _instanced) {
  RX_ASSERT(!(m_recorded & k_instanced), "already recorded instanced");
  m_recorded |= k_instanced;
  m_instanced = _instanced;
}

inline void Buffer::record_vertex_stride(Size _stride) {
  RX_ASSERT(!(m_recorded & k_vertex_stride), "already recorded vertex stride");
  m_recorded |= k_vertex_stride;
  m_vertex_stride = _stride;
}

inline void Buffer::record_instance_stride(Size _stride) {
  RX_ASSERT(!(m_recorded & k_instance_stride), "already recorded instance stride");
  m_recorded |= k_instance_stride;
  m_instance_stride = _stride;
}

inline void Buffer::record_element_type(ElementType _type) {
  RX_ASSERT(!(m_recorded & k_element_type), "already recorded element Type");
  m_recorded |= k_element_type;
  m_element_type = _type;
}

inline void Buffer::record_elements_edit(Size _offset, Size _size) {
  RX_ASSERT(m_element_type != ElementType::k_none,
    "cannot record edit to elements");
  m_edits.emplace_back(0_z, _offset, _size);
}

inline void Buffer::record_vertices_edit(Size _offset, Size _size) {
  m_edits.emplace_back(1_z, _offset, _size);
}

inline void Buffer::record_instances_edit(Size _offset, Size _size) {
  RX_ASSERT(is_instanced(), "cannot record edit to instances");
  m_edits.emplace_back(2_z, _offset, _size);
}

inline const Vector<Byte>& Buffer::vertices() const & {
  return m_vertices_store;
}

inline const Vector<Byte>& Buffer::elements() const & {
  return m_elements_store;
}

inline const Vector<Byte>& Buffer::instances() const & {
  return m_instances_store;
}

inline const Vector<Buffer::Attribute>& Buffer::vertex_attributes() const & {
  return m_vertex_attributes;
}

inline const Vector<Buffer::Attribute>& Buffer::instance_attributes() const & {
  return m_instance_attributes;
}

inline Size Buffer::vertex_stride() const {
  return m_vertex_stride;
}

inline Size Buffer::instance_stride() const {
  return m_instance_stride;
}

inline Buffer::ElementType Buffer::element_type() const {
  return m_element_type;
}

inline Size Buffer::element_size() const {
  switch (m_element_type) {
  case Buffer::ElementType::k_none:
    return 0;
  case Buffer::ElementType::k_u8:
    return 1;
  case Buffer::ElementType::k_u16:
    return 2;
  case Buffer::ElementType::k_u32:
    return 4;
  }
  RX_HINT_UNREACHABLE();
}

inline Buffer::Type Buffer::type() const {
  return m_type;
}

inline Size Buffer::size() const {
  return m_vertices_store.size() + m_elements_store.size() + m_instances_store.size();
}

inline bool Buffer::is_instanced() const {
  return m_instanced;
}

inline bool Buffer::is_indexed() const {
  return m_element_type != ElementType::k_none;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_BUFFER_H
