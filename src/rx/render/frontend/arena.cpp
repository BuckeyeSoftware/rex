#include <string.h> // memcpy

#include "rx/render/frontend/arena.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/buffer.h"

namespace Rx::Render::Frontend {

// [Arena::List]
bool Arena::List::allocate(Uint32 _size, Uint32& offset_) {
  // Search for a region that fits |_size|
  for (Size i = 0; i < m_size; i++) {
    auto& r = m_data[i];
    if (!r.free || r.size < _size) {
      continue;
    }

    const auto remain = r.size - _size;
    const auto offset = r.offset;

    r.free = 0;
    r.size = _size;

    if (remain) {
      Region split;
      split.size = remain;
      split.offset = offset + _size;
      split.free = 1;
      if (!insert_at(i + 1, split)) {
        r.size += remain;
        return false;
      }
    }

    return true;
  }

  Size offset = 0;
  if (m_size) {
    const auto& last = m_data[m_size - 1];
    offset = last.offset + last.size;
  }

  Region r;
  r.size = _size;
  r.free = 0;
  r.offset = offset;

  if (!push(r)) {
    return false;
  }

  offset_ = offset;

  return true;
}

bool Arena::List::reallocate(Uint32 _old_offset, Uint32 _size, Uint32& new_offset_) {
  const auto index = index_of(_old_offset);
  if (!index) {
    return false;
  }

  auto& r = m_data[index];
  if (r.size == _size) {
    // Size hasn't changed.
    new_offset_ = _old_offset;
    return true;
  }

  // Size has shrunk.
  if (r.size >= _size) {
    const auto remain = r.size - _size;
    const auto offset = r.offset;

    // Shrink the region.
    r.size = _size;

    // Create a new region representing the slop after this region.
    if (remain) {
      Region split;
      split.size = remain;
      split.offset = offset + _size;
      split.free = 1;
      if (!insert_at(index + 1, split)) {
        r.size += remain;
        return false;
      }
    }

    new_offset_ = offset;
    return true;
  }

  remove_at(index);

  return allocate(_size, new_offset_);
}

bool Arena::List::deallocate(Uint32 _offset) {
  if (auto index = index_of(_offset)) {
    remove_at(*index);
    return true;
  }
  return false;
}

Arena::List::~List() {
  destroy();
}

Arena::List& Arena::List::operator=(List&& list_) {
  destroy();
  m_context = Utility::exchange(list_.m_context, nullptr);
  m_data = Utility::exchange(list_.m_data, nullptr);
  m_size = Utility::exchange(list_.m_size, 0);
  m_capacity = Utility::exchange(list_.m_capacity, 0);
  return *this;
}

void Arena::List::destroy() {
  if (m_context) {
    m_context->allocator().deallocate(m_data);
  }
}

bool Arena::List::grow() {
  if (m_size + 1 >= m_capacity) {
    m_capacity = ((m_capacity + 1) * 3) / 2;
    if (auto data = m_context->allocator().reallocate(m_data, sizeof *m_data * m_capacity); !data) {
      // Out of memory on the |m_context| allocator.
      return false;
    } else {
      m_data = reinterpret_cast<Region*>(data);
    }
  }

  m_size++;

  return true;
}

bool Arena::List::push(const Region& _region) {
  return grow() && memcpy(m_data + m_size - 1, &_region, sizeof _region);
}

void Arena::List::remove_at(Size _index) {
  m_data[_index].free = 1;

  while (coalesce_backward(_index)) {
    _index--;
  }

  while (coalesce_forward(_index)) {
    _index++;
  }
}

bool Arena::List::insert_at(Size _index, const Region& _region) {
  return shift_up_at(_index) && memcpy(m_data + _index, &_region, sizeof _region);
}

Optional<Size> Arena::List::index_of(Uint32 _offset) const {
  Size l = 0;
  Size r = m_size - 1;

  while (l <= r) {
    const Size m = l + (r - l) / 2;
    const Uint32 offset = m_data[m].offset;
    if (offset == _offset) {
      return m;
    }

    if (offset < _offset) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  return nullopt;
}

void Arena::List::shift_down_at(Size _index) {
  for (Size i = _index; i < m_size - 1; i++) {
    m_data[i] = m_data[i + 1];
  }
  m_size--;
}

bool Arena::List::shift_up_at(Size _index) {
  if (!grow()) {
    return false;
  }

  for (Size i = m_size - 1; i > _index; i--) {
    m_data[i + 1] = m_data[i];
  }

  return true;
}

bool Arena::List::is_free(Size _index) const {
  return m_data[_index].free != 0;
}

bool Arena::List::coalesce_forward(Size _index) {
  if (_index + 1 >= m_size || !is_free(_index) || !is_free(_index + 1)) {
    return false;
  }

  auto& prev = m_data[_index];
  auto& next = m_data[_index + 1];

  next.size += prev.size;
  next.offset = prev.offset;

  shift_down_at(_index);

  return true;
}

bool Arena::List::coalesce_backward(Size _index) {
  if (_index == 0 || !is_free(_index) || !is_free(_index - 1)) {
    return false;
  }

  m_data[_index - 1].size += m_data[_index].size;

  shift_down_at(_index);

  return true;
}

// [Arena::Block]
Arena::Block::~Block() {
  destroy();
}

Arena::Block& Arena::Block::operator=(Block&& block_) {
  destroy();
  m_arena = Utility::exchange(block_.m_arena, nullptr);
  m_ranges[0] = Utility::move(block_.m_ranges[0]);
  m_ranges[1] = Utility::move(block_.m_ranges[1]);
  m_ranges[2] = Utility::move(block_.m_ranges[2]);
  return *this;
}

void Arena::Block::destroy() {
  if (!m_arena) {
    return;
  }

  // Release memory owned by this block.
  for (Size i = 0; i < 3; i++) {
    if (Uint32 offset = m_ranges[i].offset; offset != -1_u32) {
      RX_ASSERT(m_arena->m_lists[i].deallocate(offset), "consistency error");
    }
  }
}

bool Arena::Block::write_sink_data(Buffer::Sink _sink, const Byte* _data, Size _size) {
  if (auto data = map_sink_data(_sink, _size)) {
    memcpy(data, _data, _size);
    return true;
  }
  return false;
}

Byte* Arena::Block::map_sink_data(Buffer::Sink _sink, Uint32 _size) {
  Buffer* buffer = m_arena->m_buffer;

  List& list = m_arena->m_lists[static_cast<Size>(_sink)];
  Range& range = range_for(_sink);

  Vector<Byte>* store = nullptr;
  switch (_sink) {
  case Buffer::Sink::VERTICES:
    store = &buffer->m_vertices_store;
    break;
  case Buffer::Sink::ELEMENTS:
    store = &buffer->m_elements_store;
    break;
  case Buffer::Sink::INSTANCES:
    store = &buffer->m_instances_store;
    break;
  }

  Uint32 new_offset = 0;
  Uint32 new_size = _size;

  // Already allocated, try to resize the range.
  if (range.offset != -1_u32) {
    const auto old_offset = range.offset;
    const auto old_size = range.size;
    if (list.reallocate(old_offset, new_size, new_offset)) {
      if (new_offset != old_offset) {
        const Size total_size = new_offset + new_size;
        if (total_size >= store->size()) {
          // Ran out of memory in |store|.
          if (!store->resize(total_size, Utility::UninitializedTag{})) {
            // Undo the reallocation on |list|.
            //
            // This should never fail. Assert anyways because if it does fail it
            // means the |list| data structure is in an inconsistent state.
            RX_ASSERT(list.reallocate(new_offset, old_size, new_offset), "consistency error");
            return nullptr;
          }
        }

        // Move the data since the reallocation moved it.
        memmove(store->data() + new_offset, store->data() + old_offset, old_size);

        // When the contents of the data are moved, we need to record an edit
        // on the range of data in the |buffer| we replaced.
        buffer->record_sink_edit(_sink, new_offset, old_size);
      }
    } else {
      // Ran out of memory in |list| data structure.
      return nullptr;
    }
  }

  // This is a fresh allocation.
  if (list.allocate(_size, new_offset)) {
    const auto total_size = new_offset + new_size;
    if (total_size >= store->size()) {
      // Ran out of memory in |store|?
      if (!store->resize(total_size, Utility::UninitializedTag{})) {
        // Undo the allocation on |list|.
        //
        // This should never fail. Assert anyways because if it does fail it
        // means the |list| data structure is in an inconsistent state.
        RX_ASSERT(list.deallocate(new_offset), "consistency error");
        return nullptr;
      }
    }
  } else {
    // Ran out of memory in |list| data structure.
    return nullptr;
  }

  // Update the ranges with the new metadata.
  range.offset = new_offset;
  range.size = new_size;

  return store->data() + new_offset;
}

// [Arena]
Arena::Arena(Context* _context, const Buffer::Format& _format)
  : m_context{_context}
  , m_buffer{nullptr}
  , m_lists{List{_context}, List{_context}, List{_context}}
{
  // Create an empty buffer with the given |_format|.
  m_buffer = m_context->create_buffer(RX_RENDER_TAG("Arena"));
  m_buffer->record_format(_format);
  m_context->initialize_buffer(RX_RENDER_TAG("Arena"), m_buffer);
}

Arena::Arena(Arena&& arena_)
  : m_context{Utility::exchange(arena_.m_context, nullptr)}
  , m_buffer{Utility::exchange(arena_.m_buffer, nullptr)}
  , m_lists{Utility::move(arena_.m_lists)}
{
}

Arena::~Arena() {
  destroy();
}

void Arena::destroy() {
  if (m_context) {
    m_context->destroy_buffer(RX_RENDER_TAG("Arena"), m_buffer);
  }
}

Arena& Arena::operator=(Arena&& arena_) {
  destroy();
  m_context = Utility::exchange(arena_.m_context, nullptr);
  m_buffer = Utility::exchange(arena_.m_buffer, nullptr);
  m_lists = Utility::move(arena_.m_lists);
  return *this;
}

} // namespace Rx::render::frontend
