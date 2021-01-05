#include "rx/model/loader.h"
#include "rx/model/iqm.h"

#include "rx/core/map.h"
#include "rx/core/ptr.h"
#include "rx/core/json.h"
#include "rx/core/filesystem/file.h"
#include "rx/core/algorithm/clamp.h"

#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/wait_group.h"

#include "rx/math/quat.h"

#include "rx/material/loader.h"

namespace Rx::Model {

RX_LOG("model/loader", logger);

Loader::Loader(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , as_nat{}
  , m_elements{allocator()}
  , m_meshes{allocator()}
  , m_animations{allocator()}
  , m_joints{allocator()}
  , m_positions{allocator()}
  , m_frames{allocator()}
  , m_materials{allocator()}
  , m_name{allocator()}
  , m_flags{0}
{
}

Loader::~Loader() {
  destroy();
}

void Loader::destroy() {
  if (m_flags & CONSTRUCTED) {
    if (m_flags & ANIMATED) {
      Utility::destruct<Vector<AnimatedVertex>>(&as_animated_vertices);
    } else {
      Utility::destruct<Vector<Vertex>>(&as_vertices);
    }
  }

  m_flags = 0;
}

bool Loader::load(Stream* _stream) {
  if (auto contents = read_text_stream(allocator(), _stream)) {
    if (auto disown = contents->disown()) {
      return parse({*disown});
    }
  }
  return false;
}

bool Loader::load(const String& _file_name) {
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file);
  }
  return false;
}

