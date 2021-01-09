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

Skybox::Skybox(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("skybox")}
  , m_texture{nullptr}
{
}

Skybox::~Skybox() {
  m_frontend->destroy_texture(RX_RENDER_TAG("skybox"), m_texture);
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

  Frontend::Program* program = m_technique->permute(_grading ? 1 << 0 : 0);

  Frontend::State state;
  state.depth.record_test(true);
  state.depth.record_write(true);

  state.blend.record_enable(false);
  state.cull.record_enable(false);

  state.viewport.record_dimensions(_target->dimensions());

  // Record all uniforms and textures.
  Frontend::Textures draw_textures;
  program->uniforms()[0].record_mat4x4f(Math::Mat4x4f::invert(_projection));
  program->uniforms()[1].record_mat4x4f(Math::Mat4x4f::invert(view));
  program->uniforms()[2].record_sampler(draw_textures.add(m_texture));
  if (_grading) {
    program->uniforms()[3].record_sampler(draw_textures.add(_grading->atlas()->texture()));
    program->uniforms()[4].record_vec2f(_grading->properties());
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
  Concurrency::ThreadPool::instance().add([=](int) {
    load(_file_name, _max_face_dimensions);
  });
}

bool Skybox::load(const String& _file_name, const Math::Vec2z& _max_face_dimensions) {
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file, _max_face_dimensions);
  }
  return false;
}

bool Skybox::load(Stream* _stream, const Math::Vec2z& _max_face_dimensions) {
  auto data = read_text_stream(Memory::SystemAllocator::instance(), _stream);
  if (!data) {
    return false;
  }

  auto disown = data->disown();
  if (!disown) {
    return false;
  }

  const JSON description{*disown};
  if (!description) {
    // could not parse json
    return false;
  }

  if (!description.is_object()) {
    // expected object
    return false;
  }

  const auto& name{description["name"]};
  if (!name || !name.is_string()) {
    return false;
  }

  const auto& faces{description["faces"]};
  if (!faces || !faces.is_array_of(JSON::Type::STRING, 6)) {
    return false;
  }

  auto texture = m_frontend->create_textureCM(RX_RENDER_TAG("skybox"));
  if (!texture) {
    return false;
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
  bool result = faces.each([&](const JSON& file_name) {
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

    if (loader.format() != Texture::PixelFormat::RGBA_U8) {
      // Convert everything to RGB8 if not already.
      auto data = Texture::convert(
        m_frontend->allocator(),
        loader.data().data(),
        loader.dimensions().area(),
        loader.format(),
        Texture::PixelFormat::RGBA_U8);
      if (!data) {
        return false;
      }
      texture->write(data->data(), face, 0);
    } else {
      texture->write(loader.data().data(), face, 0);
    }
    face = static_cast<Frontend::TextureCM::Face>(static_cast<Size>(face) + 1);
    return true;
  });

  if (!result) {
    return false;
  }

  m_frontend->initialize_texture(RX_RENDER_TAG("skybox"), texture);

  m_lock.lock();
  m_frontend->destroy_texture(RX_RENDER_TAG("skybox"), m_texture);
  m_texture = texture;
  m_name = name.as_string();
  m_lock.unlock();

  return false;
}

} // namespace Rx::Render
