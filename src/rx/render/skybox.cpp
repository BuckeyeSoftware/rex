#include "rx/render/skybox.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/math/vec3.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"
#include "rx/core/profiler.h"
#include "rx/core/concurrency/thread_pool.h"

#include "rx/texture/loader.h"
#include "rx/texture/convert.h"

namespace Rx::Render {

Skybox::Skybox(Skybox&& skybox_) {
  Concurrency::ScopeLock lock{skybox_.m_lock};
  m_frontend = Utility::exchange(skybox_.m_frontend, nullptr);
  m_technique = Utility::exchange(skybox_.m_technique, nullptr);
  m_texture.store(skybox_.m_texture.exchange(nullptr));
  m_name = Utility::move(skybox_.m_name);
}

Skybox& Skybox::operator=(Skybox&& skybox_) {
  if (this != &skybox_) {
    Concurrency::ScopeLock this_lock{m_lock};
    Concurrency::ScopeLock that_lock{skybox_.m_lock};

    release();

    m_frontend = Utility::exchange(skybox_.m_frontend, nullptr);
    m_technique = Utility::exchange(skybox_.m_technique, nullptr);
    m_texture.store(skybox_.m_texture.exchange(nullptr));
    m_name = Utility::move(skybox_.m_name);
  }

  return *this;
}

Optional<Skybox> Skybox::create(Frontend::Context* _frontend) {
  auto technique = _frontend->find_technique_by_name("skybox");
  if (!technique) {
    return nullopt;
  }
  return Skybox{_frontend, technique};
}

void Skybox::render(Frontend::Target* _target, const Math::Mat4x4f& _view,
  const Math::Mat4x4f& _projection, const ColorGrader::Entry* _grading)
{
  RX_PROFILE_CPU("skybox::render");

  if (!m_texture) {
    return;
  }

  // Eliminate translation from the view matrix.
  Math::Mat4x4f view{_view};
  view.w = {0.0f, 0.0f, 0.0f, 1.0f};

  bool is_hdri = false;
  if (m_texture.load()->resource_type() == Frontend::Resource::Type::TEXTURE2D) {
    is_hdri = true;
  }

  Frontend::Program* program =
    m_technique->configuration(is_hdri ? 1 : 0).permute(_grading ? 1 << 0 : 0);

  Frontend::State state;
  state.depth.record_test(true);
  state.depth.record_write(true);

  state.blend.record_enable(false);
  state.cull.record_enable(false);

  state.viewport.record_dimensions(_target->dimensions());

  // Record all uniforms and textures.
  Frontend::Textures draw_textures;
  program->uniforms()[0].record_mat4x4f(Math::invert(_projection));
  program->uniforms()[1].record_mat4x4f(Math::invert(view));
  program->uniforms()[is_hdri ? 2 : 3].record_sampler(draw_textures.add(m_texture));
  if (_grading) {
    program->uniforms()[4].record_sampler(draw_textures.add(_grading->atlas()->texture()));
    program->uniforms()[5].record_vec2f(_grading->properties());
  }


  // Record all draw buffers.
  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  m_frontend->draw(
    RX_RENDER_TAG("skybox"),
    state,
    _target,
    draw_buffers,
    nullptr,
    program,
    3,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    draw_textures);
}

void Skybox::load_async(const String& _file_name, const Math::Vec2z& _max_face_dimensions) {
  Concurrency::ThreadPool::instance().add([=, this](int) {
    // TODO(dweiler): Promise<bool> for status.
    (void)load(_file_name, _max_face_dimensions);
  });
}

bool Skybox::load(const String& _file_name, const Math::Vec2z& _max_face_dimensions) {
  if (auto file = Filesystem::File::open(_file_name, "r")) {
    return load(*file, _max_face_dimensions);
  }
  return false;
}

bool Skybox::load(Stream& _stream, const Math::Vec2z& _max_face_dimensions) {
  auto& allocator = m_frontend->allocator();

  auto data = _stream.read_text(allocator);
  if (!data) {
    return false;
  }

  auto disown = data->disown();
  if (!disown) {
    return false;
  }

  auto parse = JSON::parse(allocator, *disown);
  if (!parse) {
    // Out of memory.
    return false;
  }

  auto& description = *parse;

  if (!description) {
    // Could not parse json.
    return false;
  }

  if (!description.is_object()) {
    // Expected object.
    return false;
  }

  const auto& name = description["name"];
  if (!name || !name.is_string()) {
    return false;
  }

  const auto& faces = description["faces"];
  const auto& hdri = description["hdri"];
  // Can only be one of these.
  if (faces && hdri) {
    return false;
  }

  Frontend::Texture* new_texture = nullptr;
  if (faces) {
    if (!faces.is_array_of(JSON::Type::STRING, 6)) {
      return false;
    }
    new_texture = create_cubemap(faces, _max_face_dimensions);
  } else if (hdri) {
    if (!hdri.is_string()) {
      return false;
    }
    new_texture = create_hdri(hdri);
  } else {
    return false;
  }

  if (!new_texture) {
    return false;
  }

  // While locked.
  {
    Concurrency::ScopeLock lock{m_lock};
    release();
    m_texture = new_texture;
    m_name = name.as_string();
  }

  return true;
}

Frontend::TextureCM* Skybox::create_cubemap(const JSON& _faces,
  const Math::Vec2z& _max_face_dimensions) const
{
  auto texture = m_frontend->create_textureCM(RX_RENDER_TAG("skybox"));
  if (!texture) {
    return nullptr;
  }

  texture->record_type(Frontend::Texture::Type::STATIC);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_levels(1);
  texture->record_filter({false, false, false});
  texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                        Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                        Frontend::Texture::WrapType::CLAMP_TO_EDGE});

  Math::Vec2z dimensions;
  Frontend::TextureCM::Face face{Frontend::TextureCM::Face::RIGHT};
  bool result = _faces.each([&](const JSON& file_name) {
    Texture::Loader loader{m_frontend->allocator()};
    if (!loader.load(file_name.as_string(), Texture::PixelFormat::RGBA_U8, _max_face_dimensions)) {
      return false;
    }
    if (dimensions.is_all(0)) {
      dimensions = loader.dimensions();
      texture->record_dimensions(dimensions);
    } else if (dimensions != loader.dimensions()) {
      return false;
    }
    texture->write(loader.data().data(), face, 0);
    face = static_cast<Frontend::TextureCM::Face>(static_cast<Size>(face) + 1);
    return true;
  });

  if (!result) {
    return nullptr;
  }

  m_frontend->initialize_texture(RX_RENDER_TAG("skybox"), texture);

  return texture;
}