bool Loader::parse(const JSON& _definition) {
  if (!_definition) {
    const auto json_error{_definition.error()};
    if (json_error) {
      return error("%s", *json_error);
    } else {
      return error("empty definition");
    }
  }

  if (!_definition.is_object()) {
    return error("expected Object");
  }

  const auto& name = _definition["name"];
  const auto& file = _definition["file"];
  const auto& materials = _definition["materials"];
  const auto& transform = _definition["transform"];

  if (!name) {
    return error("missing 'name'");
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  m_name = name.as_string();

  if (!file) {
    return error("missing 'file'");
  }

  if (!file.is_string()) {
    return error("expected String for 'file'");
  }

  if (!materials) {
    return error("missing 'materials'");
  }

  if (!materials.is_array_of(JSON::Type::OBJECT) && !materials.is_array_of(JSON::Type::STRING)) {
    return error("expected Array[Object] or Array[String] for 'materials'");
  }

  m_transform = nullopt;
  if (transform && !parse_transform(transform)) {
    return false;
  }

  if (!import(file.as_string())) {
    return false;
  }

  // Load all the materials across multiple threads.
  // concurrency::thread_pool pool{m_allocator, 32};

  // Clear incase we're being run multiple times to change.
  m_materials.clear();

  Concurrency::Mutex mutex;
  Concurrency::WaitGroup group{materials.size()};
  materials.each([&](const JSON& _material) {
    Concurrency::ThreadPool::instance().add([&, _material](int) {
      Material::Loader loader{allocator()};
      if (_material.is_string() && loader.load(_material.as_string())) {
        Concurrency::ScopeLock lock{mutex};
        m_materials.insert(loader.name(), Utility::move(loader));
      } else if (_material.is_object() && loader.parse(_material)) {
        Concurrency::ScopeLock lock{mutex};
        m_materials.insert(loader.name(), Utility::move(loader));
      }
      group.signal();
    });
  });
  group.wait();

  return true;
}

bool Loader::import(const String& _file_name) {
  Ptr<Importer> new_loader;

  // determine the model format based on the extension
  if (_file_name.ends_with(".iqm")) {
    auto& this_allocator = allocator();
    if (!(new_loader = make_ptr<IQM>(this_allocator, this_allocator))) {
      return error("out of memory");
    }
  } else {
    return error("unsupported model format");
  }

  const bool result = new_loader->load(_file_name);
  if (!result) {
    return false;
  }

  // We may possibly call import multiple times.
  destroy();
  m_materials.clear();

  m_positions = Utility::move(new_loader->positions());
  m_meshes = Utility::move(new_loader->meshes());
  m_elements = Utility::move(new_loader->elements());
  m_animations = Utility::move(new_loader->animations());
  m_frames = Utility::move(new_loader->frames());
  m_joints = Utility::move(new_loader->joints());

  const auto& normals = new_loader->normals();
  const auto& tangents = new_loader->tangents();
  const auto& coordinates =new_loader->coordinates();

  const Size n_vertices{m_positions.size()};

  if (m_animations.is_empty()) {
    Utility::construct<Vector<Vertex>>(&as_vertices, allocator(), n_vertices);
    m_flags = CONSTRUCTED;
  } else {
    Utility::construct<Vector<AnimatedVertex>>(&as_animated_vertices, allocator(), n_vertices);
    m_flags = CONSTRUCTED | ANIMATED;
  }

  // Hoist the transform check outside the for loops for faster model loading.
  if (m_transform) {
    const auto transform = m_transform->as_mat4();
    if (m_animations.is_empty()) {
      for (Size i{0}; i < n_vertices; i++) {
        const Math::Vec3f tangent{Math::Mat4x4f::transform_vector({tangents[i].x, tangents[i].y, tangents[i].z}, transform)};
        as_vertices[i].position = Math::Mat4x4f::transform_point(m_positions[i], transform);
        as_vertices[i].normal = Math::Mat4x4f::transform_vector(normals[i], transform);
        as_vertices[i].tangent = {tangent.x, tangent.y, tangent.z, tangents[i].w};
        as_vertices[i].coordinate = coordinates[i];
      }
    } else {
      const auto& blend_weights{new_loader->blend_weights()};
      const auto& blend_indices{new_loader->blend_indices()};
      for (Size i{0}; i < n_vertices; i++) {
        const Math::Vec3f tangent{Math::Mat4x4f::transform_vector({tangents[i].x, tangents[i].y, tangents[i].z}, transform)};
        as_animated_vertices[i].position = Math::Mat4x4f::transform_point(m_positions[i], transform);
        as_animated_vertices[i].normal = Math::Mat4x4f::transform_vector(normals[i], transform);
        as_animated_vertices[i].tangent = {tangent.x, tangent.y, tangent.z, tangents[i].w};
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weights = blend_weights[i];
        as_animated_vertices[i].blend_indices = blend_indices[i];
      }
    }

    const Math::Mat3x4f& xform{
      {transform.x.x, transform.y.x, transform.z.x, transform.w.x},
      {transform.x.y, transform.y.y, transform.z.y, transform.w.y},
      {transform.x.z, transform.y.z, transform.z.z, transform.w.z}};

    const Math::Mat3x4f& inv_xform{Math::Mat3x4f::invert(xform)};

    m_frames.each_fwd([&](Math::Mat3x4f& frame_) {
      frame_ = xform * frame_ * inv_xform;
    });

    m_joints.each_fwd([&](Importer::Joint& joint_) {
      joint_.frame = xform * joint_.frame * inv_xform;
    });

    m_animations.each_fwd([&](Importer::Animation& animation_) {
      animation_.bounds.each_fwd([&](Math::AABB& aabb_) {
        aabb_ = aabb_.transform(transform); // TODO(dweiler): verify?
      });
    });
  } else {
    if (m_animations.is_empty()) {
      for (Size i{0}; i < n_vertices; i++) {
        as_vertices[i].position = m_positions[i];
        as_vertices[i].normal = normals[i];
        as_vertices[i].tangent = tangents[i];
        as_vertices[i].coordinate = coordinates[i];
      }
    } else {
      const auto& blend_weights = new_loader->blend_weights();
      const auto& blend_indices = new_loader->blend_indices();
      for (Size i{0}; i < n_vertices; i++) {
        as_animated_vertices[i].position = m_positions[i];
        as_animated_vertices[i].normal = normals[i];
        as_animated_vertices[i].tangent = tangents[i];
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weights = blend_weights[i];
        as_animated_vertices[i].blend_indices = blend_indices[i];
      }
    }
  }

  // Bounds need to be recalculated if there was a transform
  if (m_transform) {
    m_meshes.each_fwd([&](Mesh& _mesh) {
      Math::AABB bounds;
      for (Size i{0}; i < _mesh.count; i++) {
        if (m_animations.is_empty()) {
          bounds.expand(as_vertices[m_elements[_mesh.offset + i]].position);
        } else {
          bounds.expand(as_animated_vertices[m_elements[_mesh.offset + i]].position);
        }
      }
      _mesh.bounds = bounds;
    });
  }

  return true;
}

bool Loader::parse_transform(const JSON& _transform) {
  const auto& scale = _transform["scale"];
  const auto& rotate = _transform["rotate"];
  const auto& translate = _transform["translate"];

  auto parse_vec3 = [&](const JSON& _array, const char* _tag, Math::Vec3f& result_) {
    if (!_array.is_array_of(JSON::Type::NUMBER, 3)) {
      return error("expected Array[Number, 3] for '%s'", _tag);
    }
    result_.x = Algorithm::clamp(_array[0_z].as_number(), 0.0, 360.0);
    result_.y = Algorithm::clamp(_array[1_z].as_number(), 0.0, 360.0);
    result_.z = Algorithm::clamp(_array[2_z].as_number(), 0.0, 360.0);
    return true;
  };

  Math::Transform transform;
  if (scale && !parse_vec3(scale, "scale", transform.scale)) {
    return false;
  }

  if (rotate && !parse_vec3(rotate, "rotate", transform.rotate)) {
    return false;
  }

  if (translate && !parse_vec3(translate, "translate", transform.translate)) {
    return false;
  }

  m_transform = transform;
  return true;
}

void Loader::write_log(Log::Level _level, String&& message_) const {
  if (m_name.is_empty()) {
    logger->write(_level, "%s", Utility::move(message_));
  } else {
    logger->write(_level, "%s: %s", m_name, Utility::move(message_));
  }
}

} // namespace Rx::Model
