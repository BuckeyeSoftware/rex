#include <stddef.h> // offsetof
#include <string.h> // memcpy

#include "rx/render/canvas.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/technique.h"

#include "rx/texture/chain.h"

#include "rx/core/log.h"

#include "lib/nanovg.h"

namespace Rx::Render {

RX_LOG("render/canvas", logger);

struct Paint {
  enum class Type : Uint8 {
    FILL_GRADIENT,
    FILL_IMAGE,
    FILL_STENCIL,
    IMAGE
  };

  struct Data {
    Math::Mat3x3f scissor_transform;
    Math::Mat3x3f paint_transform;
    Math::Vec4f inner_color;
    Math::Vec4f outer_color;
    Math::Vec2f scissor_extent;
    Math::Vec2f scissor_scale;
    Math::Vec2f extent;
    Float32 radius            = 0.0f;
    Float32 feather           = 0.0f;
    Float32 stroke_multiplier = 0.0f;
    Float32 stroke_threshold  = 0.0f;
  };

  Sint32 image_type = 0;

  Type type;
  Data data;
};

struct Blend {
  using FactorType = Frontend::BlendState::FactorType;

  static FactorType convert_factor_type(int _factor) {
    switch (_factor) {
    case NVG_ZERO:
      return FactorType::ZERO;
    case NVG_ONE:
      return FactorType::ONE;
    case NVG_SRC_COLOR:
      return FactorType::SRC_COLOR;
    case NVG_ONE_MINUS_SRC_COLOR:
      return FactorType::ONE_MINUS_SRC_COLOR;
    case NVG_DST_COLOR:
      return FactorType::DST_COLOR;
    case NVG_ONE_MINUS_DST_COLOR:
      return FactorType::ONE_MINUS_DST_COLOR;
    case NVG_SRC_ALPHA:
      return FactorType::SRC_ALPHA;
    case NVG_ONE_MINUS_SRC_ALPHA:
      return FactorType::ONE_MINUS_SRC_ALPHA;
    case NVG_DST_ALPHA:
      return FactorType::DST_ALPHA;
    case NVG_ONE_MINUS_DST_ALPHA:
      return FactorType::ONE_MINUS_DST_ALPHA;
    case NVG_SRC_ALPHA_SATURATE:
      return FactorType::SRC_ALPHA_SATURATE;
    }
    RX_HINT_UNREACHABLE();
  }

  Blend& operator=(NVGcompositeOperationState _operation) {
    src_color = convert_factor_type(_operation.srcRGB);
    dst_color = convert_factor_type(_operation.dstRGB);
    src_alpha = convert_factor_type(_operation.srcAlpha);
    dst_alpha = convert_factor_type(_operation.dstAlpha);
    return *this;
  }

  FactorType src_color = FactorType::ONE;
  FactorType dst_color = FactorType::ZERO;
  FactorType src_alpha = FactorType::ONE;
  FactorType dst_alpha = FactorType::ZERO;
};

struct Range {
  Size offset = 0;
  Size count  = 0;
};

struct Command {
  using Image = Sint32;

  enum class Type : Uint8 {
    NONE,
    FILL,
    CONVEX_FILL,
    STROKE,
    TRIANGLES
  };
  Type type   = Type::NONE;
  Image image = 0;
  Size paint  = 0;
  Range path;
  Range triangle;
  Blend blend;
};

struct Path {
  Range fill;
  Range stroke;
};

struct Vertex {
  Math::Vec2f position;
  Math::Vec2f coordinate;
};

// Should be byte-wise compatible.
static_assert(sizeof(Vertex) == sizeof(NVGvertex));
static_assert(alignof(Vertex) == alignof(NVGvertex));

struct Renderer {
  Renderer(Frontend::Context* _context, const Math::Vec2z& _dimensions,
    Uint8 _flags);

  Uint8 flags;
  Frontend::Context* context;
  Math::Vec2z dimensions;

  template<typename T>
  struct Collection : Vector<T> {
    constexpr Collection(Memory::Allocator& _allocator)
      : Vector<T>{_allocator}
    {
    }

    Optional<Size> allocate(Size _n_elements) {
      const auto elements = Vector<T>::size();
      if (Vector<T>::resize(elements + _n_elements)) {
        return elements;
      }
      return nullopt;
    }

    T* add() {
      if (auto index = allocate(1)) {
        return Vector<T>::data() + *index;
      }
      return nullptr;
    }
  };

  void clear() {
    commands.clear();
    paths.clear();
    vertices.clear();
    paints.clear();
  }

  Frontend::BufferAllocator vertex_allocator;

  Collection<Command> commands;
  Collection<Path> paths;
  Collection<Paint> paints;
  Collection<Vertex> vertices;

  // Frontend rendering resources.

  // TaggedPtr tag is composed of these bits for |textures|.
  static inline constexpr const Uint8 TEXTURE_FLIPY        = 1 << 0;
  static inline constexpr const Uint8 TEXTURE_PREMUL_ALPHA = 1 << 1;
  Vector<TaggedPtr<Frontend::Texture2D>> textures;

