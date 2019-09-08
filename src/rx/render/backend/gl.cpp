#include "rx/render/backend/gl.h"

namespace rx::render {

GLenum convert_blend_factor(frontend::blend_state::factor_type _factor_type) {
  switch (_factor_type) {
  case frontend::blend_state::factor_type::k_constant_alpha:
    return GL_CONSTANT_ALPHA;
  case frontend::blend_state::factor_type::k_constant_color:
    return GL_CONSTANT_COLOR;
  case frontend::blend_state::factor_type::k_dst_alpha:
    return GL_DST_ALPHA;
  case frontend::blend_state::factor_type::k_dst_color:
    return GL_DST_COLOR;
  case frontend::blend_state::factor_type::k_one:
    return GL_ONE;
  case frontend::blend_state::factor_type::k_one_minus_constant_alpha:
    return GL_ONE_MINUS_CONSTANT_ALPHA;
  case frontend::blend_state::factor_type::k_one_minus_constant_color:
    return GL_ONE_MINUS_CONSTANT_COLOR;
  case frontend::blend_state::factor_type::k_one_minus_dst_alpha:
    return GL_ONE_MINUS_DST_ALPHA;
  case frontend::blend_state::factor_type::k_one_minus_dst_color:
    return GL_ONE_MINUS_DST_COLOR;
  case frontend::blend_state::factor_type::k_one_minus_src_alpha:
    return GL_ONE_MINUS_SRC_ALPHA;
  case frontend::blend_state::factor_type::k_one_minus_src_color:
    return GL_ONE_MINUS_SRC_COLOR;
  case frontend::blend_state::factor_type::k_src_alpha:
    return GL_SRC_ALPHA;
  case frontend::blend_state::factor_type::k_src_alpha_saturate:
    return GL_SRC_ALPHA_SATURATE;
  case frontend::blend_state::factor_type::k_src_color:
    return GL_SRC_COLOR;
  case frontend::blend_state::factor_type::k_zero:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_operation(frontend::stencil_state::operation_type _operation_type) {
  switch (_operation_type) {
  case frontend::stencil_state::operation_type::k_decrement:
    return GL_DECR;
  case frontend::stencil_state::operation_type::k_decrement_wrap:
    return GL_DECR_WRAP;
  case frontend::stencil_state::operation_type::k_increment:
    return GL_INCR;
  case frontend::stencil_state::operation_type::k_increment_wrap:
    return GL_INCR_WRAP;
  case frontend::stencil_state::operation_type::k_invert:
    return GL_INVERT;
  case frontend::stencil_state::operation_type::k_keep:
    return GL_KEEP;
  case frontend::stencil_state::operation_type::k_replace:
    return GL_REPLACE;
  case frontend::stencil_state::operation_type::k_zero:
    return GL_ZERO;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_stencil_function(frontend::stencil_state::function_type _function_type) {
  switch (_function_type) {
  case frontend::stencil_state::function_type::k_always:
    return GL_ALWAYS;
  case frontend::stencil_state::function_type::k_equal:
    return GL_EQUAL;
  case frontend::stencil_state::function_type::k_greater:
    return GL_GREATER;
  case frontend::stencil_state::function_type::k_greater_equal:
    return GL_GEQUAL;
  case frontend::stencil_state::function_type::k_less:
    return GL_LESS;
  case frontend::stencil_state::function_type::k_less_equal:
    return GL_LEQUAL;
  case frontend::stencil_state::function_type::k_never:
    return GL_NEVER;
  case frontend::stencil_state::function_type::k_not_equal:
    return GL_NOTEQUAL;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_polygon_mode(frontend::polygon_state::mode_type _mode_type) {
  switch (_mode_type) {
  case frontend::polygon_state::mode_type::k_fill:
    return GL_FILL;
  case frontend::polygon_state::mode_type::k_line:
    return GL_LINE;
  case frontend::polygon_state::mode_type::k_point:
    return GL_POINT;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_format(frontend::texture::data_format _data_format) {
  switch (_data_format) {
  case frontend::texture::data_format::k_rgba_u8:
    return GL_RGBA8;
  case frontend::texture::data_format::k_bgra_u8:
    return GL_RGBA8; // not a bug
  case frontend::texture::data_format::k_rgba_f16:
    return GL_RGBA16F;
  case frontend::texture::data_format::k_bgra_f16:
    return GL_RGBA16F; // not a bug
  case frontend::texture::data_format::k_d16:
    return GL_DEPTH_COMPONENT16;
  case frontend::texture::data_format::k_d24:
    return GL_DEPTH_COMPONENT24;
  case frontend::texture::data_format::k_d32:
    return GL_DEPTH_COMPONENT32;
  case frontend::texture::data_format::k_d32f:
    return GL_DEPTH_COMPONENT32F;
  case frontend::texture::data_format::k_d24_s8:
    return GL_DEPTH24_STENCIL8;
  case frontend::texture::data_format::k_d32f_s8:
    return GL_DEPTH32F_STENCIL8;
  case frontend::texture::data_format::k_s8:
    return GL_STENCIL_INDEX8;
  case frontend::texture::data_format::k_r_u8:
    return GL_R8;
  case frontend::texture::data_format::k_dxt1:
    return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
  case frontend::texture::data_format::k_dxt5:
    return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_texture_data_type(frontend::texture::data_format _data_format) {
  switch (_data_format) {
  case frontend::texture::data_format::k_d16:
    return GL_UNSIGNED_SHORT;
  case frontend::texture::data_format::k_d24:
    [[fallthrough]];
  case frontend::texture::data_format::k_d32:
    return GL_UNSIGNED_INT;
  case frontend::texture::data_format::k_d32f:
    return GL_FLOAT;
  case frontend::texture::data_format::k_d24_s8:
    return GL_UNSIGNED_INT_24_8;
  case frontend::texture::data_format::k_d32f_s8:
    return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
  case frontend::texture::data_format::k_s8:
    return GL_UNSIGNED_BYTE;
  default:
    return GL_UNSIGNED_BYTE;
  }
}

GLenum convert_texture_format(frontend::texture::data_format _data_format) {
  switch (_data_format) {
  case frontend::texture::data_format::k_rgba_u8:
    return GL_RGBA;
  case frontend::texture::data_format::k_bgra_u8:
    return GL_BGRA;
  case frontend::texture::data_format::k_rgba_f16:
    return GL_RGBA;
  case frontend::texture::data_format::k_bgra_f16:
    return GL_BGRA;
  case frontend::texture::data_format::k_d16:
    return GL_DEPTH_COMPONENT;
  case frontend::texture::data_format::k_d24:
    return GL_DEPTH_COMPONENT;
  case frontend::texture::data_format::k_d32:
    return GL_DEPTH_COMPONENT;
  case frontend::texture::data_format::k_d32f:
    return GL_DEPTH_COMPONENT;
  case frontend::texture::data_format::k_d24_s8:
    return GL_DEPTH_STENCIL;
  case frontend::texture::data_format::k_d32f_s8:
    return GL_DEPTH_STENCIL;
  case frontend::texture::data_format::k_s8:
    return GL_STENCIL_INDEX;
  case frontend::texture::data_format::k_r_u8:
    return GL_RED;
  case frontend::texture::data_format::k_dxt1:
    return GL_RGB;
  case frontend::texture::data_format::k_dxt5:
    return GL_RGBA;
  }

  RX_HINT_UNREACHABLE();
}

GLenum convert_primitive_type(frontend::primitive_type _primitive_type) {
  switch (_primitive_type) {
  case frontend::primitive_type::k_triangles:
    return GL_TRIANGLES;
  case frontend::primitive_type::k_triangle_strip:
    return GL_TRIANGLE_STRIP;
  case frontend::primitive_type::k_points:
    return GL_POINTS;
  case frontend::primitive_type::k_lines:
    return GL_LINES;
  }

  RX_HINT_UNREACHABLE();
}

filter convert_texture_filter(const frontend::texture::filter_options& _filter_options) {
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

GLenum convert_texture_wrap(const frontend::texture::wrap_type _type) {
  switch (_type) {
  case frontend::texture::wrap_type::k_clamp_to_edge:
    return GL_CLAMP_TO_EDGE;
  case frontend::texture::wrap_type::k_clamp_to_border:
    return GL_CLAMP_TO_BORDER;
  case frontend::texture::wrap_type::k_mirrored_repeat:
    return GL_MIRRORED_REPEAT;
  case frontend::texture::wrap_type::k_repeat:
    return GL_REPEAT;
  case frontend::texture::wrap_type::k_mirror_clamp_to_edge:
    return GL_MIRROR_CLAMP_TO_EDGE;
  }

  RX_HINT_UNREACHABLE();
}

} // namespace rx::render