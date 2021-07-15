#include "rx/model/loader.h"
#include "rx/model/iqm.h"
#include "rx/model/obj.h"
#include "rx/model/skeleton.h"

#include "rx/core/map.h"
#include "rx/core/set.h"
#include "rx/core/ptr.h"
#include "rx/core/serialize/json.h"
#include "rx/core/filesystem/unbuffered_file.h"
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
  , m_clips{allocator()}
  , m_materials{allocator()}
  , m_name{allocator()}
  , m_flags{0}
  , m_report{allocator(), *logger}
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

bool Loader::load(Concurrency::Scheduler& _scheduler, Stream::Context& _stream) {
  if (auto contents = _stream.read_text(allocator())) {
    if (auto disown = contents->disown()) {
      if (auto json = Serialize::JSON::parse(allocator(), String{*disown})) {
        return parse(_scheduler, *json);
      }
    }
  }
  return false;
}

bool Loader::load(Concurrency::Scheduler& _scheduler, const StringView& _file_name) {
  if (auto file = Filesystem::UnbufferedFile::open(allocator(), _file_name, "r")) {
    return load(_scheduler, *file);
  }
  return false;
}

bool Loader::parse(Concurrency::Scheduler& _scheduler, const Serialize::JSON& _definition) {
  if (!_definition) {
    const auto json_error{_definition.error()};
    if (json_error) {
      return m_report.error("%s", *json_error);
    } else {
      return m_report.error("empty definition");
    }
  }

  if (!_definition.is_object()) {
    return m_report.error("expected Object");
  }

  const auto& name = _definition["name"];
  const auto& file = _definition["file"];
  const auto& materials = _definition["materials"];
  const auto& transform = _definition["transform"];

  if (!name) {
    return m_report.error("missing 'name'");
  }

  if (!name.is_string()) {
    return m_report.error("expected String for 'name'");
  }

  auto name_string = name.as_string(allocator());
  if (!name_string) {
    return false;
  }

  m_name = Utility::move(*name_string);

  if (!m_report.rename(m_name)) {
    return false;
  }

  if (!file) {
    return m_report.error("missing 'file'");
  }

  if (!file.is_string()) {
    return m_report.error("expected String for 'file'");
  }

  if (!materials) {
    return m_report.error("missing 'materials'");
  }

  if (!materials.is_array_of(Serialize::JSON::Type::OBJECT)
    && !materials.is_array_of(Serialize::JSON::Type::STRING))
  {
    return m_report.error("expected Array[Object] or Array[String] for 'materials'");
  }

  m_transform = nullopt;
  if (transform && !parse_transform(transform)) {
    return false;
  }

  // Clear incase we're being run multiple times to change.
  m_materials.clear();

  auto file_name = file.as_string(allocator());
  if (!file_name) {
    return false;
  }

  if (!import(*file_name)) {
    return false;
  }

  // Load all the materials across multiple threads.
  Concurrency::AtomicFlag error = false;
  Concurrency::Mutex mutex;
  Concurrency::WaitGroup group{materials.size()};
  materials.each([&](const Serialize::JSON& _material) {
    (void)_scheduler.add([&, _material](int) {
      Material::Loader loader{allocator()};
      if (_material.is_string()) {
        if (auto file = _material.as_string(allocator()); file && loader.load(*file)) {
          Concurrency::ScopeLock lock{mutex};
          if (!m_materials.insert(loader.name(), Utility::move(loader))) {
            error.test_and_set();
          }
        } else {
          error.test_and_set();
        }
      } else if (_material.is_object() && loader.parse(_material)) {
        Concurrency::ScopeLock lock{mutex};
        if (!m_materials.insert(loader.name(), Utility::move(loader))) {
          error.test_and_set();
        }
      } else {
        error.test_and_set();
      }
      group.signal();
    });
  });
  group.wait();

  // Validate the materials.
  return !error.test() && validate();
}

