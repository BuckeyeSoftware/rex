#include <string.h> // memcpy

#include <rx/render/buffer.h>
#include <rx/core/log.h>

RX_LOG("render/buffer", buffer_log);

namespace rx::render {

buffer::buffer()
  : resource{resource::category::k_buffer}
  , m_element_type{element_category::k_none}
  , m_stride{0}
{
  buffer_log(rx::log::level::k_verbose, "%p: init buffer", this);
}

buffer::~buffer() {
  buffer_log(rx::log::level::k_verbose, "%p: fini buffer", this);
}

void buffer::write_vertices_data(const rx_byte* _data, rx_size _size, rx_size _stride) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_size != 0, "_size is zero");
  RX_ASSERT(_size % _stride == 0, "_size not a multiple of _stride");

  if (m_stride != 0) {
    RX_ASSERT(m_stride == _stride, "vertex stride changed");
  } else {
    m_stride = _stride;
  }
  const auto old_size{m_vertices_store.size()};
  m_vertices_store.resize(old_size + _size);
  memcpy(m_vertices_store.data() + old_size, _data, _size);
}

void buffer::write_elements_data(element_category _type, const rx_byte* _data, rx_size _size) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_size != 0, "_size is zero");

  if (m_element_type != element_category::k_none) {
    RX_ASSERT(m_element_type == _type, "element type changed");
  } else {
    m_element_type = _type;
  }
  const auto old_size{m_elements_store.size()};
  m_elements_store.resize(old_size + _size);
  memcpy(m_elements_store.data() + old_size, _data, _size);
}

void buffer::flush() {
  m_vertices_store.clear();
  m_elements_store.clear();
}

} // namespace rx::render