  Frontend::Buffer* buffer;
  Frontend::Target* target;
  Frontend::Texture2D* texture;
  Frontend::Technique* technique;
  Frontend::State state;
};

Renderer::Renderer(Frontend::Context* _context, const Math::Vec2z& _dimensions,
  Uint8 _flags)
  : flags{_flags}
  , context{_context}
  , dimensions{_dimensions}
  , commands{_context->allocator()}
  , paths{_context->allocator()}
  , paints{_context->allocator()}
  , vertices{vertex_allocator}
  , textures{_context->allocator()}
  , buffer{nullptr}
  , target{nullptr}
  , texture{nullptr}
  , technique{nullptr}
{
}

// Helper functions
static inline Math::Mat3x3f transform_to_mat3x3f(const Float32 (&_transform)[6]) {
  return {
    {_transform[0], _transform[1], 0.0f},
    {_transform[2], _transform[3], 0.0f},
    {_transform[4], _transform[5], 1.0f}
  };
}

static inline Math::Vec4f premul_color(NVGcolor _color) {
  return {
    _color.r * _color.a,
    _color.g * _color.a,
    _color.b * _color.a,
    _color.a
  };
}

static Size count_vertices_for_paths(const NVGpath* _paths, int _n_paths) {
  Size count = 0;
  for (Sint32 i = 0; i < _n_paths; i++) {
    const auto& path = _paths[i];
    count += path.nfill;
    count += path.nstroke;
  }
  return count;
}

[[nodiscard]]
static bool resize_target(Renderer* _renderer, const Math::Vec2z& _dimensions) {
  const auto context = _renderer->context;

  // Create new texture and target.
  auto texture = context->create_texture2D(RX_RENDER_TAG("canvas"));
  auto target = context->create_target(RX_RENDER_TAG("canvas"));
  if (!texture || !target) {
    logger->error("target resize failed");
    context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
    return false;
  }

  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_filter({false, false, false});
  texture->record_levels(1);
  texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                        Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  texture->record_dimensions(_dimensions);

  target->request_stencil(
    Frontend::Texture::DataFormat::S8, _dimensions);

  target->attach_texture(texture, 0);

  context->initialize_texture(RX_RENDER_TAG("canvas"), texture);
  context->initialize_target(RX_RENDER_TAG("canvas"), target);

  context->destroy_texture(RX_RENDER_TAG("canvas"), _renderer->texture);
  context->destroy_target(RX_RENDER_TAG("canvas"), _renderer->target);

  _renderer->texture = texture;
  _renderer->target = target;

  _renderer->state.viewport.record_dimensions(_dimensions);

  logger->verbose("resized target to %s", _dimensions);

  return true;
}

// Actual rendering functions are prefixed |draw_| and are after |nvg_|.
static void draw_fill(Renderer* _renderer, const Command& _command);
static void draw_convex_fill(Renderer* _renderer, const Command& _command);
static void draw_stroke(Renderer* _renderer, const Command& _command);
static void draw_triangles(Renderer* _renderer, const Command& _command);

// Every function prefixed with |nvg_| are functions given to NVGparams and
// passed to nvgCreateInternal to create the renderer.
static int nvg_render_create(void* _user) {
  // Construct target, texture and buffer for rendering.
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto context = renderer->context;

  const auto technique = context->find_technique_by_name("canvas");
  if (!technique) {
    logger->error("failed to find rendering technique");
    return 0;
  }

  Frontend::Buffer::Format format;
  format.record_type(Frontend::Buffer::Type::DYNAMIC);
  format.record_element_type(Frontend::Buffer::ElementType::NONE);
  format.record_instance_stride(0);
  format.record_vertex_stride(sizeof(Vertex));
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, position)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, coordinate)});
  format.finalize();

  auto buffer = context->create_buffer(RX_RENDER_TAG("buffer"));
  if (!buffer || !buffer->record_format(format)) {
    logger->error("failed to create buffer");
    context->destroy_buffer(RX_RENDER_TAG("canvas"), buffer);
    return 0;
  }

  context->initialize_buffer(RX_RENDER_TAG("canvas"), buffer);

  renderer->vertex_allocator = {Frontend::Buffer::Sink::VERTICES, buffer};
  renderer->buffer = buffer;
  renderer->technique = technique;

  // Construct the target and color attachment for the target with a helper,
  // since |nvg_viewport| must recreate it.
  if (!resize_target(renderer, renderer->dimensions)) {
    context->destroy_buffer(RX_RENDER_TAG("canvas"), buffer);
    return 0;
  }

  return 1;
}

