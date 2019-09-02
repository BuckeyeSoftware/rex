#include "rx/model/interface.h"
#include "rx/model/iqm.h"

#include "rx/math/trig.h"

namespace rx::model {

interface::~interface() {
  if (m_flags & k_constructed) {
    if (m_flags & k_animated) {
      utility::destruct<vector<animated_vertex>>(&as_animated_vertices);
    } else {
      utility::destruct<vector<vertex>>(&as_vertices);
    }
  }
}

bool interface::load(const string& _file_name) {
  RX_ASSERT(!(m_flags & k_constructed), "already loaded");

  loader* new_loader{nullptr};

  // determine the model format based on the extension
  if (_file_name.ends_with(".iqm")) {
    new_loader = m_allocator->create<iqm>(m_allocator);
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
      utility::construct<vector<vertex>>(&as_vertices, m_allocator, vertices);
      m_flags |= k_constructed;

      for (rx_size i{0}; i < vertices; i++) {
        as_vertices[i].position = positions[i];
        as_vertices[i].normal = normals[i];
        as_vertices[i].tangent = tangents[i];
        as_vertices[i].coordinate = coordinates[i];
      }
    } else {
      const auto& blend_weights{new_loader->blend_weights()};
      const auto& blend_indices{new_loader->blend_indices()};

      utility::construct<vector<animated_vertex>>(&as_animated_vertices, m_allocator, vertices);
      m_flags |= k_constructed;

      for (rx_size i{0}; i < vertices; i++) {
        as_animated_vertices[i].position = positions[i];
        as_animated_vertices[i].normal = normals[i];
        as_animated_vertices[i].tangent = tangents[i];
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weights = blend_weights[i];
        as_animated_vertices[i].blend_indices = blend_indices[i];
      }
      m_flags |= k_animated;
    }

    m_meshes = utility::move(new_loader->meshes());
    m_elements = utility::move(new_loader->elements());
    m_animations = utility::move(new_loader->animations());
    m_frames = utility::move(new_loader->frames());
    m_joints = utility::move(new_loader->joints());
  }

  m_allocator->destroy<loader>(new_loader);
  return result;
}

} // namespace rx::model