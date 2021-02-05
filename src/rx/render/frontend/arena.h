#ifndef RX_RENDER_FRONTEND_ARENA_H
#define RX_RENDER_FRONTEND_ARENA_H
#include "rx/render/frontend/buffer.h"
#include "rx/core/array.h"

namespace Rx::Render::Frontend {

struct Context;

// # Arena
//
// The purpose of arena is to maintain batching of geometry under a single
// buffer for geometry that shares the same format.
//
// This is an optimization strategy that all backends can benefit from without
// needing to implement it themselves, as a buffer is meant to represent a
// single resource.
struct Arena {
  RX_MARK_NO_COPY(Arena);

  struct Block {
    RX_MARK_NO_COPY(Block);

    constexpr Block();
    constexpr Block(Arena* _arena);
    Block(Block&& block_);
    ~Block();
    Block& operator=(Block&& block_);

    Byte* map_vertices(Size _size);
    Byte* map_elements(Size _size);
    Byte* map_instances(Size _size);

    template<typename T>
    bool write_vertices(const T* _data, Size _size);
    template<typename T>
    bool write_elements(const T* _data, Size _size);
    template<typename T>
    bool write_instances(const T* _data, Size _size);

    bool record_vertices_edit(Size _offset, Size _size);
    bool record_elements_edit(Size _offset, Size _size);
    bool record_instances_edit(Size _offset, Size _size);

    Size base_vertex() const;
    Size base_element() const;
    Size base_instance() const;

  private:
    void destroy();

    Byte* map_sink_data(Buffer::Sink _sink, Uint32 _size);
    bool write_sink_data(Buffer::Sink _sink, const Byte* _data, Size _size);
    bool record_sink_edit(Buffer::Sink _sink, Size _offset, Size _size);

    // The arena which owns this |Block|.
    Arena* m_arena;

    // The ranges in the arena for each |Sink|.
    struct Range {
      Uint32 offset = -1_u32;
      Uint32 size   = -1_u32;
    };
    Array<Range[3]> m_ranges;

    // Helpers to form a reference to a range given a |Sink| enum value.
    Range& range_for(Buffer::Sink _sink) &;
    const Range& range_for(Buffer::Sink _sink) const &;
  };

  Arena(Context* _frontend, const Buffer::Format& _format);
  Arena(Arena&& arena_);
  ~Arena();
  Arena& operator=(Arena&& arena_);

  Buffer* buffer() const;

private:
  // Region management.
  struct List {
    RX_MARK_NO_COPY(List);

    constexpr List(Context* _frontend);
    List(List&& list_);
    ~List();

    List& operator=(List&& list_);

    struct Region {
      Uint32 offset;
      Uint32 size : 31;
      Uint32 free : 1;
    };

    bool allocate(Uint32 _size, Uint32& offset_);
    bool reallocate(Uint32 _old_offset, Uint32 _size, Uint32& new_offset_);
    bool deallocate(Uint32 _offset);

    // Get the region for a given |_index| or |_offset|.
    const Region& region_by_index(Size _index) const &;
    const Region& region_by_offset(Uint32 _offset) const &;

  private:
    void destroy();

    // Find the index in |m_data| for a given |Region| based on it's |_offset|.
    // This performs a binary search and has O(log n) performance.
    Optional<Size> index_of(Uint32 _offset) const;

    bool grow();
    bool push(const Region& _region);

    void remove_at(Size _index);
    bool insert_at(Size _index, const Region& _region);

    void shift_down_at(Size _index);
    bool shift_up_at(Size _index);

    bool is_free(Size _index) const;

    bool coalesce_forward(Size _index);
    bool coalesce_backward(Size _index);

    Context* m_context;
    Region* m_data;
    Size m_size;
    Size m_capacity;
  };

  void destroy();

