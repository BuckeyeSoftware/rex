#include "rx/render/backend/gl.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Render {

GLenum convert_blend_factor(Frontend::BlendState::FactorType _factor_type) {
  switch (_factor_type) {
  case Frontend::BlendState::FactorType::k_constant_alpha:
    return GL_CONSTANT_ALPHA;
  case Frontend::BlendState::FactorType::k_constant_color:
    return GL_CONSTANT_COLOR;
  case Frontend::BlendState::FactorType::k_dst_alpha:
    return GL_DST_ALPHA;
  case Frontend::BlendState::FactorType::k_dst_color:
    return GL_DST_COLOR;
  case Frontend::BlendState::FactorType::k_one:
    return GL_ONE;
  case Frontend::BlendState::FactorType::k_one_minus_constant_alpha:
    return GL_ONE_MINUS_CONSTANT_ALPHA;
  case Frontend::BlendState::FactorType::k_one_minus_constant_color:
    return GL_ONE_MINUS_CONSTANT_COLOR;
  case Frontend::BlendState::FactorType::k_one_minus_dst_alpha:
    return GL_ONE_MINUS_DST_ALPHA;
  case Frontend::BlendState::FactorType::k_one_minus_dst_color:
    return GL_ONE_MINUS_DST_COLOR;
  case Frontend::BlendState::FactorType::k_one_minus_src_alpha:
    return GL_ONE_MINUS_SRC_ALPHA;
  case Frontend::BlendState::FactorType::k_one_minus_src_color:
    return GL_ONE_MINUS_SRC_COLOR;
  case Frontend::BlendState::FactorType::k_src_alpha:
    return GL_SRC_ALPHA;
  case Frontend::BlendState::FactorType::k_src_alpha_saturate:
    return GL_SRC_ALPHA_SATURATE;
  case Frontend::BlendState::FactorType::k_src_color:
    return GL_SRC_COLOR;
  case Frontend::BlendState::FactorType::k_zero:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_operation(Frontend::StencilState::OperationType _operation_type) {
  switch (_operation_type) {
  case Frontend::StencilState::OperationType::k_decrement:
    return GL_DECR;
  case Frontend::StencilState::OperationType::k_decrement_wrap:
    return GL_DECR_WRAP;
  case Frontend::StencilState::OperationType::k_increment:
    return GL_INCR;
  case Frontend::StencilState::OperationType::k_increment_wrap:
    return GL_INCR_WRAP;
  case Frontend::StencilState::OperationType::k_invert:
    return GL_INVERT;
  case Frontend::StencilState::OperationType::k_keep:
    return GL_KEEP;
  case Frontend::StencilState::OperationType::k_replace:
    return GL_REPLACE;
  case Frontend::StencilState::OperationType::k_zero:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_function(Frontend::StencilState::FunctionType _function_type) {
  switch (_function_type) {
  case Frontend::StencilState::FunctionType::k_always:
    return GL_ALWAYS;
  case Frontend::StencilState::FunctionType::k_equal:
    return GL_EQUAL;
  case Frontend::StencilState::FunctionType::k_greater:
    return GL_GREATER;
  case Frontend::StencilState::FunctionType::k_greater_equal:
    return GL_GEQUAL;
  case Frontend::StencilState::FunctionType::k_less:
    return GL_LESS;
  case Frontend::StencilState::FunctionType::k_less_equal:
    return GL_LEQUAL;
  case Frontend::StencilState::FunctionType::k_never:
    return GL_NEVER;
  case Frontend::StencilState::FunctionType::k_not_equal:
    return GL_NOTEQUAL;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_polygon_mode(Frontend::PolygonState::ModeType _mode_type) {
  switch (_mode_type) {
  case Frontend::PolygonState::ModeType::k_fill:
    return GL_FILL;
  case Frontend::PolygonState::ModeType::k_line:
    return GL_LINE;
  case Frontend::PolygonState::ModeType::k_point:
    return GL_POINT;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_format(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::k_rgba_u8:
    return GL_RGBA8;
  case Frontend::Texture::DataFormat::k_rgb_u8:
    return GL_RGB8;
  case Frontend::Texture::DataFormat::k_bgra_u8:
    return GL_RGBA8; // not a bug
  case Frontend::Texture::DataFormat::k_bgr_u8:
    return GL_RGB8; // not a bug
  case Frontend::Texture::DataFormat::k_rgba_f16:
    return GL_RGBA16F;
  case Frontend::Texture::DataFormat::k_bgra_f16:
    return GL_RGBA16F; // not a bug
  case Frontend::Texture::DataFormat::k_d16:
    return GL_DEPTH_COMPONENT16;
  case Frontend::Texture::DataFormat::k_d24:
    return GL_DEPTH_COMPONENT24;
  case Frontend::Texture::DataFormat::k_d32:
    return GL_DEPTH_COMPONENT32;
  case Frontend::Texture::DataFormat::k_d32f:
    return GL_DEPTH_COMPONENT32F;
  case Frontend::Texture::DataFormat::k_d24_s8:
    return GL_DEPTH24_STENCIL8;
  case Frontend::Texture::DataFormat::k_d32f_s8:
    return GL_DEPTH32F_STENCIL8;
  case Frontend::Texture::DataFormat::k_s8:
    return GL_STENCIL_INDEX8;
  case Frontend::Texture::DataFormat::k_r_u8:
    return GL_R8;
  case Frontend::Texture::DataFormat::k_dxt1:
    return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
  case Frontend::Texture::DataFormat::k_dxt5:
    return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
  case Frontend::Texture::DataFormat::k_srgb_u8:
    return GL_SRGB8;
  case Frontend::Texture::DataFormat::k_srgba_u8:
    return GL_SRGB8_ALPHA8;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_type(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::k_d16:
    return GL_UNSIGNED_SHORT;
  case Frontend::Texture::DataFormat::k_d24:
    [[fallthrough]];
  case Frontend::Texture::DataFormat::k_d32:
    return GL_UNSIGNED_INT;
  case Frontend::Texture::DataFormat::k_d32f:
    return GL_FLOAT;
  case Frontend::Texture::DataFormat::k_d24_s8:
    return GL_UNSIGNED_INT_24_8;
  case Frontend::Texture::DataFormat::k_d32f_s8:
    return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
  case Frontend::Texture::DataFormat::k_s8:
    return GL_UNSIGNED_BYTE;
  default:
    return GL_UNSIGNED_BYTE;
  }
}

GLenum convert_texture_format(Frontend::Texture::DataFormat _data_format) {
  switch (_data_format) {
  case Frontend::Texture::DataFormat::k_rgba_u8:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::k_rgb_u8:
    return GL_RGB;
  case Frontend::Texture::DataFormat::k_bgra_u8:
    return GL_BGRA;
  case Frontend::Texture::DataFormat::k_bgr_u8:
    return GL_BGR;
  case Frontend::Texture::DataFormat::k_rgba_f16:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::k_bgra_f16:
    return GL_BGRA;
  case Frontend::Texture::DataFormat::k_d16:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::k_d24:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::k_d32:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::k_d32f:
    return GL_DEPTH_COMPONENT;
  case Frontend::Texture::DataFormat::k_d24_s8:
    return GL_DEPTH_STENCIL;
  case Frontend::Texture::DataFormat::k_d32f_s8:
    return GL_DEPTH_STENCIL;
  case Frontend::Texture::DataFormat::k_s8:
    return GL_STENCIL_INDEX;
  case Frontend::Texture::DataFormat::k_r_u8:
    return GL_RED;
  case Frontend::Texture::DataFormat::k_dxt1:
    return GL_RGB;
  case Frontend::Texture::DataFormat::k_dxt5:
    return GL_RGBA;
  case Frontend::Texture::DataFormat::k_srgb_u8:
    return GL_RGB;
  case Frontend::Texture::DataFormat::k_srgba_u8:
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
  case Frontend::Texture::WrapType::k_clamp_to_edge:
    return GL_CLAMP_TO_EDGE;
  case Frontend::Texture::WrapType::k_clamp_to_border:
    return GL_CLAMP_TO_BORDER;
  case Frontend::Texture::WrapType::k_mirrored_repeat:
    return GL_MIRRORED_REPEAT;
  case Frontend::Texture::WrapType::k_repeat:
    return GL_REPEAT;
  case Frontend::Texture::WrapType::k_mirror_clamp_to_edge:
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
  static constexpr const GLenum k_min_table[]{
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
  };

  int filter_index{_filter_options.bilinear ? 1 : 0};
  filter_index |= _filter_options.mipmaps << 1;
  filter_index |= _filter_options.trilinear << 2;

  const GLenum mag{static_cast<GLenum>(filter_index & 1 ? GL_LINEAR : GL_NEAREST)};
  const GLenum min{k_min_table[filter_index]};

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
