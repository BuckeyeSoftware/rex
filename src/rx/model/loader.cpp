#include <rx/model/loader.h>
#include <rx/core/filesystem/file.h>
#include <rx/core/log.h>
#include <rx/core/algorithm/max.h>

RX_LOG("model/loader", log_model);

namespace rx::model {

loader::loader(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_elements{_allocator}
  , m_positions{_allocator}
  , m_coordinates{_allocator}
  , m_normals{_allocator}
  , m_tangents{_allocator}
  , m_blend_indices{_allocator}
  , m_blend_weights{_allocator}
{
}

bool loader::load(const string& _file_name) {
  const auto data = filesystem::read_binary_file(_file_name);
  if (!data) {
    return false;
  }

  if (!read(*data)) {
    log_model(log::level::k_error, "failed to decode '%s' [%s]", _file_name, m_error);
    return false;
  }

  if (m_elements.is_empty() || m_positions.is_empty()) {
    log_model(log::level::k_error, "'%s' lacks vertices", _file_name);
    return false;
  }

  // ensure none of our elements go out of bounds
  const rx_size vertices{m_positions.size()};
  rx_u32 max_element{0};
  m_elements.each_fwd([&max_element](rx_u32 _element) {
    max_element = algorithm::max(max_element, _element);
  });

  if (max_element >= vertices) {
    log_model(log::level::k_error, "'%s' element out of bounds", _file_name);
    return false;
  }

  if (m_normals.is_empty()) {
    log_model(log::level::k_warning, "'%s' lacks normals", _file_name);
    generate_normals();
  }

  if (m_tangents.is_empty()) {
    // not possible to generate tangents without texture coordinates
    if (m_coordinates.is_empty()) {
      log_model(log::level::k_error, "'%s' lacks tangents and texture coordinates bailing", _file_name);
      return false;
    } else {
      log_model(log::level::k_warning, "'%s' lacks tangents", _file_name);
      generate_tangents();
    }
  }

  if (m_normals.size() != vertices) {
    log_model(log::level::k_warning, "'%s' has too %s normals",
      m_normals.size() > vertices ? "many" : "few");
    m_normals.resize(vertices);
  }

  if (m_tangents.size() != vertices) {
    log_model(log::level::k_warning, "'%s' has too %s tangents",
      m_tangents.size() > vertices ? "many" : "few");
    m_tangents.resize(vertices);
  }

  if (!m_coordinates.is_empty() && m_coordinates.size() != vertices) {
    log_model(log::level::k_warning, "'%s' has too %s coordinates",
      m_coordinates.size() > vertices ? "many" : "few");
    m_coordinates.resize(vertices);
  }

  return true;
}

void loader::generate_normals() {
  const rx_size vertices{m_positions.size()};
  const rx_size elements{m_elements.size()};

  m_normals.resize(vertices);

  for (rx_size i{0}; i < elements; i += 3) {
    const rx_u32 index0{m_elements[i+0]};
    const rx_u32 index1{m_elements[i+1]};
    const rx_u32 index2{m_elements[i+2]};
  
    const math::vec3f p1p0{m_positions[index1] - m_positions[index0]};
    const math::vec3f p2p0{m_positions[index2] - m_positions[index0]};

    const math::vec3f normal{math::normalize(math::cross(p1p0, p2p0))};

    m_normals[index0] += normal;
    m_normals[index1] += normal;
    m_normals[index2] += normal;
  }

  for (rx_size i{0}; i < vertices; i++) {
    m_normals[i] = math::normalize(m_normals[i]);
  }
}

void loader::generate_tangents() {
  // TODO
}

} // namespace rx::model
