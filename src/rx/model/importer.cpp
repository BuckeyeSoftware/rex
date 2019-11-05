#include <string.h> // memcpy

#include "rx/model/loader.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/map.h"
#include "rx/core/log.h"

#include "rx/core/math/abs.h"

#include "rx/math/constants.h"
#include "rx/math/mat3x3.h"

RX_LOG("model/importer", logger);

namespace rx::model {

importer::importer(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_meshes{m_allocator}
  , m_elements{m_allocator}
  , m_positions{m_allocator}
  , m_coordinates{m_allocator}
  , m_normals{m_allocator}
  , m_tangents{m_allocator}
  , m_blend_indices{m_allocator}
  , m_blend_weights{m_allocator}
  , m_frames{m_allocator}
  , m_animations{m_allocator}
  , m_joints{m_allocator}
  , m_file_name{m_allocator}
{
}

bool importer::load(const string& _file_name) {
  const auto data = filesystem::read_binary_file(_file_name);
  if (!data) {
    return false;
  }

  m_file_name = _file_name;

  if (!read(*data)) {
    return false;
  }

  if (m_elements.is_empty() || m_positions.is_empty()) {
    return error("'%s' lacks vertices", _file_name);
  }

  // Ensure none of the elements go out of bounds.
  const rx_size vertices{m_positions.size()};
  rx_u32 max_element{0};
  m_elements.each_fwd([&max_element](rx_u32 _element) {
    max_element = algorithm::max(max_element, _element);
  });

  if (max_element >= vertices) {
    return error("element %zu out of bounds", max_element);
  }

  if (m_elements.size() % 3 != 0) {
    return error("unfinished triangles");
  }

  logger(log::level::k_verbose, "%zu triangles, %zu vertices, %zu meshes",
    m_elements.size() / 3, m_positions.size(), m_meshes.size());

  // Check for normals.
  if (m_normals.is_empty()) {
    logger(log::level::k_warning, "'%s' lacks normals", _file_name);
    generate_normals();
  }

  // Check for tangents.
  if (m_tangents.is_empty()) {
    // Generating tangent vectors cannot be done unless the model contains
    // appropriate texture coordinates.
    if (m_coordinates.is_empty()) {
      return error("'%s' lacks tangents and texture coordinates bailing", _file_name);
    } else {
      logger(log::level::k_warning, "'%s' lacks tangents", _file_name);
      if (!generate_tangents()) {
        return error("'%s' could not generate tangents", _file_name);
      }
    }
  }

  // Ensure none of the normals, tangents or coordinates go out of bounds of
  // the loaded model.
  if (m_normals.size() != vertices) {
    logger(log::level::k_warning, "too %s normals",
      m_normals.size() > vertices ? "many" : "few");
    m_normals.resize(vertices);
  }

  if (m_tangents.size() != vertices) {
    logger(log::level::k_warning, "too %s tangents",
      m_tangents.size() > vertices ? "many" : "few");
    m_tangents.resize(vertices);
  }

  if (!m_coordinates.is_empty() && m_coordinates.size() != vertices) {
    logger(log::level::k_warning, "too %s coordinates",
      m_coordinates.size() > vertices ? "many" : "few");
    m_coordinates.resize(vertices);
  }

  // Coalesce batches that share the same material.
  struct batch {
    rx_size offset;
    rx_size count;
    math::aabb bounds;
  };

  map<string, vector<batch>> batches{m_allocator};
  m_meshes.each_fwd([&](const mesh& _mesh) {
    math::aabb bounds;
    for (rx_size i{0}; i < _mesh.count; i++) {
      bounds.expand(m_positions[m_elements[_mesh.offset + i]]);
    }
    if (auto* find{batches.find(_mesh.material)}) {
      find->push_back({_mesh.offset, _mesh.count, bounds});
    } else {
      batches.insert(_mesh.material, {m_allocator, 1, batch{_mesh.offset, _mesh.count, bounds}});
    }
  });

  vector<mesh> optimized_meshes{m_allocator};
  vector<rx_u32> optimized_elements{m_allocator};
  batches.each_pair([&](const string& _material_name, const vector<batch>& _batches) {
    math::aabb bounds;
    const rx_size elements{optimized_elements.size()};
    _batches.each_fwd([&](const batch& _batch) {
      const rx_size count{optimized_elements.size()};
      optimized_elements.resize(count + _batch.count);
      memcpy(optimized_elements.data() + count,
        m_elements.data() + _batch.offset, sizeof(rx_u32) * _batch.count);
      bounds.expand(_batch.bounds);
    });
    const rx_size count{optimized_elements.size() - elements};
    optimized_meshes.push_back({elements, count, _material_name, bounds});
  });

  if (optimized_meshes.size() < m_meshes.size()) {
    logger(log::level::k_info, "reduced %zu meshes to %zu", m_meshes.size(),
      optimized_meshes.size());
  }

  m_meshes = utility::move(optimized_meshes);
  m_elements = utility::move(optimized_elements);

  return true;
}

void importer::generate_normals() {
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

bool importer::generate_tangents() {
  const rx_size vertex_count{m_positions.size()};

  vector<math::vec3f> tangents{m_allocator, vertex_count};
  vector<math::vec3f> bitangents{m_allocator, vertex_count};

  for (rx_size i{0}; i < m_elements.size(); i += 3) {
    const rx_u32 index0{m_elements[i+0]};
    const rx_u32 index1{m_elements[i+1]};
    const rx_u32 index2{m_elements[i+2]};

    const math::mat3x3f triangle{m_positions[index0], m_positions[index1], m_positions[index2]};

    const math::vec2f uv0{m_coordinates[index1] - m_coordinates[index0]};
    const math::vec2f uv1{m_coordinates[index2] - m_coordinates[index0]};

    const math::vec3f q1{triangle.y - triangle.x};
    const math::vec3f q2{triangle.z - triangle.x};

    const rx_f32 det{uv0.s*uv1.t - uv1.s*uv0.t};

    if (math::abs(det) <= math::k_epsilon<rx_f32>) {
      return false;
    }

    const rx_f32 inv_det{1.0f/det};

    const math::vec3f tangent{
      inv_det*(uv1.t*q1.x - uv0.t*q2.x),
      inv_det*(uv1.t*q1.y - uv0.t*q2.y),
      inv_det*(uv1.t*q1.z - uv0.t*q2.z)
    };

    const math::vec3f bitangent{
      inv_det*(-uv1.s*q1.x * uv0.s*q2.x),
      inv_det*(-uv1.s*q1.y * uv0.s*q2.y),
      inv_det*(-uv1.s*q1.z * uv0.s*q2.z)
    };

    tangents[index0] += tangent;
    tangents[index1] += tangent;
    tangents[index2] += tangent;

    bitangents[index0] += bitangent;
    bitangents[index1] += bitangent;
    bitangents[index2] += bitangent;
  }

  m_tangents.resize(vertex_count);

  for (rx_size i{0}; i < vertex_count; i++) {
    const math::vec3f& normal{m_normals[i]};
    const math::vec3f& tangent{tangents[i]};
    const math::vec3f& bitangent{bitangents[i]};

    const auto real_tangent{math::normalize(tangent - normal * math::dot(normal, tangent))};
    const auto real_bitangent{math::dot(math::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f};
    m_tangents[i] = {real_tangent.x, real_tangent.y, real_tangent.z, real_bitangent};
  }

  return true;
}

void importer::write_log(log::level _level, string&& message_) const {
  if (m_file_name.is_empty()) {
    logger(_level, "%s", utility::move(message_));
  } else {
    logger(_level, "%s: %s", m_file_name, utility::move(message_));
  }
}

} // namespace rx::model