static int nvg_render_create_texture(void* _user, int _type, int _w, int _h,
  int _image_flags, const unsigned char* _data)
{
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto context = renderer->context;
  auto& textures = renderer->textures;

  // Search for an empty slot.
  auto find = textures.find_if([](const TaggedPtr<Frontend::Texture2D>& _texture) {
    return _texture.as_ptr() == nullptr;
  });

  if (!find) {
    // Append new empty slot if one could not be found.
    find = textures.size();
    if (!textures.emplace_back(nullptr, 0_u8)) {
      return 0;
    }
  }

  auto texture = context->create_texture2D(RX_RENDER_TAG("canvas"));
  if (!texture) {
    return 0;
  }

  // Encode the image flags in the TaggedPtr.
  const Uint8 tag =
      ((_image_flags & NVG_IMAGE_FLIPY)
        ? Renderer::TEXTURE_FLIPY
        : 0)
    |
      ((_image_flags & NVG_IMAGE_PREMULTIPLIED)
        ? Renderer::TEXTURE_PREMUL_ALPHA
        : 0);

  textures[*find] = {texture, tag};

  const Bool nearest = _image_flags & NVG_IMAGE_NEAREST;
  const Bool mipmaps = _image_flags & NVG_IMAGE_GENERATE_MIPMAPS;

  const auto dimensions = Math::Vec2i(_w, _h).cast<Size>();

  // NVG doesn't provide information about if a texture is going to be modified
  // freqently. Just assume static for now. Should we recreate the texture as
  // dynamic if it's changed frequently?
  texture->record_type(Frontend::Texture::Type::STATIC);

  // Convert NVG texture type |_type| to our data format.
  switch (_type) {
  case NVG_TEXTURE_RGBA:
    texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
    break;
  case NVG_TEXTURE_ALPHA:
    texture->record_format(Frontend::Texture::DataFormat::R_U8);
    break;
  }

  // Determine wrapping behavior for the texture.
  texture->record_wrap({
    ((_image_flags & NVG_IMAGE_REPEATX)
      ? Frontend::Texture::WrapType::REPEAT
      : Frontend::Texture::WrapType::CLAMP_TO_EDGE),
    ((_image_flags & NVG_IMAGE_REPEATY)
      ? Frontend::Texture::WrapType::REPEAT
      : Frontend::Texture::WrapType::CLAMP_TO_EDGE)});

  // TODO(dweiler): Check...
  texture->record_filter({!nearest, false, mipmaps});

  // When we want mipmaps create a mipchain for the initial specification.
  if (mipmaps) {
    Texture::PixelFormat pixel_format;
    switch (_type) {
    case NVG_TEXTURE_RGBA:
      pixel_format = Texture::PixelFormat::RGBA_U8;
      break;
    case NVG_TEXTURE_ALPHA:
      pixel_format = Texture::PixelFormat::R_U8;
      break;
    default:
      logger->error("unknown texture format %d", _type);
      context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
      return false;
    }

    // Generate the mipchain for this texture.
    Texture::Chain chain{context->allocator()};
    if (!chain.generate(_data, pixel_format, pixel_format, dimensions, false, true)) {
      context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
      return false;
    }

    const auto& levels = chain.levels();
    const auto& data = chain.data();
    const auto n_levels = levels.size();

    texture->record_levels(n_levels);

    if (!texture->record_dimensions(dimensions)) {
      context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
      return false;
    }

    // Write every miplevel.
    for (Size i = 0; i < n_levels; i++) {
      texture->write(data.data() + levels[i].offset, i);
    }
  } else {
    texture->record_levels(1);

    if (!texture->record_dimensions(dimensions)) {
      context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
      return false;
    }

    // Write mipless levels.
    if (_data) {
      texture->write(_data, 0);
    }
  }

  context->initialize_texture(RX_RENDER_TAG("canvas"), texture);

  // NVG uses 1-based indices for texture ids because 0 indicates error.
  const auto index = static_cast<Sint32>(*find) + 1;

  logger->verbose("created texture %d (%zu x %zu)",
    index, dimensions.w, dimensions.h);

  return index;
}

static int nvg_render_delete_texture(void* _user, int _image) {
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto context = renderer->context;
  auto& textures = renderer->textures;

  const auto index = static_cast<Size>(_image - 1);

  if (!textures.in_range(index)) {
    logger->warning("invalid texture %d referenced", _image);
    return 0;
  }

  const auto texture = textures[index].as_ptr();
  const auto dimensions = texture->dimensions();
  context->destroy_texture(RX_RENDER_TAG("canvas"), texture);
  textures[index] = nullptr;

  logger->verbose("deleted texture %d (%zu x %zu)",
    _image, dimensions.w, dimensions.h);

  return 1;
}

static int nvg_render_update_texture(void* _user, int _image, int _x, int _y,
  int _w, int _h, const unsigned char* _data)
{
  RX_ASSERT(_w > 0 && _h > 0, "negative width or height");
  RX_ASSERT(_x >= 0 && _y >= 0, "out of bounds texture update");

  auto renderer = reinterpret_cast<Renderer*>(_user);
  auto context = renderer->context;
  auto& textures = renderer->textures;

  // NVG uses 1-based indices for texture ids because 0 indicates error.
  const auto index = static_cast<Size>(_image - 1);
  if (!textures.in_range(index)) {
    logger->warning("invalid texture %d referenced", _image);
    return 0;
  }

  auto texture = textures[index].as_ptr();
  if (!texture) {
    return 0;
  }

  // Update the area that is modified.
  const auto bits_per_pixel = texture->bits_per_pixel();
  const auto offset = Math::Vec2i(0, _y).cast<Size>();
  const auto dimensions = Math::Vec2z(texture->dimensions().w, _h);
  const auto skip = (offset.y * dimensions.w * bits_per_pixel) / 8;

  memcpy(texture->map(0) + skip, _data + skip, (dimensions.area() * bits_per_pixel) / 8);

  texture->record_edit(0, offset, dimensions);

  context->update_texture(RX_RENDER_TAG("canvas"), texture);

  return 1;
}

static int nvg_render_get_texture_size(void* _user, int _image, int* w_, int* h_)
{
  const auto renderer = reinterpret_cast<const Renderer*>(_user);
  const auto& textures = renderer->textures;

  const auto index = static_cast<Size>(_image - 1);
  if (!textures.in_range(index)) {
    logger->warning("invalid texture %d referenced", _image);
    return 0;
  }

  const auto& dimensions = textures[index].as_ptr()->dimensions().cast<int>();

  *w_ = dimensions.w;
  *h_ = dimensions.h;

  return 1;
}

