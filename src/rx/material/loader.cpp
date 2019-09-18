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
  , m_alpha_test{false}
  , m_has_alpha{false}
{
}

loader::loader(loader&& _loader)
  : m_allocator{_loader.m_allocator}
  , m_textures{utility::move(_loader.m_textures)}
  , m_name{utility::move(_loader.m_name)}
  , m_alpha_test{_loader.m_alpha_test}
  , m_has_alpha{_loader.m_has_alpha}
{
  _loader.m_allocator = nullptr;
  _loader.m_alpha_test = false;
  _loader.m_has_alpha = false;
}

void loader::operator=(loader&& _loader) {
  m_allocator = _loader.m_allocator;
  m_textures = utility::move(_loader.m_textures);
  m_name = utility::move(_loader.m_name);
  m_alpha_test = _loader.m_alpha_test;
  m_has_alpha = _loader.m_has_alpha;

  _loader.m_allocator = nullptr;
  _loader.m_alpha_test = false;
  _loader.m_has_alpha = false;
}

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

  m_alpha_test = alpha_test ? alpha_test.as_boolean() : false;

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

    if (scale && !parse(scale, "scale", m_transform.scale)) {
      return false;
    }

    if (rotate && !parse(rotate, "rotate", m_transform.rotate)) {
      return false;
    }

    if (translate && !parse(translate, "translate", m_transform.translate)) {
      return false;
    }

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
      m_has_alpha = true;
    } else if (m_alpha_test) {
      logger(log::level::k_warning, "'alpha_test' disabled (\"diffuse\" has no alpha channel)");
      m_alpha_test = false;
    }
    return false;
  });

  return true;
}

bool loader::parse_textures(const json& _textures) {
  const rx_size n_textures{_textures.size()};
  concurrency::thread_pool threads{m_allocator, n_textures};
  concurrency::wait_group group;
  concurrency::atomic<bool> success{true};
  _textures.each([&](const json& _texture) {
    threads.add([&, _texture](int) {
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
  group.wait(n_textures);
  return success.load();
}

void loader::write_log(log::level _level, string&& _message) const {
  if (m_name.is_empty()) {
    logger(_level, "%s", utility::move(_message));
  } else {
    logger(_level, "%s: %s", m_name, utility::move(_message));
  }
}

} // namespace rx::material