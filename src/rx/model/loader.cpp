#include "rx/model/loader.h"
#include "rx/model/iqm.h"

#include "rx/core/map.h"
#include "rx/core/filesystem/file.h"
#include "rx/core/concurrency/thread_pool.h"

#include "rx/material/loader.h"

namespace rx::model {

RX_LOG("model/loader", logger);

bool loader::load(const string& _file_name) {
  auto contents{filesystem::read_binary_file(m_allocator, _file_name)};
  if (!contents) {
    return false;
  }
  return parse({contents->disown()});
}

bool loader::parse(const json& _definition) {
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

  const auto& name{_definition["name"]};
  const auto& file{_definition["file"]};
  const auto& materials{_definition["materials"]};

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

  const auto& file_name{file.as_string()};

  if (!import(file.as_string())) {
    return false;
  }

  // Load all the materials across multiple threads.
  concurrency::thread_pool pool{m_allocator, 32};
  concurrency::mutex mutex;
  concurrency::condition_variable condition_variable;
  rx_size count{0};
  materials.each([&](const json& _material) {
    pool.add([&, _material](int) {
      material::loader loader{m_allocator};
      if (loader.parse(_material)) {
        concurrency::scope_lock lock{mutex};
        m_materials.insert(loader.name(), utility::move(loader));
      }
      {
        concurrency::scope_lock lock{mutex};
        count++;
        condition_variable.signal();
      }
    });
  });
  {
    concurrency::scope_lock lock{mutex};
    condition_variable.wait(lock, [&]{ return count == materials.size(); });
  }

  return true;
}

bool loader::import(const string& _file_name) {
  RX_ASSERT(!(m_flags & k_constructed), "already imported");

  importer* new_loader{nullptr};

  // determine the model format based on the extension
  if (_file_name.ends_with(".iqm")) {
    new_loader = m_allocator->create<iqm>(m_allocator);
  } else {
    return error("unsupported model format");
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

  m_allocator->destroy<importer>(new_loader);
  return result;
}

void loader::write_log(log::level _level, string&& _message) const {
  if (m_name.is_empty()) {
    logger(_level, "%s", utility::move(_message));
  } else {
    logger(_level, "%s: %s", m_name, utility::move(_message));
  }
}

} // namespace rx::model