[[nodiscard]]
static bool convert_paint(Renderer* _renderer, Size _paint_index,
  const NVGpaint* _paint, const NVGscissor* _scissor, Float32 _width,
  Float32 _fringe, Float32 _stroke_threshold)
{
  Float32 inverse_transform[6];

  auto& paint = _renderer->paints[_paint_index];
  auto& data = paint.data;

  data.inner_color = premul_color(_paint->innerColor);
  data.outer_color = premul_color(_paint->outerColor);

  if (_scissor->extent[0] < -0.5f || _scissor->extent[1] < -0.5f) {
    // We do not want the identity matrix here, otherwise the scissoring code
    // in the shader will incorrectly scissor this paint.
    data.scissor_transform = {{0.0f, 0.0f, 0.0f},
                              {0.0f, 0.0f, 0.0f},
                              {0.0f, 0.0f, 0.0f}};
    data.scissor_extent = {1.0f, 1.0f};
    data.scissor_scale = {1.0f, 1.0f};
  } else {
    nvgTransformInverse(inverse_transform, _scissor->xform);
    data.scissor_transform = transform_to_mat3x3f(inverse_transform);
    data.scissor_extent = {_scissor->extent[0], _scissor->extent[1]};
    data.scissor_scale = {
      Math::length({_scissor->xform[0], _scissor->xform[2]}) / _fringe,
      Math::length({_scissor->xform[1], _scissor->xform[3]}) / _fringe
    };
  }

  data.extent = {_paint->extent[0], _paint->extent[1]};
  data.stroke_multiplier = (_width * 0.5f + _fringe * 0.5f) / _fringe;
  data.stroke_threshold = _stroke_threshold;

  if (_paint->image) {
    const auto& texture = _renderer->textures[_paint->image - 1];
    if (!texture.as_ptr()) {
      return false;
    }

    // Flip the texture on the Y-axis.
    if (texture.as_tag() & Renderer::TEXTURE_FLIPY) {
      Float32 m1[6];
      Float32 m2[6];
      nvgTransformTranslate(m1, 0.0f, data.extent.y * 0.5f);
      nvgTransformMultiply(m1, _paint->xform);
      nvgTransformScale(m2, 1.0f, -1.0f);
      nvgTransformMultiply(m2, m1);
      nvgTransformTranslate(m1, 0.0f, -data.extent.y * 0.5f);
      nvgTransformMultiply(m1, m2);
      nvgTransformInverse(inverse_transform, m1);
    } else {
      nvgTransformInverse(inverse_transform, _paint->xform);
    }

    paint.type = Paint::Type::FILL_IMAGE;

    // There's three image types handled by the technique.
    //
    //  0 - RGBA (premultiplied alpha).
    //  1 - RGBA
    //  2 - ALPHA
    //
    // Most assets should be RGBA premultiplied alpha to avoid blending artifacts
    // for transparency, though regular RGBA is useful for using as well for
    // some assets.
    //
    // ALPHA is only ever used by font rendering. It's a single channel R8
    // texture which is treated as LUMINANCE / ALPHA by the canvas technique.
    if (texture.as_ptr()->format() == Frontend::Texture::DataFormat::RGBA_U8) {
      if (texture.as_tag() & Renderer::TEXTURE_PREMUL_ALPHA) {
        paint.image_type = 0;
      } else {
        paint.image_type = 1;
      }
    } else {
      paint.image_type = 2;
    }

  } else {
    paint.type = Paint::Type::FILL_GRADIENT;

    data.radius = _paint->radius;
    data.feather = _paint->feather;

    nvgTransformInverse(inverse_transform, _paint->xform);
  }

  data.paint_transform = transform_to_mat3x3f(inverse_transform);

  return true;
}

static void nvg_render_viewport(void* _user, Float32 _width, Float32 _height,
  [[maybe_unused]] Float32 _device_pixel_ratio)
{
  auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto& dimensions = Math::Vec2f{_width, _height}.cast<Size>();

  if (dimensions != renderer->dimensions) {
    RX_ASSERT(resize_target(renderer, dimensions), "resize failed");
  }
}

static void nvg_render_cancel(void* _user) {
  reinterpret_cast<Renderer*>(_user)->clear();
}

