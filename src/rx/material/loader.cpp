#include "rx/core/filesystem/file.h"
#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/wait_group.h"
#include "rx/core/json.h"
#include "rx/core/algorithm/clamp.h"

#include "rx/material/loader.h"

namespace Rx::Material {

RX_LOG("material/loader", logger);

Loader::Loader(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_textures{allocator()}
  , m_name{allocator()}
  , m_flags{0}
  , m_roughness{1.0f}
  , m_metalness{0.0f}
  , m_occlusion{1.0f}
  , m_albedo{1.0f, 1.0f, 1.0f}
  , m_emission{0.0f, 0.0f, 0.0f}
{
}

Loader::Loader(Loader&& loader_)
  : m_allocator{&loader_.allocator()}
  , m_textures{Utility::move(loader_.m_textures)}
  , m_name{Utility::move(loader_.m_name)}
  , m_flags{Utility::exchange(loader_.m_flags, 0)}
  , m_roughness{Utility::exchange(loader_.m_roughness, 1.0f)}
  , m_metalness{Utility::exchange(loader_.m_metalness, 0.0f)}
  , m_occlusion{Utility::exchange(loader_.m_occlusion, 1.0f)}
  , m_albedo{Utility::exchange(loader_.m_albedo, {1.0f, 1.0f, 1.0f})}
  , m_emission{Utility::exchange(loader_.m_emission, {0.0f, 0.0f, 0.0f})}
{
}
void Loader::operator=(Loader&& loader_) {
  RX_ASSERT(&loader_ != this, "self assignment");

  m_allocator = &loader_.allocator();
  m_textures = Utility::move(loader_.m_textures);
  m_name = Utility::move(loader_.m_name);
  m_flags = Utility::exchange(loader_.m_flags, 0);
  m_roughness = Utility::exchange(loader_.m_roughness, 1.0f);
  m_metalness = Utility::exchange(loader_.m_metalness, 0.0f);
  m_occlusion = Utility::exchange(loader_.m_occlusion, 1.0f);
  m_albedo = Utility::exchange(loader_.m_albedo, {1.0f, 1.0f, 1.0f});
  m_emission = Utility::exchange(loader_.m_emission, {0.0f, 0.0f, 0.0f});
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

  if (const auto& roughness = _definition["roughness"]) {
    if (!roughness.is_number()) {
      return error("expected Number for 'roughness'");
    }
    m_roughness = Algorithm::clamp(roughness.as_float(), 0.0f, 1.0f);
  }

  if (const auto& metalness = _definition["metalness"]) {
    if (!metalness.is_number()) {
      return error("expected Number for 'metalness'");
    }
    m_metalness = Algorithm::clamp(metalness.as_float(), 0.0f, 1.0f);
  }

  if (const auto& occlusion = _definition["occlusion"]) {
    if (!occlusion.is_number()) {
      return error("expected Number for 'occlusion'");
    }
    m_occlusion = Algorithm::clamp(occlusion.as_float(), 0.0f, 1.0f);
  }

  if (const auto& emission = _definition["emission"]) {
    if (!emission.is_array_of(JSON::Type::NUMBER, 3)) {
      return error("expected Array[Number, 3] for 'emission'");
    }
    m_emission.r = Algorithm::clamp(emission[0_z].as_float(), 0.0f, 1.0f);
    m_emission.g = Algorithm::clamp(emission[1_z].as_float(), 0.0f, 1.0f);
    m_emission.b = Algorithm::clamp(emission[2_z].as_float(), 0.0f, 1.0f);
  }

  if (const auto& albedo = _definition["albedo"]) {
    if (!albedo.is_array_of(JSON::Type::NUMBER, 3)) {
      return error("expected Array[Number, 3] for 'emission'");
    }
    m_albedo.r = Algorithm::clamp(albedo[0_z].as_float(), 0.0f, 1.0f);
    m_albedo.g = Algorithm::clamp(albedo[1_z].as_float(), 0.0f, 1.0f);
    m_albedo.b = Algorithm::clamp(albedo[2_z].as_float(), 0.0f, 1.0f);
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

    auto parse{[&](const auto& _vec, const char* _tag, Math::Vec3f& result_) {
      if (!_vec.is_array_of(JSON::Type::NUMBER, 2)) {
        return error("expected Array[Number, 2] for '%s'", _tag);
      }
      result_.x = _vec[0_z].as_float();
      result_.y = _vec[1_z].as_float();
      return true;
    }};

    Math::Transform xform;
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

  if (const auto& textures = _definition["textures"]) {
    if (!textures.is_array_of(JSON::Type::OBJECT)) {
      return error("expected Array[Object] for 'textures'");
    }

    if (!m_textures.reserve(textures.size()) || !parse_textures(textures)) {
      return false;
    }
  }

  // Determine if the diffuse texture has an alpha channel.
  m_textures.each_fwd([this](const Texture& _texture) {
    if (_texture.type() != "albedo") {
      return true;
    }

    if (_texture.chain().bpp() == 4) {
      m_flags |= k_has_alpha;
    } else if (m_flags & k_alpha_test) {
      logger->warning("'alpha_test' disabled (\"albedo\" has no alpha channel)");
      m_flags &= ~k_alpha_test;
    }
    return false;
  });

  return true;
}

bool Loader::parse_textures(const JSON& _textures) {
  return _textures.each([&](const JSON& _texture) {
    Texture new_texture{allocator()};
    if (_texture.is_string() && new_texture.load(_texture.as_string())) {
      return m_textures.push_back(Utility::move(new_texture));
    } else if (_texture.is_object() && new_texture.parse(_texture)) {
      return m_textures.push_back(Utility::move(new_texture));
    } else {
      return false;
    }
  });
}

void Loader::write_log(Log::Level _level, String&& message_) const {
  if (m_name.is_empty()) {
    logger->write(_level, "%s", Utility::move(message_));
  } else {
    logger->write(_level, "%s: %s", m_name, Utility::move(message_));
  }
}

} // namespace Rx::Material
