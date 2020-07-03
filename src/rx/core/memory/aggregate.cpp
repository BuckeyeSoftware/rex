#include "rx/core/memory/aggregate.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Memory {

// The following calculation of padding for element offsets follows the same
// padding rules of structures in C/C++.
//
// The final alignment is based on which field has the largest alignment.
bool Aggregate::finalize() {
  if (m_bytes || m_size == 0) {
    return false;
  }

  auto align = [](Size _alignment, Size _offset) {
    return (_alignment - (_offset & (_alignment - 1))) & (_alignment - 1);
  };

  Size offset = 0;
  Size alignment = 0;
  for (Size i = 0; i < m_size; i++) {
    auto& entry = m_entries[i];
    const Size padding = align(entry.align, offset);
    const Size aligned = offset + padding;
    entry.offset = aligned;
    offset = aligned + entry.size;
    alignment = Algorithm::max(alignment, entry.align);
  }

  m_bytes = align(alignment, offset);

  return true;
}

bool Aggregate::add(Size _size, Size _alignment) {
  RX_ASSERT(_size && _alignment, "empty field");
  RX_ASSERT(!m_bytes, "already finalized");
  if (m_size < sizeof m_entries / sizeof *m_entries) {
    m_entries[m_size++] = {_size, _alignment, 0};
    return true;
  }
  return false;
}

} // namespace Rx::Memory
