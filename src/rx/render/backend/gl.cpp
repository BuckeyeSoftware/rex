#include "rx/render/backend/gl.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Render {

GLenum convert_blend_factor(Frontend::BlendState::FactorType _factor_type) {
  switch (_factor_type) {
  case Frontend::BlendState::FactorType::CONSTANT_ALPHA:
    return GL_CONSTANT_ALPHA;
  case Frontend::BlendState::FactorType::CONSTANT_COLOR:
    return GL_CONSTANT_COLOR;
  case Frontend::BlendState::FactorType::DST_ALPHA:
    return GL_DST_ALPHA;
  case Frontend::BlendState::FactorType::DST_COLOR:
    return GL_DST_COLOR;
  case Frontend::BlendState::FactorType::ONE:
    return GL_ONE;
  case Frontend::BlendState::FactorType::ONE_MINUS_CONSTANT_ALPHA:
    return GL_ONE_MINUS_CONSTANT_ALPHA;
  case Frontend::BlendState::FactorType::ONE_MINUS_CONSTANT_COLOR:
    return GL_ONE_MINUS_CONSTANT_COLOR;
  case Frontend::BlendState::FactorType::ONE_MINUS_DST_ALPHA:
    return GL_ONE_MINUS_DST_ALPHA;
  case Frontend::BlendState::FactorType::ONE_MINUS_DST_COLOR:
    return GL_ONE_MINUS_DST_COLOR;
  case Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA:
    return GL_ONE_MINUS_SRC_ALPHA;
  case Frontend::BlendState::FactorType::ONE_MINUS_SRC_COLOR:
    return GL_ONE_MINUS_SRC_COLOR;
  case Frontend::BlendState::FactorType::SRC_ALPHA:
    return GL_SRC_ALPHA;
  case Frontend::BlendState::FactorType::SRC_ALPHA_SATURATE:
    return GL_SRC_ALPHA_SATURATE;
  case Frontend::BlendState::FactorType::SRC_COLOR:
    return GL_SRC_COLOR;
  case Frontend::BlendState::FactorType::ZERO:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_operation(Frontend::StencilState::OperationType _operation_type) {
  switch (_operation_type) {
  case Frontend::StencilState::OperationType::DECREMENT:
    return GL_DECR;
  case Frontend::StencilState::OperationType::DECREMENT_WRAP:
    return GL_DECR_WRAP;
  case Frontend::StencilState::OperationType::INCREMENT:
    return GL_INCR;
  case Frontend::StencilState::OperationType::INCREMENT_WRAP:
    return GL_INCR_WRAP;
  case Frontend::StencilState::OperationType::INVERT:
    return GL_INVERT;
  case Frontend::StencilState::OperationType::KEEP:
    return GL_KEEP;
  case Frontend::StencilState::OperationType::REPLACE:
    return GL_REPLACE;
  case Frontend::StencilState::OperationType::ZERO:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_function(Frontend::StencilState::FunctionType _function_type) {
  switch (_function_type) {
  case Frontend::StencilState::FunctionType::ALWAYS:
    return GL_ALWAYS;
  case Frontend::StencilState::FunctionType::EQUAL:
    return GL_EQUAL;
  case Frontend::StencilState::FunctionType::GREATER:
    return GL_GREATER;
  case Frontend::StencilState::FunctionType::GREATER_EQUAL:
    return GL_GEQUAL;
  case Frontend::StencilState::FunctionType::LESS:
    return GL_LESS;
  case Frontend::StencilState::FunctionType::LESS_EQUAL:
    return GL_LEQUAL;
  case Frontend::StencilState::FunctionType::NEVER:
    return GL_NEVER;
  case Frontend::StencilState::FunctionType::NOT_EQUAL:
    return GL_NOTEQUAL;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_polygon_mode(Frontend::PolygonState::ModeType _mode_type) {
  switch (_mode_type) {
  case Frontend::PolygonState::ModeType::FILL:
    return GL_FILL;
  case Frontend::PolygonState::ModeType::LINE:
    return GL_LINE;
  case Frontend::PolygonState::ModeType::POINT:
    return GL_POINT;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_format(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::RGBA_U8:
    return GL_RGBA8;
  case Frontend::Texture::DataFormat::RGB_U8:
    return GL_RGB8;
  case Frontend::Texture::DataFormat::BGRA_U8:
    return GL_RGBA8; // not a bug
  case Frontend::Texture::DataFormat::BGR_U8:
    return GL_RGB8; // not a bug
  case Frontend::Texture::DataFormat::RGBA_F16:
    return GL_RGBA16F;
  case Frontend::Texture::DataFormat::BGRA_F16:
    return GL_RGBA16F; // not a bug
  case Frontend::Texture::DataFormat::RGBA_F32:
    return GL_RGBA32F;
  case Frontend::Texture::DataFormat::D16:
    return GL_DEPTH_COMPONENT16;
  case Frontend::Texture::DataFormat::D24:
    return GL_DEPTH_COMPONENT24;
  case Frontend::Texture::DataFormat::D32:
    return GL_DEPTH_COMPONENT32;
  case Frontend::Texture::DataFormat::D32F:
    return GL_DEPTH_COMPONENT32F;
  case Frontend::Texture::DataFormat::D24_S8:
    return GL_DEPTH24_STENCIL8;
  case Frontend::Texture::DataFormat::D32F_S8:
    return GL_DEPTH32F_STENCIL8;
  case Frontend::Texture::DataFormat::S8:
    return GL_STENCIL_INDEX8;
  case Frontend::Texture::DataFormat::R_U8:
    return GL_R8;
  case Frontend::Texture::DataFormat::DXT1:
    return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
  case Frontend::Texture::DataFormat::DXT5:
    return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
  case Frontend::Texture::DataFormat::SRGB_U8:
    return GL_SRGB8;
  case Frontend::Texture::DataFormat::SRGBA_U8:
    return GL_SRGB8_ALPHA8;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_type(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::D16:
    return GL_UNSIGNED_SHORT;
  case Frontend::Texture::DataFormat::D24:
    [[fallthrough]];
  case Frontend::Texture::DataFormat::D32:
    return GL_UNSIGNED_INT;
  case Frontend::Texture::DataFormat::D32F:
    return GL_FLOAT;
  case Frontend::Texture::DataFormat::RGBA_F16:
    [[fallthrough]];
  case Frontend::Texture::DataFormat::BGRA_F16:
    return GL_HALF_FLOAT;
  case Frontend::Texture::DataFormat::RGBA_F32:
    return GL_FLOAT;
  case Frontend::Texture::DataFormat::D24_S8:
    return GL_UNSIGNED_INT_24_8;
  case Frontend::Texture::DataFormat::D32F_S8:
    return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
  case Frontend::Texture::DataFormat::S8:
    return GL_UNSIGNED_BYTE;
  default:
    return GL_UNSIGNED_BYTE;
  }
}

GLenum convert_texture_format(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::RGBA_U8:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::RGB_U8:
    return GL_RGB;
  case Frontend::Texture::DataFormat::BGRA_U8:
    return GL_BGRA;
  case Frontend::Texture::DataFormat::BGR_U8:
    return GL_BGR;
  case Frontend::Texture::DataFormat::RGBA_F16:
    [[fallthrough]];
  case Frontend::Texture::DataFormat::RGBA_F32:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::BGRA_F16:
    return GL_BGRA;
  case Frontend::Texture::DataFormat::D16:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::D24:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::D32:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::D32F:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::D24_S8:
    return GL_DEPTH_STENCIL;
  case Frontend::Texture::DataFormat::D32F_S8:
    return GL_DEPTH_STENCIL;
  case Frontend::Texture::DataFormat::S8:
    return GL_STENCIL_INDEX;
  case Frontend::Texture::DataFormat::R_U8:
    return GL_RED;
  case Frontend::Texture::DataFormat::DXT1:
    return GL_RGB;
  case Frontend::Texture::DataFormat::DXT5:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::SRGB_U8:
    return GL_RGB;
  case Frontend::Texture::DataFormat::SRGBA_U8:
    return GL_RGBA;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_primitive_type(Frontend::PrimitiveType _primitive_type) {
  switch (_primitive_type) {
  case Frontend::PrimitiveType::TRIANGLES:
    return GL_TRIANGLES;
  case Frontend::PrimitiveType::TRIANGLE_STRIP:
    return GL_TRIANGLE_STRIP;
  case Frontend::PrimitiveType::POINTS:
    return GL_POINTS;
  case Frontend::PrimitiveType::LINES:
    return GL_LINES;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_wrap(const Frontend::Texture::WrapType _type) {
  switch (_type) {
  case Frontend::Texture::WrapType::CLAMP_TO_EDGE:
    return GL_CLAMP_TO_EDGE;
  case Frontend::Texture::WrapType::CLAMP_TO_BORDER:
    return GL_CLAMP_TO_BORDER;
  case Frontend::Texture::WrapType::MIRRORED_REPEAT:
    return GL_MIRRORED_REPEAT;
  case Frontend::Texture::WrapType::REPEAT:
    return GL_REPEAT;
  case Frontend::Texture::WrapType::MIRROR_CLAMP_TO_EDGE:
    return GL_MIRROR_CLAMP_TO_EDGE;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_element_type(Frontend::Buffer::ElementType _element_type) {
  using Type = Frontend::Buffer::ElementType;
  switch (_element_type) {
  case Type::NONE:
    return GL_NONE;
  case Type::U8:
    return GL_UNSIGNED_BYTE;
  case Type::U16:
    return GL_UNSIGNED_SHORT;
  case Type::U32:
    return GL_UNSIGNED_INT;
  }
  RX_HINT_UNREACHABLE();
}

Filter convert_texture_filter(const Frontend::Texture::FilterOptions& _filter_options) {
  static constexpr const GLenum MIN_TABLE[] = {
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
  };

  int filter_index = _filter_options.bilinear ? 1 : 0;
  filter_index |= _filter_options.mipmaps << 1;
  filter_index |= _filter_options.trilinear << 2;

  const GLenum mag = static_cast<GLenum>(filter_index & 1 ? GL_LINEAR : GL_NEAREST);
  const GLenum min = MIN_TABLE[filter_index];

  return {min, mag};
}

Attribute convert_attribute(const Frontend::Buffer::Attribute& _attribute) {
  using Type = Frontend::Buffer::Attribute::Type;
  switch (_attribute.type) {
  case Type::F32:
    return {GL_FLOAT,         sizeof(Float32), 1, 1};
  case Type::VEC2F:
    return {GL_FLOAT,         sizeof(Float32), 2, 1};
  case Type::VEC3F:
    return {GL_FLOAT,         sizeof(Float32), 3, 1};
  case Type::VEC4F:
    return {GL_FLOAT,         sizeof(Float32), 4, 1};
  case Type::VEC4B:
    return {GL_UNSIGNED_BYTE, sizeof(Byte),    4, 1};
  case Type::MAT4X4F:
    return {GL_FLOAT,         sizeof(Float32), 4, 4};
  }

  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Render