bool Loader::import(const StringView& _file_name) {
  Ptr<Importer> new_loader;

  // determine the model format based on the extension
  if (_file_name.ends_with(".iqm")) {
    new_loader = make_ptr<IQM>(allocator(), allocator());
  } else if (_file_name.ends_with(".obj")) {
    new_loader = make_ptr<OBJ>(allocator(), allocator());
  } else {
    return m_report.error("unsupported model format");
  }

  if (!new_loader) {
    return m_report.error("out of memory");
  }

  const bool result = new_loader->load(_file_name);
  if (!result) {
    return m_report.error("failed to import");
  }

  // We may possibly call import multiple times.
  destroy();

  m_meshes = Utility::move(new_loader->meshes());
  m_elements = Utility::move(new_loader->elements());
  m_clips = Utility::move(new_loader->clips());
  m_skeleton = Utility::move(new_loader->skeleton());

  const auto& positions = new_loader->positions();
  const auto& normals = new_loader->normals();
  const auto& tangents = new_loader->tangents();
  const auto& coordinates = new_loader->coordinates();
  const auto& occlusions = new_loader->occlusions();

  const Size n_vertices = positions.size();

  if (m_clips.is_empty()) {
    Utility::construct<Vector<Vertex>>(&as_vertices, allocator());
    m_flags = CONSTRUCTED;
    if (!as_vertices.resize(n_vertices)) {
      return m_report.error("out of memory");
    }
  } else {
    Utility::construct<Vector<AnimatedVertex>>(&as_animated_vertices, allocator());
    m_flags = CONSTRUCTED | ANIMATED;
    if (!as_animated_vertices.resize(n_vertices)) {
      return m_report.error("out of memory");
    }
  }

  // Hoist the transform check outside the for loops for faster model loading.
  if (m_transform) {
    const auto transform = m_transform->as_mat4();
    if (m_clips.is_empty()) {
      for (Size i{0}; i < n_vertices; i++) {
        const Math::Vec3f tangent{Math::transform_vector({tangents[i].x, tangents[i].y, tangents[i].z}, transform)};
        as_vertices[i].position = Math::transform_point(positions[i], transform);
        as_vertices[i].normal = Math::transform_vector(normals[i], transform);
        as_vertices[i].tangent = {tangent.x, tangent.y, tangent.z, tangents[i].w};
        as_vertices[i].coordinate = coordinates[i];
        as_vertices[i].occlusion = occlusions[i];
      }
    } else {
      const auto& blend_weights{new_loader->blend_weights()};
      const auto& blend_indices{new_loader->blend_indices()};
      for (Size i{0}; i < n_vertices; i++) {
        const Math::Vec3f tangent{Math::transform_vector({tangents[i].x, tangents[i].y, tangents[i].z}, transform)};
        as_animated_vertices[i].position = Math::transform_point(positions[i], transform);
        as_animated_vertices[i].normal = Math::transform_vector(normals[i], transform);
        as_animated_vertices[i].tangent = {tangent.x, tangent.y, tangent.z, tangents[i].w};
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weights = blend_weights[i];
        as_animated_vertices[i].blend_indices = blend_indices[i];
        as_animated_vertices[i].occlusion = occlusions[i];
      }
    }

    const Math::Mat3x4f& xform{
      {transform.x.x, transform.y.x, transform.z.x, transform.w.x},
      {transform.x.y, transform.y.y, transform.z.y, transform.w.y},
      {transform.x.z, transform.y.z, transform.z.z, transform.w.z}};

    if (m_skeleton) {
      m_skeleton->transform(xform);
    }
  } else {
    if (m_clips.is_empty()) {
      for (Size i = 0; i < n_vertices; i++) {
        as_vertices[i].position = positions[i];
        as_vertices[i].normal = normals[i];
        as_vertices[i].tangent = tangents[i];
        as_vertices[i].coordinate = coordinates[i];
        as_vertices[i].occlusion = occlusions[i];
      }
    } else {
      const auto& blend_weights = new_loader->blend_weights();
      const auto& blend_indices = new_loader->blend_indices();
      for (Size i = 0; i < n_vertices; i++) {
        as_animated_vertices[i].position = positions[i];
        as_animated_vertices[i].normal = normals[i];
        as_animated_vertices[i].tangent = tangents[i];
        as_animated_vertices[i].coordinate = coordinates[i];
        as_animated_vertices[i].blend_weights = blend_weights[i];
        as_animated_vertices[i].blend_indices = blend_indices[i];
        as_animated_vertices[i].occlusion = occlusions[i];
      }
    }
  }

  // Bounds need to be transformed when we have a transform.
  if (m_transform) {
    const auto& xform = m_transform->as_mat4();
    m_meshes.each_fwd([&](Mesh& _mesh) {
      _mesh.bounds.each_fwd([&](Vector<Math::AABB>& aabbs_) {
        aabbs_.each_fwd([&](Math::AABB& aabb_) {
          aabb_ = aabb_.transform(xform);
        });
      });
    });
  }

  return true;
}

bool Loader::parse_transform(const Serialize::JSON& _transform) {
  const auto& scale = _transform["scale"];
  const auto& rotate = _transform["rotate"];
  const auto& translate = _transform["translate"];

  auto parse_vec3 = [&](const Serialize::JSON& _array, const char* _tag, Math::Vec3f& result_) -> bool {
    if (!_array.is_array_of(Serialize::JSON::Type::NUMBER, 3)) {
      return m_report.error("expected Array[Number, 3] for '%s'", _tag);
    }
    result_.x = Algorithm::clamp(_array[0_z].as_number(), -180.0, 180.0);
    result_.y = Algorithm::clamp(_array[1_z].as_number(), -180.0, 180.0);
    result_.z = Algorithm::clamp(_array[2_z].as_number(), -180.0, 180.0);
    return true;
  };

  Math::Transform transform;
  if (scale && !parse_vec3(scale, "scale", transform.scale)) {
    return false;
  }

  Math::Vec3f euler;
  if (rotate && !parse_vec3(rotate, "rotate", euler)) {
    return false;
  }

  transform.rotation = Math::Mat3x3f::rotate(euler);

  if (translate && !parse_vec3(translate, "translate", transform.translate)) {
    return false;
  }

  m_transform = transform;

  return true;
}

bool Loader::validate() {
  // Validate that each mesh has a material and is referenced.
  Set<String> referenced{m_allocator};
  auto check_mesh = [&](const Mesh& _mesh) -> bool {
    if (!m_materials.find(_mesh.material)) {
      return m_report.error("missing material \"%s\" for mesh \"%s\"",
        _mesh.material, _mesh.name);
    }
    return referenced.insert(_mesh.material);
  };

  if (!m_meshes.each_fwd(check_mesh)) {
    return false;
  }

  // Validate each material is referenced.
  auto check_material = [&](const String& _name) -> bool {
    if (!referenced.find(_name)) {
      return m_report.error("material \"%s\" is not referenced by any mesh", _name);
    }
    return true;
  };

  if (!m_materials.each_key(check_material)) {
    return false;
  }

  return true;
}

} // namespace Rx::Model
