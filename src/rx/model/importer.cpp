#include "rx/model/loader.h"
#include "rx/model/aobake.h"

#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/filesystem/buffered_file.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/map.h"
#include "rx/core/log.h"

#include "rx/core/math/abs.h"
#include "rx/core/math/constants.h"

#include "rx/math/mat3x3.h"

RX_LOG("model/importer", logger);

namespace Rx::Model {

Importer::Importer(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_meshes{allocator()}
  , m_elements{allocator()}
  , m_positions{allocator()}
  , m_coordinates{allocator()}
  , m_normals{allocator()}
  , m_tangents{allocator()}
  , m_blend_indices{allocator()}
  , m_blend_weights{allocator()}
  , m_clips{allocator()}
  , m_name{allocator()}
  , m_report{allocator(), *logger}
{
}

bool Importer::load(Stream::UntrackedStream& _stream) {
  m_name = _stream.name();
  m_report.rename(m_name);

  if (!read(_stream)) {
    return false;
  }

  if (m_elements.is_empty() || m_positions.is_empty()) {
    return m_report.error("missing vertices");
  }

  // Ensure none of the elements go out of bounds.
  const Size vertices = m_positions.size();
  Uint32 max_element = 0;
  m_elements.each_fwd([&max_element](Uint32 _element) {
    max_element = Algorithm::max(max_element, _element);
  });

  if (max_element >= vertices) {
    return m_report.error("element %zu out of bounds", max_element);
  }

  if (m_elements.size() % 3 != 0) {
    return m_report.error("unfinished triangles");
  }

  logger->verbose("%zu triangles, %zu vertices, %zu meshes",
    m_elements.size() / 3, m_positions.size(), m_meshes.size());

  // Check for normals.
  if (m_normals.is_empty()) {
    logger->warning("missing normals");
    if (!generate_normals()) {
      return false;
    }
  }

  // Check for tangents.
  if (m_tangents.is_empty()) {
    // Generating tangent vectors cannot be done unless the model contains
    // appropriate texture coordinates.
    if (m_coordinates.is_empty()) {
      return m_report.error("missing tangents and texture coordinates, bailing");
    } else {
      logger->warning("missing tangents, generating them");
      if (!generate_tangents()) {
        return false;
      }
    }
  }

  // Ensure none of the normals, tangents or coordinates go out of bounds of
  // the loaded model.
  if (m_normals.size() != vertices) {
    logger->warning("too %s normals",
      m_normals.size() > vertices ? "many" : "few");
    if (!m_normals.resize(vertices)) {
      return m_report.error("out of memory");
    }
  }

  if (m_tangents.size() != vertices) {
    logger->warning("too %s tangents",
      m_tangents.size() > vertices ? "many" : "few");
    if (!m_tangents.resize(vertices)) {
      return m_report.error("out of memory");
    }
  }

  if (!m_coordinates.is_empty() && m_coordinates.size() != vertices) {
    logger->warning("too %s coordinates",
      m_coordinates.size() > vertices ? "many" : "few");
    if (!m_coordinates.resize(vertices)) {
      return m_report.error("out of memory");
    }
  }

  // Coalesce batches that share the same material.
  struct Batch {
    Size offset;
    Size count;
  };

  Map<String, Vector<Batch>> batches{allocator()};
  auto batch = [&](const Mesh& _mesh) {
    if (auto* find = batches.find(_mesh.material)) {
      return find->emplace_back(_mesh.offset, _mesh.count);
    } else {
      Vector<Batch> batch{allocator()};
      if (!batch.emplace_back(_mesh.offset, _mesh.count)) {
        return false;
      }
      return batches.insert(_mesh.material, Utility::move(batch)) != nullptr;
    }
  };

  if (!m_meshes.each_fwd(batch)) {
    return m_report.error("out of memory");
  }

  Vector<Mesh> optimized_meshes{allocator()};
  Vector<Uint32> optimized_elements{allocator()};

  auto coalesce_batch = [&](const String& _material_name, const Vector<Batch>& _batches) {
    const auto n_elements = optimized_elements.size();

    auto append_batch = [&](const Batch& _batch) {
      const auto count = optimized_elements.size();
      if (!optimized_elements.resize(count + _batch.count)) {
        return false;
      }
      Memory::copy(optimized_elements.data() + count,
        m_elements.data() + _batch.offset, _batch.count);
      return true;
    };

    if (!_batches.each_fwd(append_batch)) {
      return false;
    }

    const auto count = optimized_elements.size() - n_elements;
    return optimized_meshes.emplace_back(n_elements, count, _material_name,
      Vector<Vector<Math::AABB>>{allocator()});
  };

  if (!batches.each_pair(coalesce_batch)) {
    return m_report.error("out of memory");
  }

  if (optimized_meshes.size() < m_meshes.size()) {
    logger->info("reduced %zu meshes to %zu", m_meshes.size(),
      optimized_meshes.size());
  }

  m_meshes = Utility::move(optimized_meshes);
  m_elements = Utility::move(optimized_elements);

  // Calculate per frame AABBs for each mesh.
  const auto n_animations = m_clips.size();
  const auto n_meshes = m_meshes.size();

  const auto animated = n_animations > 0;

  for (Size i = 0; i < n_meshes; i++) {
    auto& mesh = m_meshes[i];
    if (!mesh.bounds.resize(animated ? n_animations : 1, {allocator()})) {
      return false;
    }
    if (animated) {
      const auto& frames = m_skeleton->lb_frames();
      const auto& joints = m_skeleton->joints();

      for (Size j = 0; j < n_animations; j++) {
        const auto& animation = m_clips[j];
        for (Size k = 0; k < mesh.count; k++) {
          const auto element = m_elements[mesh.offset + k];
          const auto& position = m_positions[element];
          const auto& blend_indices = m_blend_indices[element];
          const auto& blend_weights = m_blend_weights[element];
          if (!mesh.bounds[j].resize(animation.frame_count)) {
            return false;
          }
          for (Size l = 0; l < animation.frame_count; l++) {
            const auto index = (animation.frame_offset + l) * joints.size();
            Math::Mat3x4f transform;
            transform  = frames[index + blend_indices.x] * blend_weights.x;
            transform += frames[index + blend_indices.y] * blend_weights.y;
            transform += frames[index + blend_indices.z] * blend_weights.z;
            transform += frames[index + blend_indices.w] * blend_weights.w;
            const Math::Vec3f x = {transform.x.x, transform.y.x, transform.z.x};
            const Math::Vec3f y = {transform.x.y, transform.y.y, transform.z.y};
            const Math::Vec3f z = {transform.x.z, transform.y.z, transform.z.z};
            const Math::Vec3f w = {transform.x.w, transform.y.w, transform.z.w};
            const Math::Mat4x4f m = {
              {x.x, x.y, x.z, 0.0f},
              {y.x, y.y, y.z, 0.0f},
              {z.x, z.y, z.z, 0.0f},
              {w.x, w.y, w.z, 1.0f}
            };
            mesh.bounds[j][l].expand(Math::transform_point(position, m));
          }
        }
      }
    } else {
      // Calculate the bounds for this mesh.
      Math::AABB aabb;
      for (Size k = 0; k < mesh.count; k++) {
        const auto element = m_elements[mesh.offset + k];
        const auto& position = m_positions[element];
        aabb.expand(position);
      }
      if (!mesh.bounds[0].push_back(aabb)) {
        return false;
      }
    }
  }

  // Check for occlusions.
  if (m_occlusions.is_empty()) {
    Math::AABB aabb;
    m_meshes.each_fwd([&](const Mesh& _mesh) {
      aabb.expand(_mesh.bounds[0][0]);
    });

    /*
    auto& scheduler = Concurrency::ThreadPool::instance();

    // Set them all to 1.0 (full distance).
    AoConfig config;
    if (auto ao = bake_ao(scheduler, aabb, m_positions, m_elements, config)) {
      m_occlusions = Utility::move(*ao);
    }
    */

    // Set them all to 1.0 if none.
    if (m_occlusions.is_empty() && !m_occlusions.resize(m_positions.size(), 1.0f)) {
      return m_report.error("out of memory");
    }
  }

  return true;
}

bool Importer::load(const String& _file_name) {
  if (auto file = Filesystem::BufferedFile::open(allocator(), _file_name, "r")) {
    return load(*file);
  }
  return false;
}

bool Importer::generate_normals() {
  const auto n_vertices = m_positions.size();
  const auto n_elements = m_elements.size();

  if (!m_normals.resize(n_vertices)) {
    return m_report.error("out of memory");
  }

  for (Size i = 0; i < n_elements; i += 3) {
    const Uint32 index0 = m_elements[i + 0];
    const Uint32 index1 = m_elements[i + 1];
    const Uint32 index2 = m_elements[i + 2];

    const Math::Vec3f p1p0{m_positions[index1] - m_positions[index0]};
    const Math::Vec3f p2p0{m_positions[index2] - m_positions[index0]};

    const Math::Vec3f normal{Math::normalize(Math::cross(p1p0, p2p0))};

    m_normals[index0] += normal;
    m_normals[index1] += normal;
    m_normals[index2] += normal;
  }

  for (Size i{0}; i < n_vertices; i++) {
    m_normals[i] = Math::normalize(m_normals[i]);
  }

  return true;
}

bool Importer::generate_tangents() {
  const auto n_vertices = m_positions.size();
  const auto n_elements = m_elements.size();

  Vector<Math::Vec3f> tangents{allocator()};
  Vector<Math::Vec3f> bitangents{allocator()};

  bool result = true;
  result &= tangents.resize(n_vertices);
  result &= bitangents.resize(n_vertices);
  result &= m_tangents.resize(n_vertices);
  if (!result) {
    return m_report.error("out of memory");
  }

  for (Size i = 0; i < n_elements; i += 3) {
    const Uint32 index0 = m_elements[i + 0];
    const Uint32 index1 = m_elements[i + 1];
    const Uint32 index2 = m_elements[i + 2];

    const Math::Mat3x3f triangle{m_positions[index0], m_positions[index1], m_positions[index2]};

    const Math::Vec2f uv0{m_coordinates[index1] - m_coordinates[index0]};
    const Math::Vec2f uv1{m_coordinates[index2] - m_coordinates[index0]};

    const Math::Vec3f q1{triangle.y - triangle.x};
    const Math::Vec3f q2{triangle.z - triangle.x};

    const Float32 det = uv0.s * uv1.t - uv1.s * uv0.t;

    if (Math::abs(det) <= Math::EPSILON<Float32>) {
      return false;
    }

    const Float32 inv_det = 1.0f / det;

    const Math::Vec3f tangent{
      inv_det*(uv1.t*q1.x - uv0.t*q2.x),
      inv_det*(uv1.t*q1.y - uv0.t*q2.y),
      inv_det*(uv1.t*q1.z - uv0.t*q2.z)
    };

    const Math::Vec3f bitangent{
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

  for (Size i = 0; i < n_vertices; i++) {
    const auto& normal = m_normals[i];
    const auto& tangent = tangents[i];
    const auto& bitangent = bitangents[i];

    const auto real_tangent =
      Math::normalize(tangent - normal * Math::dot(normal, tangent));

    const auto real_bitangent =
      Math::dot(Math::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;

    m_tangents[i] = {real_tangent.x, real_tangent.y, real_tangent.z, real_bitangent};
  }

  return true;
}

} // namespace Rx::Model
