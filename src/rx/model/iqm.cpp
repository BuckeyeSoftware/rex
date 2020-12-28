#include <string.h> // memcmp

#include "rx/math/quat.h"

#include "rx/model/iqm.h"

#include "rx/core/stream.h"

namespace Rx::Model {

struct IQMMesh {
  Uint32 name;
  Uint32 material;
  Uint32 first_vertex;
  Uint32 num_vertexes;
  Uint32 first_triangle;
  Uint32 num_triangles;
};

enum class VertexAttribute {
  k_position,
  k_coordinate,
  k_normal,
  k_tangent,
  k_blend_indexes,
  k_blend_weights,
  k_color,
  k_custom = 0x10
};

enum class VertexFormat {
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

struct IQMTriangle {
  Uint32 vertex[3];
};

struct IQMJoint {
  Uint32 name;
  Sint32 parent;
  Float32 translate[3];
  Float32 rotate[4];
  Float32 scale[3];
};

struct IQMPose {
  Sint32 parent;
  Uint32 mask;
  Float32 channel_offset[10];
  Float32 channel_scale[10];
};

struct IQMAnimation {
  Uint32 name;
  Uint32 first_frame;
  Uint32 num_frames;
  Float32 frame_rate;
  Uint32 flags;
};

struct IQMVertexArray {
  Uint32 type;
  Uint32 flags;
  Uint32 format;
  Uint32 size;
  Uint32 offset;
};

struct IQMBounds {
  Float32 min[3];
  Float32 max[3];
  Uint32 xy_radius;
  Uint32 radius;
};

struct IQM::Header {
  char magic[16];
  Uint32 version;
  Uint32 file_size;
  Uint32 flags;
  Uint32 text;
  Uint32 text_offset;
  Uint32 meshes;
  Uint32 meshes_offset;
  Uint32 vertex_arrays;
  Uint32 vertexes;
  Uint32 vertex_arrays_offset;
  Uint32 triangles;
  Uint32 triangles_offset;
  Uint32 adjacency_offset;
  Uint32 joints;
  Uint32 joints_offset;
  Uint32 poses;
  Uint32 poses_offset;
  Uint32 animations;
  Uint32 animations_offset;
  Uint32 frames;
  Uint32 frame_channels;
  Uint32 frames_offset;
  Uint32 bounds_offset;
  Uint32 comment;
  Uint32 comment_offset;
  Uint32 extensions;
  Uint32 extensions_offset;
};

bool IQM::read(Stream* _stream) {
  const auto size = _stream->size();
  if (!size) {
    return false;
  }

  // Don't read the contents entierly into memory until we know it looks like a
  // valid IQM.
  Header read_header;
  if (_stream->read(reinterpret_cast<Byte*>(&read_header), sizeof read_header) != sizeof read_header) {
    return error("could not read header");
  }

  // Don't load files which report the wrong size.
  if (*size != read_header.file_size ||
    memcmp(read_header.magic, "INTERQUAKEMODEL\0", sizeof read_header.magic) != 0)
  {
    return error("malformed header");
  }

  // Don't load files which have the wrong version.
  if (read_header.version != 2) {
    return error("unsupported iqm version %d", read_header.version);
  }

  // Offsets in the header are relative to the beginning of the file, make a
  // hole in the memory and skip it, such that |read_meshes| and
  // |read_animations| can use the |read_header|'s values directly.
  Vector<Byte> data{allocator(), static_cast<Size>(*size), Utility::UninitializedTag{}};
  const auto size_no_header = data.size() - sizeof read_header;
  if (_stream->read(data.data() + sizeof read_header, size_no_header) != size_no_header) {
    return error("unexpected end of file");
  }

  if (read_header.meshes && !read_meshes(read_header, data)) {
    return false;
  }

  if (read_header.animations && !read_animations(read_header, data)) {
    return false;
  }

  return true;
}

bool IQM::read_meshes(const Header& _header, const Vector<Byte>& _data) {
  const char* string_table{_header.text_offset ? reinterpret_cast<const char *>(_data.data() + _header.text_offset) : ""};

  const Float32* in_position{nullptr};
  const Float32* in_normal{nullptr};
  const Float32* in_tangent{nullptr};
  const Float32* in_coordinate{nullptr};
  const Byte* in_blend_index{nullptr};
  const Byte* in_blend_weight{nullptr};

  const auto vertex_arrays{reinterpret_cast<const IQMVertexArray*>(_data.data() + _header.vertex_arrays_offset)};
  for (Uint32 i{0}; i < _header.vertex_arrays; i++) {
    const IQMVertexArray& array{vertex_arrays[i]};
    const auto attribute{static_cast<VertexAttribute>(array.type)};
    const auto format{static_cast<VertexFormat>(array.format)};
    const Size size{array.size};
    const Size offset{array.offset};

    switch (attribute) {
    case VertexAttribute::k_position:
      if (format != VertexFormat::k_f32) {
        return error("unsupported format for position");
      }
      if (size != 3) {
        return error("invalid size for position");
      }
      in_position = reinterpret_cast<const Float32*>(_data.data() + offset);
      break;
    case VertexAttribute::k_normal:
      if (format != VertexFormat::k_f32) {
        return error("unsupported format for normal");
      }
      if (size != 3) {
        return error("invalid size for normal");
      }
      in_normal = reinterpret_cast<const Float32*>(_data.data() + offset);
      break;
    case VertexAttribute::k_tangent:
      if (format != VertexFormat::k_f32) {
        return error("unsupported format for tangent");
      }
      if (size != 4) {
        return error("invalid size for tangent");
      }
      in_tangent = reinterpret_cast<const Float32*>(_data.data() + offset);
      break;
    case VertexAttribute::k_coordinate:
      if (format != VertexFormat::k_f32) {
        return error("unsupported format for coordinate");
      }
      if (size != 2) {
        return error("invalid size for coordinate");
      }
      in_coordinate = reinterpret_cast<const Float32*>(_data.data() + offset);
      break;
    case VertexAttribute::k_blend_weights:
      if (format != VertexFormat::k_u8) {
        return error("unsupported format for blend weights");
      }
      if (size != 4) {
        return error("invalid size for blend weights");
      }
      in_blend_weight = _data.data() + offset;
      break;
    case VertexAttribute::k_blend_indexes:
      if (format != VertexFormat::k_u8) {
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

  const Size vertices{_header.vertexes};

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

  for (Size i{0}; i < vertices; i++) {
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

  const auto meshes{reinterpret_cast<const IQMMesh *>(_data.data() + _header.meshes_offset)};
  for (Uint32 i{0}; i < _header.meshes; i++) {
    const auto& this_mesh{meshes[i]};
    const char* material_name{string_table + this_mesh.material};
    m_meshes.emplace_back(this_mesh.first_triangle * 3, this_mesh.num_triangles * 3, material_name, Math::AABB{});
  }

  m_elements.resize(_header.triangles * 3);
  for (Uint32 i{0}; i < _header.triangles; i++) {
    const auto* this_triangle{reinterpret_cast<const IQMTriangle*>(_data.data() + _header.triangles_offset) + i};
    m_elements[i * 3 + 0] = this_triangle->vertex[0];
    m_elements[i * 3 + 1] = this_triangle->vertex[1];
    m_elements[i * 3 + 2] = this_triangle->vertex[2];
  }

  return true;
}

bool IQM::read_animations(const Header& _header, const Vector<Byte>& _data) {
  const auto n_joints{static_cast<Size>(_header.joints)};

  Vector<Math::Mat3x4f> generic_base_frame{allocator(), n_joints};
  Vector<Math::Mat3x4f> inverse_base_frame{allocator(), n_joints};

  m_joints.resize(n_joints);

  const auto joints{reinterpret_cast<const IQMJoint*>(_data.data() + _header.joints_offset)};

  // Read base bind pose.
  for (Size i{0}; i < n_joints; i++) {
    const auto& this_joint{joints[i]};

    // Convert IQM's Z-up coordinate system to Y-up since that's what we use.
    const Math::Vec3f scale{this_joint.scale[0], this_joint.scale[2], this_joint.scale[1]};
    const Math::Quatf rotate{this_joint.rotate[0], this_joint.rotate[2], this_joint.rotate[1], -this_joint.rotate[3]};
    const Math::Vec3f translate{this_joint.translate[0], this_joint.translate[2], this_joint.translate[1]};

    generic_base_frame[i] = Math::Mat3x4f{scale, Math::normalize(rotate), translate};
    inverse_base_frame[i] = Math::Mat3x4f::invert(generic_base_frame[i]);

    if (this_joint.parent >= 0) {
      generic_base_frame[i] = generic_base_frame[this_joint.parent] * generic_base_frame[i];
      inverse_base_frame[i] *= inverse_base_frame[this_joint.parent];
    }

    m_joints[i] = {generic_base_frame[i], this_joint.parent};
  }

  const char* string_table{reinterpret_cast<const char *>(_data.data() + _header.text_offset)};
  const IQMAnimation* animations{reinterpret_cast<const IQMAnimation*>(_data.data() + _header.animations_offset)};
  for (Uint32 i{0}; i < _header.animations; i++) {
    const IQMAnimation& this_animation{animations[i]};
    m_animations.emplace_back(this_animation.frame_rate, this_animation.first_frame,
      this_animation.num_frames, string_table + this_animation.name);
  }

  m_frames.resize(n_joints * _header.frames);
  const auto* poses{reinterpret_cast<const IQMPose*>(_data.data() + _header.poses_offset)};
  const Uint16* frame_data{reinterpret_cast<const Uint16*>(_data.data() + _header.frames_offset)};

  for (Uint32 i{0}; i < _header.frames; i++) {
    for (Uint32 j{0}; j < _header.poses; j++) {
      const IQMPose& this_pose{poses[j]};
      Float32 channel_data[10];
      for (Size k{0}; k < 10; k++) {
        channel_data[k] = this_pose.channel_offset[k];
        if (this_pose.mask & (1 << k)) {
          channel_data[k] += (*frame_data++) * this_pose.channel_scale[k];
        }
      }

      // Convert IQM's Z-up coordinate system to Y-up since that's what we use.
      const Math::Vec3f scale{channel_data[7], channel_data[9], channel_data[8]};
      const Math::Quatf rotate{channel_data[3], channel_data[5], channel_data[4], -channel_data[6]};
      const Math::Vec3f translate{channel_data[0], channel_data[2], channel_data[1]};

      Math::Mat3x4 scale_rotate_translate{scale, Math::normalize(rotate), translate};

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
      // this will need to be moved to support more complicated animation blending.
      if (joints[j].parent >= 0) {
        scale_rotate_translate = m_frames[i * _header.poses + joints[j].parent] * scale_rotate_translate;
      }

      m_frames[i * _header.poses + j] = scale_rotate_translate;
    }
  }

  return true;
}

} // namespace Rx::Model