static void nvg_render_flush(void* _user) {
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto context = renderer->context;

  auto& state = renderer->state;

  const auto& commands = renderer->commands;
  if (commands.is_empty()) {
    return renderer->clear();
  }

  // Enable(CULL_FACE)
  state.cull.record_enable(true);
  // CullFace(BACK)
  state.cull.record_cull_face(Frontend::CullState::CullFaceType::BACK);
  // FrontFace(CCW)
  state.cull.record_front_face(Frontend::CullState::FrontFaceType::COUNTER_CLOCK_WISE);

  // Enable(BLEND)
  state.blend.record_enable(true);
  // ColorMask(TRUE, TRUE, TRUE, TRUE)
  state.blend.record_write_mask(0b1111);

  // Disable(DEPTH_TEST)
  state.depth.record_test(false);
  state.depth.record_write(false);

  // Disable(SCISSOR_TEST)
  state.scissor.record_enable(false);

  // StencilMask(0xff)
  state.stencil.record_write_mask(0xff);
  // StencilOp(KEEP, KEEP, KEEP)
  state.stencil.record_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::KEEP);
  // StencilFunc(ALWAYS, 0, 0xff)
  state.stencil.record_function(Frontend::StencilState::FunctionType::ALWAYS);
  state.stencil.record_reference(0);
  state.stencil.record_mask(0xff);

  // Since |renderer->vertices| is modeled ontop of a BufferAllocator, there's
  // no need to write the vertices into |renderer->buffer| as they're already
  // constructed there.
  //
  // Just record an edit to the vertices and update the buffer here.
  renderer->buffer->record_vertices_edit(0, renderer->vertices.size() * sizeof(Vertex));
  context->update_buffer(RX_RENDER_TAG("canvas"), renderer->buffer);

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  context->clear(
    RX_RENDER_TAG("canvas"),
    state,
    renderer->target,
    draw_buffers,
    RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL | RX_RENDER_CLEAR_COLOR(0),
    1.0,
    0,
    Math::Vec4f{0.0f, 0.0f, 0.0f, 1.0f}.data());

  renderer->commands.each_fwd([&](const Command& _command) {
    const auto& blend = _command.blend;
    state.blend.record_color_blend_factors(blend.src_color, blend.dst_color);
    state.blend.record_alpha_blend_factors(blend.src_alpha, blend.dst_alpha);

    switch (_command.type) {
    case Command::Type::FILL:
      draw_fill(renderer, _command);
      break;
    case Command::Type::CONVEX_FILL:
      draw_convex_fill(renderer, _command);
      break;
    case Command::Type::STROKE:
      draw_stroke(renderer, _command);
      break;
    case Command::Type::TRIANGLES:
      draw_triangles(renderer, _command);
      break;
    case Command::Type::NONE:
      // Should not reach.
      break;
    }
    return true;
  });

  state.cull.record_enable(false);

  renderer->clear();
}

static void nvg_render_fill(void* _user, NVGpaint* _paint,
  NVGcompositeOperationState _composite_operation, NVGscissor* _scissor,
  Float32 _fringe, const Float32* _bounds, const NVGpath* _paths, Sint32 _n_paths)
{
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto command = renderer->commands.add();
  if (!command) {
    return;
  }

  const auto path = renderer->paths.allocate(_n_paths);
  if (!path) {
    return renderer->commands.pop_back();
  }

  command->type = Command::Type::FILL;
  command->triangle.count = 4;
  command->path.offset = *path;
  command->path.count = _n_paths;
  command->image = _paint->image;
  command->blend = _composite_operation;

  if (_n_paths == 1 && _paths[0].convex) {
    command->type = Command::Type::CONVEX_FILL;
    // Don't need bounding-box fill quad for convex fills.
    command->triangle.count = 0;
  }

  const auto n_vertices = count_vertices_for_paths(_paths, _n_paths)
    + command->triangle.count;

  const auto index = renderer->vertices.allocate(n_vertices);
  if (!index) {
    return renderer->commands.pop_back();
  }

  Size offset = *index;
  for (Sint32 i = 0; i < _n_paths; i++) {
    auto& dst = renderer->paths[command->path.offset + i];
    const auto& src = _paths[i];

    if (src.nfill) {
      dst.fill.offset = offset;
      dst.fill.count = src.nfill;
      for (Sint32 j = 0; j < src.nfill; j++) {
        const auto& fill = src.fill[j];
        renderer->vertices[offset + j] = {{fill.x, fill.y}, {fill.u, fill.v}};
      }
      offset += src.nfill;
    }

    if (src.nstroke) {
      dst.stroke.offset = offset;
      dst.stroke.count = src.nstroke;
      for (Sint32 j = 0; j < src.nstroke; j++) {
        const auto& stroke = src.stroke[j];
        renderer->vertices[offset + j] = {{stroke.x, stroke.y}, {stroke.u, stroke.v}};
      }
      offset += src.nstroke;
    }
  }

  if (command->type == Command::Type::FILL) {
    command->triangle.offset = offset;

    // Emit quad vertices.
    auto* quad = &renderer->vertices[offset];

    quad[0] = {{_bounds[2], _bounds[3]}, {0.5f, 1.0f}};
    quad[1] = {{_bounds[2], _bounds[1]}, {0.5f, 1.0f}};
    quad[2] = {{_bounds[0], _bounds[3]}, {0.5f, 1.0f}};
    quad[3] = {{_bounds[0], _bounds[1]}, {0.5f, 1.0f}};

    const auto index = renderer->paints.allocate(2);
    if (!index) {
      return renderer->commands.pop_back();
    }

    command->paint = *index;

    // Simple pass for filling stencil.
    auto& paint = renderer->paints[*index];

    paint.type = Paint::Type::FILL_STENCIL;
    paint.data.stroke_threshold = -1.0f;

    // Fill pass.
    if (!convert_paint(renderer, *index + 1, _paint, _scissor, _fringe, _fringe, -1.0f)) {
      return renderer->commands.pop_back();
    }
  } else {
    const auto index = renderer->paints.allocate(1);
    if (!index) {
      return renderer->commands.pop_back();
    }

    command->paint = *index;

    // Fill pass.
    if (!convert_paint(renderer, *index + 0, _paint, _scissor, _fringe, _fringe, -1.0f)) {
      return renderer->commands.pop_back();
    }
  }
}

