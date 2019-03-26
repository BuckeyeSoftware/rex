#include <rx/model/model.h>
#include <rx/model/iqm.h>

namespace rx::model {

model::~model() {
  if (m_is_animated) {
    utility::destruct<array<animated_vertex>>(&as_animated_vertices);
  } else {
    utility::destruct<array<vertex>>(&as_vertices);
  }
}

bool model::load(const string& _file_name) {
  loader* new_loader{nullptr};

  // determine the model format based on the extension
  if (_file_name.ends_with(".iqm")) {
    auto iqm_loader{reinterpret_cast<iqm*>(m_allocator->allocate(sizeof(iqm)))};
    utility::construct<iqm>(iqm_loader, m_allocator);
    new_loader = iqm_loader;
  } else {
    // future model formats go here
    return false;
  }

  const bool result{new_loader->load(_file_name)};
  if (result) {
    const auto& animations{new_loader->animations()};
    const auto& positions{new_loader->positions()};
    const auto& normals{new_loader->normals()};
    const auto& tangents{new_loader->tangents()};
    const auto& coordinates{new_loader->coordinates()};
    const auto vertices{static_cast<rx_size>(positions.size())};

    if (animations.is_empty()) {
      utility::construct<array<vertex>>(&as_vertices, m_allocator, vertices);
      for (rx_size i{0}; i < vertices; i++) {
        as_vertices[i].position = positions[i];
        as_vertices[i].normal = normals[i];
        as_vertices[i].tangent = tangents[i];
        as_vertices[i].coordinate = coordinates[i];
      }
      m_is_animated = false;
    } else {
      const auto& blend_weights{new_loader->blend_weights()};
      const auto& blend_indices{new_loader->blend_indices()};
      utility::construct<array<animated_vertex>>(&as_animated_vertices, m_allocator, vertices);
      for (rx_size i{0}; i < vertices; i++) {
        as_animated_vertices[i].position = positions[i];
        as_animated_vertices[i].normal = normals[i];
        as_animated_vertices[i].tangent = tangents[i];
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weight = blend_weights[i];
        as_animated_vertices[i].blend_index = blend_indices[i];
      }
      m_is_animated = true;
    }

    m_elements = new_loader->elements();
  }

  utility::destruct<loader>(new_loader);
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(new_loader));
  return result;
}

} // namespace rx::model