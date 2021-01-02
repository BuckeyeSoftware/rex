#ifndef RX_RENDER_FRONTEND_BUFFER_H
#define RX_RENDER_FRONTEND_BUFFER_H
#include "rx/core/linear_buffer.h"
#include "rx/core/vector.h"
#include "rx/core/hash.h"

#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;

struct Buffer : Resource {
  struct Attribute {
    enum class Type : Uint8 {
      // Scalars
      F32,    // 1 x Float32

      // Vectors
      VEC2F,  // 2 x Float32
      VEC3F,  // 3 x Float32
      VEC4F,  // 4 x Float32
      VEC4B,  // 4 x Byte

      // Matrices
      MAT4X4F // 4x4 x Float32
    };

    bool operator!=(const Attribute& _other) const;
    Size hash() const;

    Type type;
    Size offset;
  };

  enum class ElementType : Uint32 {
    NONE,
    U8,
    U16,
    U32
  };

  enum class Type : Uint32 {
    STATIC,
    DYNAMIC
  };

  // Type that describes the format.
  struct Format {
    Format();

    void record_type(Type _type);

    void record_element_type(ElementType _type);

    void record_vertex_stride(Size _stride);
    void record_instance_stride(Size _stride);

    bool record_vertex_attribute(const Attribute& _attribute);
    bool record_instance_attribute(const Attribute& _attribute);

    bool is_indexed() const;
    bool is_instanced() const;

    Type type() const;
    ElementType element_type() const;

    const Vector<Attribute>& vertex_attributes() const &;
    const Vector<Attribute>& instance_attributes() const &;

    Size vertex_stride() const;
    Size instance_stride() const;
    Size element_size() const;

    bool operator!=(const Format& _format) const;
    bool operator==(const Format& _format) const;

    void finalize();
    Size hash() const;

  private:
    enum {
      TYPE            = 1 << 0,
      ELEMENT_TYPE    = 1 << 1,
      VERTEX_STRIDE   = 1 << 2,
      INSTANCE_STRIDE = 1 << 3,
      FINALIZED       = 1 << 4
    };

    Uint32 m_flags;
    Type m_type;
    ElementType m_element_type;
    Size m_vertex_stride;
    Size m_instance_stride;
    Vector<Attribute> m_vertex_attributes;
    Vector<Attribute> m_instance_attributes;
    Size m_hash;
  };

  enum class Sink : Size {
    ELEMENTS,
    VERTICES,
    INSTANCES
  };

  struct Edit {
    Sink sink;
    Size offset;
    Size size;
  };

  Buffer(Context* _frontend);
  ~Buffer();

  // Record buffer format.
  void record_format(const Format& _format);

  // Map |_size| bytes for vertices.
  Byte* map_vertices(Size _size);
  // Map |_size| bytes for elements.
  Byte* map_elements(Size _size);
  // Map |_size| bytes for instances.
  Byte* map_instances(Size _size);

  // Write |_size| bytes of vertices from |_data|.
  template<typename T>
  bool write_vertices(const T* _data, Size _size);
  // Write |_size| bytes of elements from |_data|.
  template<typename T>
  bool write_elements(const T* _data, Size _size);
  // Write |_size| bytes of instances from |_data|.
  template<typename T>
  bool write_instances(const T* _data, Size _size);

  bool record_vertices_edit(Size _offset, Size _size);
  bool record_elements_edit(Size _offset, Size _size);
  bool record_instances_edit(Size _offset, Size _size);

  const LinearBuffer& vertices() const &;
  const LinearBuffer& elements() const &;
  const LinearBuffer& instances() const &;

  Size size() const;

  const Vector<Edit>& edits() const;
  Size bytes_for_edits() const;
  void optimize_edits();
  void clear_edits();

  void validate() const;

  const Format& format() const &;

private:
  // Map |_size| bytes of sink |_sink|.
  Byte* map_sink_data(Sink _sink, Size _size);

  // Write |_size| bytes from |_data| into sink |_sink|.
  bool write_sink_data(Sink _sink, const Byte* _data, Size _size);

  // Record edit to sinkk |_sink| at offset |_offset| of size |_size|.
  bool record_sink_edit(Sink _sink, Size _offset, Size _size);

  friend struct Arena;

  enum {
    FORMAT = 1 << 1
  };

  LinearBuffer m_vertices_store;
  LinearBuffer m_elements_store;
  LinearBuffer m_instances_store;

  Format m_format;
  Vector<Edit> m_edits;
  Uint32 m_recorded;
};


// [Buffer::Attribute]
inline bool Buffer::Attribute::operator!=(const Attribute& _other) const {
  return _other.offset != offset || _other.type != type;
}

inline Size Buffer::Attribute::hash() const {
  return hash_combine(Hash<Size>{}(offset), Hash<Type>{}(type));
}

