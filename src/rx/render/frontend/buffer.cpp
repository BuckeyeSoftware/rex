#include <string.h> // memcpy

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render::Frontend {

Buffer::Buffer(Context* _frontend)
  : Resource{_frontend, Resource::Type::k_buffer}
  , m_vertices_store{m_frontend->allocator()}
  , m_elements_store{m_frontend->allocator()}
  , m_instances_store{m_frontend->allocator()}
  , m_vertex_attributes{m_frontend->allocator()}
  , m_instance_attributes{m_frontend->allocator()}
  , m_edits{m_frontend->allocator()}
  , m_recorded{0}
{
}

Buffer::~Buffer() {
}

void Buffer::write_vertices_data(const Byte* _data, Size _size) {
  RX_ASSERT(_data, "null data");
  memcpy(map_vertices(_size), _data, _size);
}

void Buffer::write_elements_data(const Byte* _data, Size _size) {
  RX_ASSERT(_data, "null data");
  memcpy(map_elements(_size), _data, _size);
}

void Buffer::write_instances_data(const Byte* _data, Size _size) {
  RX_ASSERT(_data, "null data");
  memcpy(map_instances(_size), _data, _size);
}

Byte* Buffer::map_vertices(Size _size) {
  RX_ASSERT(m_recorded & k_vertex_stride, "vertex stride not recorded");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % m_vertex_stride == 0, "_size not a multiple of vertex stride");

  m_vertices_store.resize(_size, Utility::UninitializedTag{});
  update_resource_usage(size());

  return m_vertices_store.data();
}

Byte* Buffer::map_elements(Size _size) {
  RX_ASSERT(m_recorded & k_element_type, "element type not recorded");
  RX_ASSERT(m_element_type != ElementType::k_none, "element Type is k_none");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % element_size() == 0, "_size is not a multiple of element size");

  m_elements_store.resize(_size, Utility::UninitializedTag{});
  update_resource_usage(size());

  return m_elements_store.data();
}

Byte* Buffer::map_instances(Size _size) {
  RX_ASSERT(m_recorded & k_instanced, "instanced not recorded");
  RX_ASSERT(m_recorded & k_instance_stride, "instance stride not recorded");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % m_instance_stride == 0, "_size not a multiple of instance stride");

  m_instances_store.resize(_size, Utility::UninitializedTag{});
  update_resource_usage(size());

  return m_instances_store.data();
}

void Buffer::validate() const {
  RX_ASSERT(m_recorded & k_vertex_attribute, "no vertex attributes specified");
  RX_ASSERT(m_recorded & k_vertex_stride, "no vertex stride specified");
  RX_ASSERT(m_recorded & k_element_type, "no element type specified");
  RX_ASSERT(m_recorded & k_instanced, "no instancing specified");
  RX_ASSERT(m_recorded & k_type, "no type specified");

  if (m_instanced) {
    RX_ASSERT(m_recorded & k_instance_stride, "no instance stride specified");
  }
}

Size Buffer::bytes_for_edits() const {
  Size bytes = 0;
  m_edits.each_fwd([&](const Edit& _edit) { bytes += _edit.size; });
  return bytes;
}

void Buffer::optimize_edits() {
  // Determines if |_lhs| is fully inside |_rhs|.
  auto inside = [](const Edit& _lhs, const Edit& _rhs) {
    return !(_lhs.offset < _rhs.offset || _lhs.offset + _lhs.size > _rhs.offset + _rhs.size);
  };

  // When an edit is inside a larger edit, that larger edit will include the
  // nested edit. Remove such edits (duplicate and fully overlapping) to
  // produce a minimal and coalesced edit list for the backend.

  // WARN(dweiler): This behaves O(n^2), except n should be small.
  for (Size i = 0; i < m_edits.size(); i++) {
    for (Size j = 0; j < m_edits.size(); j++) {
      if (i == j) {
        continue;
      }

      // The edits need to be to the same sink.
      const auto& e1 = m_edits[i];
      const auto& e2 = m_edits[j];
      if (e1.sink != e2.sink) {
        continue;
      }

      // Edit exists fully inside another, remove it.
      if (inside(e1, e2)) {
        m_edits.erase(i, i + 1);
      }
    }
  }
}

} // namespace rx::render::frontend
