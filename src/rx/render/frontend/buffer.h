#ifndef RX_RENDER_FRONTEND_BUFFER_H
#define RX_RENDER_FRONTEND_BUFFER_H
#include "rx/core/linear_buffer.h"
#include "rx/core/vector.h"
#include "rx/core/tagged_ptr.h"

#include "rx/core/hash/combine.h"
#include "rx/core/hash/mix_enum.h"

#include "rx/render/frontend/resource.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Render::Frontend {

struct Context;

struct Buffer
  : Resource
{
  RX_MARK_NO_COPY(Buffer);
  RX_MARK_NO_MOVE(Buffer);

  struct Attribute {
    enum class Type : Uint8 {
      // Scalars.
      F32,
      S8,
      S16,
      S32,
      U8,
      U16,
      U32,

      // Vectors.
      F32x2,
      F32x3,
      F32x4,

      S8x2,
      S8x3,
      S8x4,

      S16x2,
      S16x3,
      S16x4,

      S32x2,
      S32x3,
      S32x4,

      U8x2,
      U8x3,
      U8x4,

      U16x2,
      U16x3,
      U16x4,

      U32x2,
      U32x3,
      U32x4,

      // Matrices
      F32x4x4
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
    Format(Memory::Allocator& _allocator);

    static Optional<Format> copy(const Format& _other);

    void record_element_type(ElementType _type);

    void record_vertex_stride(Size _stride);
    void record_instance_stride(Size _stride);

    bool record_vertex_attribute(const Attribute& _attribute);
    bool record_instance_attribute(const Attribute& _attribute);

    bool is_indexed() const;
    bool is_instanced() const;

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

    Memory::Allocator& allocator() const;

  private:
    enum {
      ELEMENT_TYPE    = 1 << 0,
      VERTEX_STRIDE   = 1 << 1,
      INSTANCE_STRIDE = 1 << 2,
      FINALIZED       = 1 << 3
    };

    Uint32 m_flags;
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

  // Ensure the Edit type maintains multiple of Size.
  static_assert(sizeof(Edit) == sizeof(Size[3]),
    "invalid size for command stream");

  Buffer(Context* _frontend);
  ~Buffer();

  // Record buffer format.
  bool record_format(const Format& _format);
  // Record buffer type.
  void record_type(Type _type);

  // Map |_size| bytes of sink |_sink|.
  [[nodiscard]] Byte* map_sink(Sink _sink, Size _size);

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
  void clear_edits();

  void validate() const;

  const Format& format() const &;
  Type type() const;

private:
  friend struct Context;

  // Write |_size| bytes from |_data| into sink |_sink|.
  [[nodiscard]] bool write_sink_data(Sink _sink, const Byte* _data, Size _size);

  // Record edit to sinkk |_sink| at offset |_offset| of size |_size|.
  [[nodiscard]] bool record_sink_edit(Sink _sink, Size _offset, Size _size);

  friend struct Arena;

  enum {
    FORMAT = 1 << 0,
    TYPE   = 1 << 1
  };

  LinearBuffer m_vertices_store;
  LinearBuffer m_elements_store;
  LinearBuffer m_instances_store;

  Format m_format;
  Type m_type;
  Vector<Edit> m_edits;
  Uint32 m_recorded;
};

// Memory allocator over a Buffer to avoid copies. |allocate| may only be called
// once, subsequent allocations should only reallocate the original allocation.
// This allocator is really only useful for types like Vector<T>, LinearBuffer,
// and anything that manages a single contiguous allocation which it reallocates.
struct BufferAllocator
  : Memory::Allocator
{
  BufferAllocator() = default;
  BufferAllocator(Buffer::Sink _sink, Buffer* _buffer);

  BufferAllocator& operator=(const BufferAllocator& _allocator);

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void*, Size _size);
  virtual void deallocate(void*);

private:
  Buffer::Sink m_sink;

  // When tagged an allocation is managed by the allocator.
  TaggedPtr<Buffer> m_buffer;
};

// [BufferAllocator]
inline BufferAllocator::BufferAllocator(Buffer::Sink _sink, Buffer* _buffer)
  : m_sink{_sink}
  , m_buffer{_buffer, 0}
{
}

inline BufferAllocator& BufferAllocator::operator=(const BufferAllocator& _allocator) {
  m_sink = _allocator.m_sink;
  m_buffer = _allocator.m_buffer;
  return *this;
}

// [Buffer::Attribute]
inline bool Buffer::Attribute::operator!=(const Attribute& _other) const {
  return _other.offset != offset || _other.type != type;
}

inline Size Buffer::Attribute::hash() const {
  return Hash::combine(Hash::mix_int(offset), Hash::mix_enum(type));
}

// [Buffer::Format]
inline Buffer::Format::Format(Memory::Allocator& _allocator)
  : m_flags{0}
  , m_element_type{ElementType::NONE}
  , m_vertex_stride{0}
  , m_instance_stride{0}
  , m_vertex_attributes{_allocator}
  , m_instance_attributes{_allocator}
  , m_hash{0}
{
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

inline Memory::Allocator& Buffer::Format::allocator() const {
  // TODO(dweiler): Put m_allocator in Format.
  return m_vertex_attributes.allocator();
}

// [Buffer]
inline Byte* Buffer::map_vertices(Size _size) {
  return map_sink(Sink::VERTICES, _size);
}

inline Byte* Buffer::map_elements(Size _size) {
  return map_sink(Sink::ELEMENTS, _size);
}

inline Byte* Buffer::map_instances(Size _size) {
  return map_sink(Sink::INSTANCES, _size);
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
  RX_ASSERT(m_type != Type::STATIC, "cannot edit static buffer");
  return record_sink_edit(Sink::VERTICES, _offset, _size);
}

inline bool Buffer::record_elements_edit(Size _offset, Size _size) {
  RX_ASSERT(m_type != Type::STATIC, "cannot edit static buffer");
  return record_sink_edit(Sink::ELEMENTS, _offset, _size);
}

inline bool Buffer::record_instances_edit(Size _offset, Size _size) {
  RX_ASSERT(m_type != Type::STATIC, "cannot edit static buffer");
  return record_sink_edit(Sink::INSTANCES, _offset, _size);
}

inline bool Buffer::record_format(const Format& _format) {
  RX_ASSERT(!(m_recorded & FORMAT), "already recorded");
  if (auto format = Utility::copy(_format)) {
    m_format = Utility::move(*format);
    m_recorded |= FORMAT;
    return true;
  }
  return false;
}

inline void Buffer::record_type(Type _type) {
  RX_ASSERT(!(m_recorded & TYPE), "already recorded");
  m_type = _type;
  m_recorded |= TYPE;
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

inline Buffer::Type Buffer::type() const {
  return m_type;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_BUFFER_H
