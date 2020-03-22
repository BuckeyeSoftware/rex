#include "rx/core/filesystem/file.h"
#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/wait_group.h"
#include "rx/core/json.h"

#include "rx/material/loader.h"

namespace rx::material {

RX_LOG("material/loader", logger);

loader::loader(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_textures{_allocator}
  , m_name{_allocator}
  , m_flags{0}
  , m_roughness{1.0f}
  , m_metalness{0.0f}
{
}

loader::loader(loader&& loader_)
  : m_allocator{utility::exchange(loader_.m_allocator, nullptr)}
  , m_textures{utility::move(loader_.m_textures)}
  , m_name{utility::move(loader_.m_name)}
  , m_flags{utility::exchange(loader_.m_flags, 0)}
  , m_roughness{utility::exchange(loader_.m_roughness, 1.0f)}
  , m_metalness{utility::exchange(loader_.m_metalness, 0.0f)}
{
}

void loader::operator=(loader&& loader_) {
  RX_ASSERT(&loader_ != this, "self assignment");

  m_allocator = utility::exchange(loader_.m_allocator, nullptr);
  m_textures = utility::move(loader_.m_textures);
  m_name = utility::move(loader_.m_name);
  m_flags = utility::exchange(loader_.m_flags, 0);
  m_roughness = utility::exchange(loader_.m_roughness, 1.0f);
  m_metalness = utility::exchange(loader_.m_metalness, 0.0f);
}

bool loader::load(stream* _stream) {
  if (auto contents = read_text_stream(m_allocator, _stream)) {
    return parse({contents->disown()});
  }
  return false;
}

bool loader::load(const string& _file_name) {
  if (filesystem::file file{_file_name, "rb"}) {
    return load(&file);
  }
  return false;
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

  const auto& name{_definition["name"]};
  if (!name) {
    return error("missing 'name'");
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  m_name = name.as_string();

  const auto& alpha_test{_definition["alpha_test"]};
  if (alpha_test && !alpha_test.is_boolean()) {
    return error("expected Boolean for 'alpha_test'");
  }

  if (alpha_test && alpha_test.as_boolean()) {
    m_flags |= k_alpha_test;
  }

  const auto& no_compress{_definition["no_compress"]};
  if (no_compress && !no_compress.is_boolean()) {
    return error("expected Boolean for 'no_compress'");
  }

  if (no_compress && no_compress.as_boolean()) {
    m_flags |= k_no_compress;
  }

  if (const auto& transform{_definition["transform"]}) {
    const auto& scale{transform["scale"]};
    const auto& rotate{transform["rotate"]};
    const auto& translate{transform["translate"]};

    auto parse{[&](const auto& _vec, const char* _tag, math::vec3f& result_) {
      if (!_vec.is_array_of(json::type::k_number) || _vec.size() != 2) {
        return error("expected Array[Number, 2] for '%s'", _tag);
      }
      result_.x = _vec[0_z].as_float();
      result_.y = _vec[1_z].as_float();
      return true;
    }};

    math::transform xform;
    if (scale && !parse(scale, "scale", xform.scale)) {
      return false;
    }

    if (rotate && !parse(rotate, "rotate", xform.rotate)) {
      return false;
    }

    if (translate && !parse(translate, "translate", xform.translate)) {
      return false;
    }

    m_transform = xform;

    return true;
  }

  const auto& textures{_definition["textures"]};
  if (!textures) {
    return error("missing 'textures'");
  }

  if (!textures.is_array_of(json::type::k_object)) {
    return error("expected Array[Object] for 'textures'");
  }

  m_textures.reserve(textures.size());
  if (!parse_textures(textures)) {
    return false;
  }

  // Determine if the diffuse texture has an alpha channel.
  m_textures.each_fwd([this](const texture& _texture) {
    if (_texture.type() != "diffuse") {
      return true;
    }

    if (_texture.chain().bpp() == 4) {
      m_flags |= k_has_alpha;
    } else if (m_flags & k_alpha_test) {
      logger->warning("'alpha_test' disabled (\"diffuse\" has no alpha channel)");
      m_flags &= ~k_alpha_test;
    }
    return false;
  });

  return true;
}

bool loader::parse_textures(const json& _textures) {
  bool success{true};
  _textures.each([&](const json& _texture) {
    texture new_texture{m_allocator};
    if (_texture.is_string() && new_texture.load(_texture.as_string())) {
      m_textures.push_back(utility::move(new_texture));
    } else if (_texture.is_object() && new_texture.parse(_texture)) {
      m_textures.push_back(utility::move(new_texture));
    } else {
      success = false;
    }
  });
  return success;


  // Revisit with wait groups
#if 0
  const rx_size n_textures{_textures.size()};
  // concurrency::thread_pool threads{m_allocator, n_textures};
  concurrency::wait_group group{n_textures};
  concurrency::atomic<bool> success{true};
  _textures.each([&](const json& _texture) {
    concurrency::thread_pool::instance().add([&, _texture](int) {
      texture new_texture{m_allocator};
      if (_texture.is_string() && new_texture.load(_texture.as_string())) {
        m_textures.push_back(utility::move(new_texture));
      } else if (_texture.is_object() && new_texture.parse(_texture)) {
        m_textures.push_back(utility::move(new_texture));
      } else {
        success = false;
      }
      group.signal();
    });
    return true;
  });
  group.wait();
  return success.load();
#endif
}

void loader::write_log(log::level _level, string&& message_) const {
  if (m_name.is_empty()) {
    logger->write(_level, "%s", utility::move(message_));
  } else {
    logger->write(_level, "%s: %s", m_name, utility::move(message_));
  }
}

} // namespace rx::material