static void nvg_render_stroke(void* _user, NVGpaint* _paint,
  NVGcompositeOperationState _composite_operation, NVGscissor* _scissor,
  Float32 _fringe, Float32 _stroke_width, const NVGpath* _paths, Sint32 _n_paths)
{
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto command = renderer->commands.add();
  if (!command) {
    return;
  }

  const auto path = renderer->paths.allocate(_n_paths);
  if (!path) {
    return renderer->commands.pop_back();
  }

  command->type = Command::Type::STROKE;
  command->path.offset = *path;
  command->path.count = _n_paths;
  command->image = _paint->image;
  command->blend = _composite_operation;

  const auto n_vertices = count_vertices_for_paths(_paths, _n_paths);
  const auto index = renderer->vertices.allocate(n_vertices);
  if (!index) {
    return renderer->commands.pop_back();
  }

  Size offset = *index;
  for (Sint32 i = 0; i < _n_paths; i++) {
    auto& dst = renderer->paths[i];
    const auto& src = _paths[i];
    if (src.nstroke) {
      dst.stroke.offset = offset;
      dst.stroke.count = src.nstroke;
      for (Sint32 j = 0; j < src.nstroke; j++) {
        const auto& stroke = src.stroke[j];
        renderer->vertices[offset + j] = {{stroke.x, stroke.y}, {stroke.u, stroke.v}};
      }
      offset += src.nstroke;
    }
  }

  if (renderer->flags & Canvas::STENCIL_STROKES) {
    const auto paint = renderer->paints.allocate(2);
    if (!paint) {
      return renderer->commands.pop_back();
    }

    command->paint = *paint;

    if (!convert_paint(renderer, *paint + 0, _paint, _scissor, _stroke_width, _fringe, -1.0f)) {
      return renderer->commands.pop_back();
    }

    if (!convert_paint(renderer, *paint + 1, _paint, _scissor, _stroke_width, _fringe, 1.0f - 0.5f / 255.0f)) {
      return renderer->commands.pop_back();
    }
  } else {
    const auto paint = renderer->paints.allocate(1);
    if (!paint) {
      return renderer->commands.pop_back();
    }

    command->paint = *paint;

    if (!convert_paint(renderer, *paint + 0, _paint, _scissor, _stroke_width, _fringe, -1.0f)) {
      return renderer->commands.pop_back();
    }
  }
}

static void nvg_render_triangles(void* _user, NVGpaint* _paint,
  NVGcompositeOperationState _composite_operation, NVGscissor* _scissor,
  const NVGvertex* _vertices, Sint32 _n_vertices, Float32 _fringe)
{
  const auto renderer = reinterpret_cast<Renderer*>(_user);
  const auto command = renderer->commands.add();
  if (!command) {
    return;
  }

  command->type = Command::Type::TRIANGLES;
  command->image = _paint->image;
  command->blend = _composite_operation;

  const auto index = renderer->vertices.allocate(_n_vertices);
  if (!index) {
    return renderer->commands.pop_back();
  }

  command->triangle.offset = *index;
  command->triangle.count = _n_vertices;

  Size offset = *index;
  for (Sint32 i = 0; i < _n_vertices; i++) {
    const auto& vertex = _vertices[i];
    renderer->vertices[offset + i] = {{vertex.x, vertex.y}, {vertex.u, vertex.v}};
  }

  const auto paint = renderer->paints.allocate(1);
  if (!paint) {
    return renderer->commands.pop_back();
  }

  // Fill shader.
  command->paint = *paint;

  if (!convert_paint(renderer, *paint + 0, _paint, _scissor, 1.0f, _fringe, -1.0f)) {
    return renderer->commands.pop_back();
  }

  // The convert_paint call above will determine the incorrect paint type for
  // this fill pass, here we specifically want IMAGE, so change it.
  renderer->paints[*paint + 0].type = Paint::Type::IMAGE;
}

static void nvg_render_delete(void* _user) {
  auto renderer = reinterpret_cast<Renderer*>(_user);
  auto context = renderer->context;

  auto& allocator = context->allocator();

  auto& textures = renderer->textures;

  context->destroy_buffer(RX_RENDER_TAG("canvas"), renderer->buffer);
  context->destroy_target(RX_RENDER_TAG("canvas"), renderer->target);
  context->destroy_texture(RX_RENDER_TAG("canvas"), renderer->texture);

  textures.each_fwd([&](const TaggedPtr<Frontend::Texture2D>& _texture) {
    context->destroy_texture(RX_RENDER_TAG("canvas"), _texture.as_ptr());
  });

  allocator.destroy<Renderer>(renderer);
}

Optional<Canvas> Canvas::create(Frontend::Context* _context,
  const Math::Vec2z& _dimensions, Uint8 _flags)
{
  auto& allocator = _context->allocator();
  auto renderer = allocator.create<Renderer>(_context, _dimensions, _flags);
  if (!renderer) {
    return nullopt;
  }

  NVGparams params;
  params.renderCreate = nvg_render_create;
  params.renderCreateTexture = nvg_render_create_texture;
  params.renderDeleteTexture = nvg_render_delete_texture;
  params.renderUpdateTexture = nvg_render_update_texture;
  params.renderGetTextureSize = nvg_render_get_texture_size;
  params.renderViewport = nvg_render_viewport;
  params.renderCancel = nvg_render_cancel;
  params.renderFlush = nvg_render_flush;
  params.renderFill = nvg_render_fill;
  params.renderStroke = nvg_render_stroke;
  params.renderTriangles = nvg_render_triangles;
  params.renderDelete = nvg_render_delete;
  params.edgeAntiAlias = _flags & ANTIALIAS ? 1 : 0;
  params.userPtr = reinterpret_cast<void*>(renderer);

  auto context = nvgCreateInternal(&params);
  if (!context) {
    // nvgCreateInternal calls nvgDeleteInternal on failure. No need to
    // destroy |renderer| here as |nvg_render_delete| is called by
    // nvgDeleteInternal with |params.userPtr| being |renderer|.
    return nullopt;
  }

  return Canvas{context};
}

