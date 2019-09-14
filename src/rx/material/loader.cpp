#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"

#include "rx/material/loader.h"

namespace rx::material {

RX_LOG("material/loader", logger);

loader::loader(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_textures{_allocator}
  , m_name{_allocator}
  , m_alpha_test{false}
{
}

loader::loader(loader&& _loader)
  : m_allocator{_loader.m_allocator}
  , m_textures{utility::move(_loader.m_textures)}
  , m_name{utility::move(_loader.m_name)}
  , m_alpha_test{_loader.m_alpha_test}
{
  _loader.m_allocator = nullptr;
  _loader.m_alpha_test = false;
}

void loader::operator=(loader&& _loader) {
  m_allocator = _loader.m_allocator;
  m_textures = utility::move(_loader.m_textures);
  m_name = utility::move(_loader.m_name);
  m_alpha_test = _loader.m_alpha_test;

  _loader.m_allocator = nullptr;
  _loader.m_alpha_test = false;
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

  const auto& textures{_definition["textures"]};
  if (!textures) {
    return error("missing 'textures'");
  }

  if (!textures.is_array_of(json::type::k_object)) {
    return error("expected Array[Object] for 'textures'");
  }

  m_textures.reserve(textures.size());
  return textures.each([this](const json& _texture) {
    texture new_texture{m_allocator};
    if (new_texture.parse(_texture)) {
      m_textures.push_back(utility::move(new_texture));
    }
  });
}

void loader::write_log(log::level _level, string&& _message) const {
  if (m_name.is_empty()) {
    logger(_level, "%s", utility::move(_message));
  } else {
    logger(_level, "%s: %s", m_name, utility::move(_message));
  }
}

} // namespace rx::material