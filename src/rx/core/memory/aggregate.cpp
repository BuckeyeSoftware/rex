#include "rx/core/memory/aggregate.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Memory {

// The following calculation of padding for element offsets follows the same
// padding rules of structures in C/C++.
//
// The final alignment is based on which field has the largest alignment.
Byte* Aggregate::allocate() {
  if (m_entries.is_empty()) {
    return nullptr;
  }

  auto align = [](Size _alignment, Size _offset) {
    return (_offset + (_alignment - 1)) & ~(_alignment - 1);
  };

  Size offset = 0;
  Size alignment = 0;
  m_entries.each_fwd([&](Entry& entry_) {
    const Size aligned = align(entry_.align, offset);
    entry_.offset = aligned;
    offset = aligned + entry_.size;
    alignment = Algorithm::max(alignment, entry_.align);
  });

  return m_allocator.allocate(align(alignment, offset));
}

bool Aggregate::add(Size _size, Size _alignment, Size _count) {
  RX_ASSERT(_size && _alignment, "empty field");

  // Would |_size * _count| overflow?
  if (_size && _count > -1_z / _size) {
    return false;
  }

  return m_entries.emplace_back(_size * _count, _alignment, 0_z);
}

} // namespace Rx::Memory
