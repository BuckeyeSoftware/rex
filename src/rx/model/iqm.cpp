#include <string.h> // memcmp

#include "rx/math/quat.h"

#include "rx/model/iqm.h"

namespace rx::model {

struct iqm_mesh {
  rx_u32 name;
  rx_u32 material;
  rx_u32 first_vertex;
  rx_u32 num_vertexes;
  rx_u32 first_triangle;
  rx_u32 num_triangles;
};

enum class vertex_attribute {
  k_position,
  k_coordinate,
  k_normal,
  k_tangent,
  k_blend_indexes,
  k_blend_weights,
  k_color,
  k_custom = 0x10
};

enum class vertex_format {
  k_s8,
  k_u8,
  k_s16,
  k_u16,
  k_s32,
  k_u32,
  k_f16,
  k_f32,
  k_f64
};

struct iqm_triangle {
  rx_u32 vertex[3];
};

struct iqm_joint {
  rx_u32 name;
  rx_s32 parent;
  rx_f32 translate[3];
  rx_f32 rotate[4];
  rx_f32 scale[3];
};

struct iqm_pose {
  rx_s32 parent;
  rx_u32 mask;
  rx_f32 channel_offset[10];
  rx_f32 channel_scale[10];
};

struct iqm_animation {
  rx_u32 name;
  rx_u32 first_frame;
  rx_u32 num_frames;
  rx_f32 frame_rate;
  rx_u32 flags;
};

struct iqm_vertex_array {
  rx_u32 type;
  rx_u32 flags;
  rx_u32 format;
  rx_u32 size;
  rx_u32 offset;
};

struct iqm_bounds {
  rx_f32 min[3];
  rx_f32 max[3];
  rx_u32 xy_radius;
  rx_u32 radius;
};

struct iqm::header {
  char magic[16];
  rx_u32 version;
  rx_u32 file_size;
  rx_u32 flags;
  rx_u32 text;
  rx_u32 text_offset;
  rx_u32 meshes;
  rx_u32 meshes_offset;
  rx_u32 vertex_arrays;
  rx_u32 vertexes;
  rx_u32 vertex_arrays_offset;
  rx_u32 triangles;
  rx_u32 triangles_offset;
  rx_u32 adjacency_offset;
  rx_u32 joints;
  rx_u32 joints_offset;
  rx_u32 poses;
  rx_u32 poses_offset;
  rx_u32 animations;
  rx_u32 animations_offset;
  rx_u32 frames;
  rx_u32 frame_channels;
  rx_u32 frames_offset;
  rx_u32 bounds_offset;
  rx_u32 comment;
  rx_u32 comment_offset;
  rx_u32 extensions;
  rx_u32 extensions_offset;
};

bool iqm::read(const array<rx_byte>& _data) {
  const auto read_header{reinterpret_cast<const header*>(_data.data())};
  if (memcmp(read_header->magic, "INTERQUAKEMODEL\0", sizeof read_header->magic) != 0) {
    return error("malformed magic: %s", read_header->magic);
  }

  if (read_header->version != 2) {
    return error("unsupported version %d", read_header->version);
  }

  if (read_header->file_size != _data.size()) {
    return error("unexpected end of file");
  }

  if (read_header->meshes && !read_meshes(*read_header, _data)) {
    return false;
  }

  if (read_header->animations && !read_animations(*read_header, _data)) {
    return false;
  }

  return true;
}

bool iqm::read_meshes(const header& _header, const array<rx_byte>& _data) {
  const char* string_table{_header.text_offset ? reinterpret_cast<const char *>(_data.data() + _header.text_offset) : ""};

  const rx_f32* in_position{nullptr};
  const rx_f32* in_normal{nullptr};
  const rx_f32* in_tangent{nullptr};
  const rx_f32* in_coordinate{nullptr};
  const rx_byte* in_blend_index{nullptr};
  const rx_byte* in_blend_weight{nullptr};

  const auto vertex_arrays{reinterpret_cast<const iqm_vertex_array*>(_data.data() + _header.vertex_arrays_offset)};
  for (rx_u32 i{0}; i < _header.vertex_arrays; i++) {
    const iqm_vertex_array& array{vertex_arrays[i]};
    const auto attribute{static_cast<vertex_attribute>(array.type)};
    const auto format{static_cast<vertex_format>(array.format)};
    const rx_size size{array.size};
    const rx_size offset{array.offset};

    switch (attribute) {
    case vertex_attribute::k_position:
      if (format != vertex_format::k_f32) {
        return error("unsupported format for position");
      }
      if (size != 3) {
        return error("invalid size for position");
      }
      in_position = reinterpret_cast<const rx_f32*>(_data.data() + offset);
      break;
    case vertex_attribute::k_normal:
      if (format != vertex_format::k_f32) {
        return error("unsupported format for normal");
      }
      if (size != 3) {
        return error("invalid size for normal");
      }
      in_normal = reinterpret_cast<const rx_f32*>(_data.data() + offset);
      break;
    case vertex_attribute::k_tangent:
      if (format != vertex_format::k_f32) {
        return error("unsupported format for tangent");
      }
      if (size != 4) {
        return error("invalid size for tangent");
      }
      in_tangent = reinterpret_cast<const rx_f32*>(_data.data() + offset);
      break;
    case vertex_attribute::k_coordinate:
      if (format != vertex_format::k_f32) {
        return error("unsupported format for coordinate");
      }
      if (size != 2) {
        return error("invalid size for coordinate");
      }
      in_coordinate = reinterpret_cast<const rx_f32*>(_data.data() + offset);
      break;
    case vertex_attribute::k_blend_weights:
      if (format != vertex_format::k_u8) {
        return error("unsupported format for blend weights");
      }
      if (size != 4) {
        return error("invalid size for blend weights");
      }
      in_blend_weight = _data.data() + offset;
      break;
    case vertex_attribute::k_blend_indexes:
      if (format != vertex_format::k_u8) {
        return error("unsupported format for blend indices");
      }
      if (size != 4) {
        return error("invalid size for blend indices");
      }
      in_blend_index = _data.data() + offset;
    default:
      break;
    }
  }

  const rx_size vertices{_header.vertexes};

  if (in_position) {
    m_positions.resize(vertices);
  }

  if (in_normal) {
    m_normals.resize(vertices);
  }

  if (in_tangent) {
    m_tangents.resize(vertices);
  }

  if (in_coordinate) {
    m_coordinates.resize(vertices);
  }

  if (in_blend_index) {
    m_blend_indices.resize(vertices);
  }

  if (in_blend_weight) {
    m_blend_weights.resize(vertices);
  }

  for (rx_size i{0}; i < vertices; i++) {
    if (in_position) {
      m_positions[i].x = in_position[i * 3 + 0];
      m_positions[i].y = in_position[i * 3 + 2];
      m_positions[i].z = in_position[i * 3 + 1];
    }

    if (in_normal) {
      m_normals[i].x = in_normal[i * 3 + 0];
      m_normals[i].y = in_normal[i * 3 + 2];
      m_normals[i].z = in_normal[i * 3 + 1];
    }

    if (in_tangent) {
      m_tangents[i].x = in_tangent[i * 4 + 0];
      m_tangents[i].y = in_tangent[i * 4 + 2];
      m_tangents[i].z = in_tangent[i * 4 + 1];
      m_tangents[i].w = in_tangent[i * 4 + 3];
    }

    if (in_coordinate) {
      m_coordinates[i].u = in_coordinate[i * 2 + 0];
      m_coordinates[i].v = in_coordinate[i * 2 + 1];
    }

    if (in_blend_index) {
      m_blend_indices[i].x = in_blend_index[i * 4 + 0];
      m_blend_indices[i].y = in_blend_index[i * 4 + 1];
      m_blend_indices[i].z = in_blend_index[i * 4 + 2];
      m_blend_indices[i].w = in_blend_index[i * 4 + 3];
    }

    if (in_blend_weight) {
      m_blend_weights[i].x = in_blend_weight[i * 4 + 0];
      m_blend_weights[i].y = in_blend_weight[i * 4 + 1];
      m_blend_weights[i].z = in_blend_weight[i * 4 + 2];
      m_blend_weights[i].w = in_blend_weight[i * 4 + 3];
    }
  }

  const auto meshes{reinterpret_cast<const iqm_mesh *>(_data.data() + _header.meshes_offset)};
  for (rx_u32 i{0}; i < _header.meshes; i++) {
    const auto& this_mesh{meshes[i]};
    const char* material_name{string_table + this_mesh.material};
    m_meshes.push_back({this_mesh.first_triangle * 3, this_mesh.num_triangles * 3, material_name});
  }

  m_elements.resize(_header.triangles * 3);
  for (rx_u32 i{0}; i < _header.triangles; i++) {
    const auto* this_triangle{reinterpret_cast<const iqm_triangle*>(_data.data() + _header.triangles_offset) + i};
    m_elements[i * 3 + 0] = this_triangle->vertex[0];
    m_elements[i * 3 + 1] = this_triangle->vertex[1];
    m_elements[i * 3 + 2] = this_triangle->vertex[2];
  }

  return true;
}

bool iqm::read_animations(const header& _header, const array<rx_byte>& _data) {
  const auto n_joints{static_cast<rx_size>(_header.joints)};


  array<math::mat3x4f> generic_base_frame{m_allocator, n_joints};
  array<math::mat3x4f> inverse_base_frame{m_allocator, n_joints};

  m_joints.resize(n_joints);

  const auto joints{reinterpret_cast<const iqm_joint*>(_data.data() + _header.joints_offset)};

  // read base pose
  for (rx_size i{0}; i < n_joints; i++) {
    const auto& this_joint{joints[i]};

    // IQM is Z up, we're Y up
    const math::vec3f scale{this_joint.scale[0], this_joint.scale[2], this_joint.scale[1]};
    const math::quatf rotate{this_joint.rotate[0], this_joint.rotate[2], this_joint.rotate[1], -this_joint.rotate[3]};
    const math::vec3f translate{this_joint.translate[0], this_joint.translate[2], this_joint.translate[1]};

    generic_base_frame[i] = math::mat3x4f{scale, math::normalize(rotate), translate};
    inverse_base_frame[i] = math::mat3x4f::invert(generic_base_frame[i]);

    if (this_joint.parent >= 0) {
      generic_base_frame[i] = generic_base_frame[this_joint.parent] * generic_base_frame[i];
      inverse_base_frame[i] *= inverse_base_frame[this_joint.parent];
    }

    m_joints[i] = {generic_base_frame[i], this_joint.parent};
  }

  const char* string_table{reinterpret_cast<const char *>(_data.data() + _header.text_offset)};
  const iqm_animation* animations{reinterpret_cast<const iqm_animation*>(_data.data() + _header.animations_offset)};
  for (rx_u32 i{0}; i < _header.animations; i++) {
    const iqm_animation& this_animation{animations[i]};
    m_animations.push_back({this_animation.frame_rate, this_animation.first_frame,
      this_animation.num_frames, string_table + this_animation.name});
  }

  m_frames.resize(n_joints * _header.frames);
  const auto* poses{reinterpret_cast<const iqm_pose*>(_data.data() + _header.poses_offset)};
  const rx_u16* frame_data{reinterpret_cast<const rx_u16*>(_data.data() + _header.frames_offset)};

  for (rx_u32 i{0}; i < _header.frames; i++) {
    for (rx_u32 j{0}; j < _header.poses; j++) {
      const iqm_pose& this_pose{poses[j]};
      rx_f32 channel_data[10];
      for (rx_size k{0}; k < 10; k++) {
        channel_data[k] = this_pose.channel_offset[k];
        if (this_pose.mask & (1 << k)) {
          channel_data[k] += (*frame_data++) * this_pose.channel_scale[k];
        }
      }

      // IQM is Z up, we're Y up
      const math::vec3f scale{channel_data[7], channel_data[9], channel_data[8]};
      const math::quatf rotate{channel_data[3], channel_data[5], channel_data[4], -channel_data[6]};
      const math::vec3f translate{channel_data[0], channel_data[2], channel_data[1]};

      math::mat3x4 scale_rotate_translate{scale, math::normalize(rotate), translate};

      // Concatenate each and every pose with the inverse base pose to avoid having to do it
      // at animation time; if the joint has a parent, then it needs to be preconcatenated with
      // it's parent's base pose, this will all negate at animation time; consider:
      //
      // (parent_pose * parent_inverse_base_pose) * (parent_base_pose * child_pose * child_inverse_base_pose) =>
      // parent_pose * (parent_inverse_base_pose * parent_base_pose) * child_pose * child_inverse_base_pose =>
      // parent_pose * child_pose * child_inverse_base_pose
      if (this_pose.parent >= 0) {
        scale_rotate_translate = generic_base_frame[this_pose.parent] * scale_rotate_translate * inverse_base_frame[j];
      } else {
        scale_rotate_translate = scale_rotate_translate * inverse_base_frame[j];
      }

      // The parent multiplication is done here to avoid having to do it at animation time too,
      // this will need to be moved to support more complicated animation blending
      if (joints[j].parent >= 0) {
        scale_rotate_translate = m_frames[i * _header.poses + joints[j].parent] * scale_rotate_translate;
      }

      m_frames[i * _header.poses + j] = scale_rotate_translate;
    }
  }

  return true;
}

} // namespace rx::model