Frontend::Texture2D* Canvas::texture() const {
  const auto internal = nvgInternalParams(m_context)->userPtr;
  return reinterpret_cast<Renderer*>(internal)->texture;
}

void Canvas::release() {
  if (m_context) {
    logger->verbose("deleting context");
    nvgDeleteInternal(m_context);
  }
}

// Helper function which prepares a draw.
static void prepare_draw(Renderer* _renderer, const Command& _command,
  Size _paint_index, Frontend::Program*& program_, Frontend::Textures& textures_)
{
  textures_.clear();

  const auto& paint = _renderer->paints[_paint_index];
  const auto& data = paint.data;

  const auto program = _renderer->technique->variant(static_cast<Size>(paint.type));

  // When there is an image, record the sampler, add the draw texture and
  // determine the sampling type to use in the technique.
  if (_command.image) {
    program->uniforms()[0].record_sampler(
      textures_.add(_renderer->textures[_command.image - 1].as_ptr()));
    program->uniforms()[1].record_int(paint.image_type);
  }

  program->uniforms()[2].record_vec2f(_renderer->dimensions.cast<Float32>());
  program->uniforms()[3].record_mat3x3f(data.scissor_transform);
  program->uniforms()[4].record_mat3x3f(data.paint_transform);
  program->uniforms()[5].record_vec4f(data.inner_color);
  program->uniforms()[6].record_vec4f(data.outer_color);
  program->uniforms()[7].record_vec2f(data.scissor_extent);
  program->uniforms()[8].record_vec2f(data.scissor_scale);
  program->uniforms()[9].record_vec2f(data.extent);
  program->uniforms()[10].record_float(data.radius);
  program->uniforms()[11].record_float(data.feather);
  program->uniforms()[12].record_float(data.stroke_multiplier);
  program->uniforms()[13].record_float(data.stroke_threshold);

  program_ = program;
}

// Actual rendering functions are prefixed |draw_| and are after |nvg_|.
void draw_fill(Renderer* _renderer, const Command& _command) {
  const auto context = _renderer->context;
  auto& state = _renderer->state;

  //
  // Draw shapes.
  //

  // Enable(STENCIL_TEST)
  state.stencil.record_enable(true);
  // StencilMask(0xff)
  state.stencil.record_write_mask(0xff);
  // StencilFunc(ALWAYS, 0, 0xff)
  state.stencil.record_function(Frontend::StencilState::FunctionType::ALWAYS);
  state.stencil.record_reference(0);
  state.stencil.record_mask(0xff);
  // StencilOpSeparate(FRONT, KEEP, KEEP, INCR_WRAP)
  state.stencil.record_front_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_front_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_front_depth_pass_action(Frontend::StencilState::OperationType::INCREMENT_WRAP);
  // StencilOpSeparate(BACK, KEEP, KEEP, DECR_WRAP)
  state.stencil.record_back_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_back_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
  state.stencil.record_back_depth_pass_action(Frontend::StencilState::OperationType::DECREMENT_WRAP);

  // ColorMask(FALSE, FALSE, FALSE, FALSE)
  state.blend.record_write_mask(0b0000);

  // Disable(CULL_FACE)
  state.cull.record_enable(false);

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  Frontend::Program* program;

  prepare_draw(_renderer, _command, _command.paint + 0, program, draw_textures);

  for (Size i = 0; i < _command.path.count; i++) {
    const auto& path = _renderer->paths[_command.path.offset + i];
    context->draw(
      RX_RENDER_TAG("canvas"),
      state,
      _renderer->target,
      draw_buffers,
      _renderer->buffer,
      program,
      path.fill.count,
      path.fill.offset,
      0,
      0,
      0,
      Frontend::PrimitiveType::TRIANGLE_FAN,
      draw_textures);
  }

  // Enable(CULL_FACE)
  state.cull.record_enable(true);

  //
  // Draw anti-aliased pixels.
  //

  // ColorMask(TRUE, TRUE, TRUE, TRUE)
  state.blend.record_write_mask(0b1111);

  prepare_draw(_renderer, _command, _command.paint + 1, program, draw_textures);

  if (_renderer->flags & Canvas::ANTIALIAS) {
    // StencilFunc(EQUAL, 0x00, 0xff)
    state.stencil.record_function(Frontend::StencilState::FunctionType::EQUAL);
    state.stencil.record_reference(0x00);
    state.stencil.record_mask(0xff);

    // StencilOp(KEEP, KEEP, KEEP)
    state.stencil.record_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::KEEP);

    //
    // Draw fringes
    //
    for (Size i = 0; i < _command.path.count; i++) {
      const auto& path = _renderer->paths[_command.path.offset + i];
      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }
  }

  //
  // Draw fill.
  //

  if (_command.triangle.count) {
    // StencilFunc(NOTEQUAL, 0x00, 0xff)
    state.stencil.record_function(Frontend::StencilState::FunctionType::NOT_EQUAL);
    state.stencil.record_reference(0x00);
    state.stencil.record_mask(0xff);

    // StencilOp(ZERO, ZERO, ZERO)
    state.stencil.record_fail_action(Frontend::StencilState::OperationType::ZERO);
    state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::ZERO);
    state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::ZERO);

    context->draw(
      RX_RENDER_TAG("canvas"),
      state,
      _renderer->target,
      draw_buffers,
      _renderer->buffer,
      program,
      _command.triangle.count,
      _command.triangle.offset,
      0,
      0,
      0,
      Frontend::PrimitiveType::TRIANGLE_STRIP,
      draw_textures);
  }

  // Disable(STENCIL_TEST)
  state.stencil.record_enable(false);
}