// [Buffer::Format]
inline Buffer::Format::Format()
  : m_flags{0}
  , m_element_type{ElementType::NONE}
  , m_vertex_stride{0}
  , m_instance_stride{0}
  , m_hash{0}
{
}

inline void Buffer::Format::record_type(Type _type) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  m_type = _type;
  m_flags |= TYPE;
}

inline void Buffer::Format::record_element_type(ElementType _type) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  m_element_type = _type;
  m_flags |= ELEMENT_TYPE;
}

inline void Buffer::Format::record_vertex_stride(Size _stride) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  m_vertex_stride = _stride;
  m_flags |= VERTEX_STRIDE;
}

inline void Buffer::Format::record_instance_stride(Size _stride) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  m_instance_stride = _stride;
  m_flags |= INSTANCE_STRIDE;
}

inline bool Buffer::Format::record_vertex_attribute(const Attribute& _attribute) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  return m_vertex_attributes.push_back(_attribute);
}

inline bool Buffer::Format::record_instance_attribute(const Attribute& _attribute) {
  RX_ASSERT(!(m_flags & FINALIZED), "finalized");
  return m_instance_attributes.push_back(_attribute);
}

inline bool Buffer::Format::is_indexed() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_element_type != ElementType::NONE;
}

inline bool Buffer::Format::is_instanced() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return !m_instance_attributes.is_empty();
}

inline Buffer::Type Buffer::Format::type() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_type;
}

inline Buffer::ElementType Buffer::Format::element_type() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_element_type;
}

inline const Vector<Buffer::Attribute>& Buffer::Format::vertex_attributes() const & {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_vertex_attributes;
}

inline const Vector<Buffer::Attribute>& Buffer::Format::instance_attributes() const & {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_instance_attributes;
}

inline Size Buffer::Format::vertex_stride() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_vertex_stride;
}

inline Size Buffer::Format::instance_stride() const {
  RX_ASSERT(m_flags & FINALIZED, "not finalized");
  return m_instance_stride;
}

inline Size Buffer::Format::element_size() const {
  switch (m_element_type) {
  case ElementType::NONE:
    return 0;
  case ElementType::U8:
    return 1;
  case ElementType::U16:
    return 2;
  case ElementType::U32:
    return 4;
  }
  RX_HINT_UNREACHABLE();
}

inline bool Buffer::Format::operator==(const Format& _format) const {
  return !operator!=(_format);
}

inline Size Buffer::Format::hash() const {
  return m_hash;
}

// [Buffer]
inline Byte* Buffer::map_vertices(Size _size) {
  return map_sink_data(Sink::VERTICES, _size);
}

inline Byte* Buffer::map_elements(Size _size) {
  return map_sink_data(Sink::ELEMENTS, _size);
}

inline Byte* Buffer::map_instances(Size _size) {
  return map_sink_data(Sink::INSTANCES, _size);
}

template<typename T>
bool Buffer::write_vertices(const T* _data, Size _size) {
  return write_sink_data(Sink::VERTICES, reinterpret_cast<const Byte*>(_data), _size);
}

template<typename T>
bool Buffer::write_elements(const T* _data, Size _size) {
  return write_sink_data(Sink::ELEMENTS, reinterpret_cast<const Byte*>(_data), _size);
}

template<typename T>
bool Buffer::write_instances(const T* _data, Size _size) {
  return write_sink_data(Sink::INSTANCES, reinterpret_cast<const Byte*>(_data), _size);
}

inline bool Buffer::record_vertices_edit(Size _offset, Size _size) {
  return record_sink_edit(Sink::VERTICES, _offset, _size);
}

inline bool Buffer::record_elements_edit(Size _offset, Size _size) {
  return record_sink_edit(Sink::ELEMENTS, _offset, _size);
}

inline bool Buffer::record_instances_edit(Size _offset, Size _size) {
  return record_sink_edit(Sink::INSTANCES, _offset, _size);
}

inline void Buffer::record_format(const Format& _format) {
  RX_ASSERT(!(m_recorded & FORMAT), "already recorded");
  m_format = _format;
  m_recorded |= FORMAT;
}

inline const LinearBuffer& Buffer::vertices() const & {
  return m_vertices_store;
}

inline const LinearBuffer& Buffer::elements() const & {
  return m_elements_store;
}

inline const LinearBuffer& Buffer::instances() const & {
  return m_instances_store;
}

inline Size Buffer::size() const {
  return m_vertices_store.size() + m_elements_store.size() + m_instances_store.size();
}

inline const Vector<Buffer::Edit>& Buffer::edits() const {
  return m_edits;
}

inline void Buffer::clear_edits() {
  m_edits.clear();
}

inline const Buffer::Format& Buffer::format() const & {
  return m_format;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_BUFFER_H