Frontend::Texture2D* Skybox::create_hdri(const JSON& _json) const {
  auto texture = m_frontend->create_texture2D(RX_RENDER_TAG("skybox"));
  if (!texture) {
    return nullptr;
  }

  texture->record_type(Frontend::Texture::Type::STATIC);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_F32);
  texture->record_levels(1);
  texture->record_filter({false, false, false});
  texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                        Frontend::Texture::WrapType::CLAMP_TO_EDGE});

  Texture::Loader loader{m_frontend->allocator()};
  if (!loader.load(_json.as_string(), Texture::PixelFormat::RGBA_F32, {4096, 4096})) {
    return nullptr;
  }

  texture->record_dimensions(loader.dimensions());
  texture->write(loader.data().data(), 0);

  m_frontend->initialize_texture(RX_RENDER_TAG("skybox"), texture);

  return texture;
}

void Skybox::release() {
  if (!m_texture) {
    return;
  }

  auto texture = m_texture.load();

  switch (texture->resource_type()) {
  default:
    break;
  case Frontend::Resource::Type::TEXTURE2D:
    m_frontend->destroy_texture(RX_RENDER_TAG("skybox"),
      static_cast<Frontend::Texture2D*>(texture));
    break;
  case Frontend::Resource::Type::TEXTURECM:
    m_frontend->destroy_texture(RX_RENDER_TAG("skybox"),
      static_cast<Frontend::TextureCM*>(texture));
    break;
  }
}

} // namespace Rx::Render