void draw_convex_fill(Renderer* _renderer, const Command& _command) {
  const auto context = _renderer->context;
  const auto& state = _renderer->state;

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  Frontend::Program* program;

  prepare_draw(_renderer, _command, _command.paint + 0, program, draw_textures);

  for (Size i = 0; i < _command.path.count; i++) {
    const auto& path = _renderer->paths[_command.path.offset + i];
    context->draw(
      RX_RENDER_TAG("canvas"),
      state,
      _renderer->target,
      draw_buffers,
      _renderer->buffer,
      program,
      path.fill.count,
      path.fill.offset,
      0,
      0,
      0,
      Frontend::PrimitiveType::TRIANGLE_FAN,
      draw_textures);

    // Draw fringes.
    if (path.stroke.count) {
      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }
  }
}

void draw_stroke(Renderer* _renderer, const Command& _command) {
  const auto context = _renderer->context;
  auto& state = _renderer->state;

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  Frontend::Program* program;

  // This is the most complicated draw.
  if (_renderer->flags & Canvas::STENCIL_STROKES) {
    // Enable(STENCIL_TEST)
    state.stencil.record_enable(true);
    // StencilMask(0xff)
    state.stencil.record_write_mask(0xff);

    //
    // Fill the stroke base without overlap.
    //

    // StencilFunc(EQUAL, 0x00, 0xff)
    state.stencil.record_function(Frontend::StencilState::FunctionType::EQUAL);
    state.stencil.record_reference(0x00);
    state.stencil.record_mask(0xff);

    // StencilOp(KEEP, KEEP, INCR)
    state.stencil.record_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::INCREMENT);

    prepare_draw(_renderer, _command, _command.paint + 1, program, draw_textures);

    for (Size i = 0; i < _command.path.count; i++) {
      const auto& path = _renderer->paths[_command.path.offset + i];
      if (path.stroke.count == 0) {
        continue;
      }

      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }

    //
    // Draw anti-aliased pixels.
    //

    // StencilFunc(EQUAL, 0x00, 0xff)
    state.stencil.record_function(Frontend::StencilState::FunctionType::EQUAL);
    state.stencil.record_reference(0x00);
    state.stencil.record_mask(0xff);

    // StencilOp(KEEP, KEEP, KEEP)
    state.stencil.record_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::KEEP);
    state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::KEEP);

    prepare_draw(_renderer, _command, _command.paint + 0, program, draw_textures);

    for (Size i = 0; i < _command.path.count; i++) {
      const auto& path = _renderer->paths[_command.path.offset + i];
      if (path.stroke.count == 0) {
        continue;
      }

      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }

    //
    // Clear stencil buffer
    //

    // ColorMask(FALSE, FALSE, FALSE)
    state.blend.record_write_mask(0b0000);

    // StencilFun(ALWAYS, 0x00, 0xff)
    state.stencil.record_function(Frontend::StencilState::FunctionType::ALWAYS);
    state.stencil.record_reference(0x00);
    state.stencil.record_mask(0xff);

    // StencilOp(ZERO, ZERO, ZERO)
    state.stencil.record_fail_action(Frontend::StencilState::OperationType::ZERO);
    state.stencil.record_depth_fail_action(Frontend::StencilState::OperationType::ZERO);
    state.stencil.record_depth_pass_action(Frontend::StencilState::OperationType::ZERO);

    for (Size i = 0; i < _command.path.count; i++) {
      const auto& path = _renderer->paths[_command.path.offset + i];
      if (path.stroke.count == 0) {
        continue;
      }

      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }

    // ColorMask(TRUE, TRUE, TRUE)
    state.blend.record_write_mask(0b1111);

    // Disable(STENCIL_TEST)
    state.stencil.record_enable(false);
  } else {
    prepare_draw(_renderer, _command, _command.paint + 0, program, draw_textures);

    for (Size i = 0; i < _command.path.count; i++) {
      const auto& path = _renderer->paths[_command.path.offset + i];
      if (path.stroke.count == 0) {
        continue;
      }

      context->draw(
        RX_RENDER_TAG("canvas"),
        state,
        _renderer->target,
        draw_buffers,
        _renderer->buffer,
        program,
        path.stroke.count,
        path.stroke.offset,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLE_STRIP,
        draw_textures);
    }
  }
}

void draw_triangles(Renderer* _renderer, const Command& _command) {
  const auto context = _renderer->context;
  const auto& state = _renderer->state;

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  Frontend::Program* program;
  prepare_draw(_renderer, _command, _command.paint + 0, program, draw_textures);

  context->draw(
    RX_RENDER_TAG("canvas"),
    state,
    _renderer->target,
    draw_buffers,
    _renderer->buffer,
    program,
    _command.triangle.count,
    _command.triangle.offset,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    draw_textures);
}

} // namespace Rx::Render