  Context* m_context;
  Buffer* m_buffer;
  Array<List[3]> m_lists;
};

// [Arena::List]
inline constexpr Arena::List::List(Context* _context)
  : m_context{_context}
  , m_data{nullptr}
  , m_size{0}
  , m_capacity{0}
{
}

inline Arena::List::List(List&& list_)
  : m_context{Utility::exchange(list_.m_context, nullptr)}
  , m_data{Utility::exchange(list_.m_data, nullptr)}
  , m_size{Utility::exchange(list_.m_size, 0)}
  , m_capacity{Utility::exchange(list_.m_capacity, 0)}
{
}

RX_HINT_FORCE_INLINE const Arena::List::Region& Arena::List::region_by_index(Size _index) const & {
  RX_ASSERT(_index < m_size, "out of bounds");
  return m_data[_index];
}

inline const Arena::List::Region& Arena::List::region_by_offset(Uint32 _offset) const & {
  const auto index = index_of(_offset);
  RX_ASSERT(index, "invalid offset");
  return region_by_index(*index);
}

// [Arena::Block]
inline constexpr Arena::Block::Block()
  : Block{nullptr}
{
}

inline constexpr Arena::Block::Block(Arena* _arena)
  : m_arena{_arena}
{
}

inline Arena::Block::Block(Block&& block_)
  : m_arena{Utility::exchange(block_.m_arena, nullptr)}
  , m_ranges{Utility::move(block_.m_ranges)}
{
}

RX_HINT_FORCE_INLINE Byte* Arena::Block::map_vertices(Size _size) {
  return map_sink_data(Buffer::Sink::VERTICES, _size);
}

RX_HINT_FORCE_INLINE Byte* Arena::Block::map_elements(Size _size) {
  return map_sink_data(Buffer::Sink::ELEMENTS, _size);
}

RX_HINT_FORCE_INLINE Byte* Arena::Block::map_instances(Size _size) {
  return map_sink_data(Buffer::Sink::INSTANCES, _size);
}

template<typename T>
bool Arena::Block::write_vertices(const T* _data, Size _size) {
  return write_sink_data(Buffer::Sink::VERTICES, reinterpret_cast<const Byte*>(_data), _size);
}

template<typename T>
bool Arena::Block::write_elements(const T* _data, Size _size) {
  return write_sink_data(Buffer::Sink::ELEMENTS, reinterpret_cast<const Byte*>(_data), _size);
}

template<typename T>
bool Arena::Block::write_instances(const T* _data, Size _size) {
  return write_sink_data(Buffer::Sink::INSTANCES, reinterpret_cast<const Byte*>(_data), _size);
}

inline bool Arena::Block::record_vertices_edit(Size _offset, Size _size) {
  return record_sink_edit(Buffer::Sink::VERTICES, _offset, _size);
}

inline bool Arena::Block::record_elements_edit(Size _offset, Size _size) {
  return record_sink_edit(Buffer::Sink::ELEMENTS, _offset, _size);
}

inline bool Arena::Block::record_instances_edit(Size _offset, Size _size) {
  return record_sink_edit(Buffer::Sink::INSTANCES, _offset, _size);
}

inline bool Arena::Block::record_sink_edit(Buffer::Sink _sink, Size _offset, Size _size) {
  // Ensure the recorded edit is inside the block allocation.
  const auto& range = range_for(_sink);
  RX_ASSERT(_offset + _size <= range.offset + range.size, "out of bounds edit in block");
  return m_arena->m_buffer->record_sink_edit(_sink, range.offset + _offset, _size);
}

RX_HINT_FORCE_INLINE Size Arena::Block::base_vertex() const {
  const auto& format = m_arena->m_buffer->format();
  return range_for(Buffer::Sink::VERTICES).offset / format.vertex_stride();
}

RX_HINT_FORCE_INLINE Size Arena::Block::base_element() const {
  const auto& format = m_arena->m_buffer->format();
  const auto& range = range_for(Buffer::Sink::ELEMENTS);
  const auto element_size = format.element_size();
  return range.offset != -1_u32 && element_size ? range.offset / element_size : 0;
}

RX_HINT_FORCE_INLINE Size Arena::Block::base_instance() const {
  const auto& format = m_arena->m_buffer->format();
  const auto& range = range_for(Buffer::Sink::INSTANCES);
  return range.offset != -1_u32 ? range.offset / format.instance_stride() : 0;
}

RX_HINT_FORCE_INLINE Arena::Block::Range& Arena::Block::range_for(Buffer::Sink _sink) & {
  return m_ranges[static_cast<Size>(_sink)];
}

RX_HINT_FORCE_INLINE const Arena::Block::Range& Arena::Block::range_for(Buffer::Sink _sink) const & {
  return m_ranges[static_cast<Size>(_sink)];
}

// [Arena]
RX_HINT_FORCE_INLINE Buffer* Arena::buffer() const {
  return m_buffer;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_ARENA_H
