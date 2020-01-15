#include <string.h> // memcpy

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/interface.h"

namespace rx::render::frontend {

static rx_size size_for_element_type(buffer::element_type _type) {
  switch (_type) {
  case buffer::element_type::k_none:
    return 0;
  case buffer::element_type::k_u8:
    return 1;
  case buffer::element_type::k_u16:
    return 2;
  case buffer::element_type::k_u32:
    return 4;
  }
  return 0;
}

buffer::buffer(interface* _frontend)
  : resource{_frontend, resource::type::k_buffer}
  , m_vertices_store{m_frontend->allocator()}
  , m_elements_store{m_frontend->allocator()}
  , m_attributes{m_frontend->allocator()}
  , m_edits{m_frontend->allocator()}
  , m_recorded{0}
{
}

buffer::~buffer() {
}

void buffer::write_vertices_data(const rx_byte* _data, rx_size _size) {
  RX_ASSERT(_data, "null data");
  memcpy(map_vertices(_size), _data, _size);
}

void buffer::write_elements_data(const rx_byte* _data, rx_size _size) {
  RX_ASSERT(_data, "null data");
  memcpy(map_elements(_size), _data, _size);
}

rx_byte* buffer::map_vertices(rx_size _size) {
  RX_ASSERT(m_recorded & k_stride, "vertex stride not recorded");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % m_stride == 0, "_size not a multiple of vertex stride");

  m_vertices_store.resize(_size);
  update_resource_usage(size());

  return m_vertices_store.data();
}

rx_byte* buffer::map_elements(rx_size _size) {
  RX_ASSERT(m_recorded & k_element_type, "element type not recorded");
  RX_ASSERT(m_element_type != element_type::k_none, "element type is k_none");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % size_for_element_type(m_element_type) == 0, "_size is not a multiple of element size");

  m_elements_store.resize(_size);
  update_resource_usage(size());

  return m_elements_store.data();
}

void buffer::validate() const {
  RX_ASSERT(m_recorded & k_attribute, "no attributes specified");
  RX_ASSERT(m_recorded & k_element_type, "no element type specified");
  RX_ASSERT(m_recorded & k_stride, "no stride specified");
  RX_ASSERT(m_recorded & k_type, "no type specified");
}

vector<buffer::edit>&& buffer::edits() {
  // Determines if |_lhs| is fully inside |_rhs|.
  auto inside = [](const edit& _lhs, const edit& _rhs) {
    return !(_lhs.offset < _rhs.offset || _lhs.offset + _lhs.size > _rhs.offset + _rhs.size);
  };

  // When an edit is inside a larger edit, that larger edit will include the
  // nested edit. Remove such edits (duplicate and fully overlapping) to
  // produce a minimal and coalesced edit list for the backend.

  // WARN(dweiler): This behaves O(n^2), except n should be small.
  for (rx_size i{0}; i < m_edits.size(); i++) {
    for (rx_size j{0}; j < m_edits.size(); j++) {
      if (i == j) {
        continue;
      }

      // The edits need to be to the same sink.
      const auto& e1{m_edits[i]};
      const auto& e2{m_edits[j]};
      if (e1.sink != e2.sink) {
        continue;
      }

      // Edit exists fully inside another, remove it.
      if (inside(e1, e2)) {
        m_edits.erase(i, i + 1);
      }
    }
  }

  return utility::move(m_edits);
}

} // namespace rx::render::frontend
