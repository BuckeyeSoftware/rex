#include <string.h> // memcpy

#include <rx/render/buffer.h>
#include <rx/render/frontend.h>
#include <rx/core/log.h>

namespace rx::render {

static rx_size size_for_element_type(buffer::element_category _type) {
  switch (_type) {
  case buffer::element_category::k_none:
    return 0;
  case buffer::element_category::k_u8:
    return 1;
  case buffer::element_category::k_u16:
    return 2;
  case buffer::element_category::k_u32:
    return 4;
  }
  return 0;
}

buffer::buffer(frontend* _frontend)
  : resource{_frontend, resource::category::k_buffer}
  , m_vertices_store{m_frontend->allocator()}
  , m_elements_store{m_frontend->allocator()}
  , m_attributes{m_frontend->allocator()}
  , m_recorded{0}
{
}

buffer::~buffer() {
}

void buffer::write_vertices_data(const rx_byte* _data, rx_size _size) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % m_stride == 0, "_size not a multiple of vertex stride");

  const auto old_size{m_vertices_store.size()};
  m_vertices_store.resize(old_size + _size);
  memcpy(m_vertices_store.data() + old_size, _data, _size);
}

void buffer::write_elements_data(const rx_byte* _data, rx_size _size) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % size_for_element_type(m_element_type) == 0, "_size is not a multiple of element size");

  const auto old_size{m_elements_store.size()};
  m_elements_store.resize(old_size + _size);
  memcpy(m_elements_store.data() + old_size, _data, _size);
}

void buffer::validate() const {
  RX_ASSERT(m_recorded & k_attribute, "no attributes specified");
  RX_ASSERT(m_recorded & k_element_type, "no element type specified");
  RX_ASSERT(m_recorded & k_stride, "no stride specified");
  RX_ASSERT(m_recorded & k_type, "no type specified");
}

void buffer::flush() {
  m_vertices_store.clear();
  m_elements_store.clear();
}

} // namespace rx::render