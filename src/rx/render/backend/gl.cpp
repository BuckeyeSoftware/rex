#include "rx/render/backend/gl.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Render {

static inline constexpr const char *GLSL_PRELUDE = R"(
#define RX_MAX_BONES 128

// GLSL does not have typedef. Just #define our types.
#define f32 float
#define s32 int
#define u32 uint
#define f32x2 vec2
#define f32x3 vec3
#define f32x4 vec4
#define s32x2 ivec2
#define s32x3 ivec3
#define s32x4 ivec4
#define u32x2 uvec2
#define u32x3 uvec3
#define u32x4 uvec4
#define f32x3x3 mat3x3
#define f32x2x4 mat2x4
#define f32x3x4 mat3x4
#define f32x4x4 mat4x4
#define lb_bones f32x3x4[RX_MAX_BONES]
#define dq_bones f32x2x4[RX_MAX_BONES]

// Sampler types.
#define rx_sampler1D sampler1D
#define rx_sampler2D sampler2D
#define rx_sampler3D sampler3D
#define rx_samplerCM samplerCube

// Functions to sample textures.
#define rx_texture1D texture
#define rx_texture2D texture
#define rx_texture3D texture
#define rx_textureCM texture
#define rx_texture1DLod textureLod
#define rx_texture2DLod textureLod
#define rx_texture3DLod textureLod
#define rx_textureCMLod textureLod

// Builtin variables.
#define rx_position gl_Position
#define rx_vertex_id gl_VertexID
#define rx_point_size gl_PointSize
#define rx_point_coord gl_PointCoord

// Casting functions.
s32   as_s32(f32 x)     { return s32(x); }
s32x2 as_s32x2(f32x2 x) { return s32x2(s32(x.x), s32(x.y)); }
s32x3 as_s32x3(f32x3 x) { return s32x3(s32(x.x), s32(x.y), s32(x.z)); }
s32x4 as_s32x4(f32x4 x) { return s32x4(s32(x.x), s32(x.y), s32(x.z), s32(x.w)); }
s32   as_s32(u32 x)     { return s32(x); }
s32x2 as_s32x2(u32x2 x) { return s32x2(s32(x.x), s32(x.y)); }
s32x3 as_s32x3(u32x3 x) { return s32x3(s32(x.x), s32(x.y), s32(x.z)); }
s32x4 as_s32x4(u32x4 x) { return s32x4(s32(x.x), s32(x.y), s32(x.z), s32(x.w)); }
u32   as_u32(f32 x)     { return u32(x); }
u32x2 as_u32x2(f32x2 x) { return u32x2(u32(x.x), u32(x.y)); }
u32x3 as_u32x3(f32x3 x) { return u32x3(u32(x.x), u32(x.y), u32(x.z)); }
u32x4 as_u32x4(f32x4 x) { return u32x4(u32(x.x), u32(x.y), u32(x.z), u32(x.w)); }
u32   as_u32(s32 x)     { return u32(x); }
u32x2 as_u32x2(s32x2 x) { return u32x2(u32(x.x), u32(x.y)); }
u32x3 as_u32x3(s32x3 x) { return u32x3(u32(x.x), u32(x.y), u32(x.z)); }
u32x4 as_u32x4(s32x4 x) { return u32x4(u32(x.x), u32(x.y), u32(x.z), u32(x.w)); }
f32   as_f32(s32 x)     { return f32(x); }
f32x2 as_f32x2(s32x2 x) { return f32x2(f32(x.x), f32(x.y)); }
f32x3 as_f32x3(s32x3 x) { return f32x3(f32(x.x), f32(x.y), f32(x.z)); }
f32x4 as_f32x4(s32x4 x) { return f32x4(f32(x.x), f32(x.y), f32(x.z), f32(x.w)); }
f32   as_f32(u32 x)     { return f32(x); }
f32x2 as_f32x2(u32x2 x) { return f32x2(f32(x.x), f32(x.y)); }
f32x3 as_f32x3(u32x3 x) { return f32x3(f32(x.x), f32(x.y), f32(x.z)); }
f32x4 as_f32x4(u32x4 x) { return f32x4(f32(x.x), f32(x.y), f32(x.z), f32(x.w)); }
)";



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

GLenum convert_shader_type(Frontend::Shader::Type _type) {
  switch (_type) {
  case Frontend::Shader::Type::FRAGMENT:
    return GL_FRAGMENT_SHADER;
  case Frontend::Shader::Type::VERTEX:
    return GL_VERTEX_SHADER;
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
  // Scalars.
  case Type::S32:
    return {GL_INT,          sizeof(Sint32), 1, 1};
  case Type::U32:
    return {GL_UNSIGNED_INT, sizeof(Uint32), 1, 1};
  case Type::F32:
    return {GL_FLOAT,        sizeof(Float32), 1, 1};

  // Vectors.
  case Type::S32x2:
    return {GL_INT,          sizeof(Sint32), 2, 1};
  case Type::S32x3:
    return {GL_INT,          sizeof(Sint32), 3, 1};
  case Type::S32x4:
    return {GL_INT,          sizeof(Sint32), 4, 1};
  case Type::U32x2:
    return {GL_UNSIGNED_INT, sizeof(Uint32), 2, 1};
  case Type::U32x3:
    return {GL_UNSIGNED_INT, sizeof(Uint32), 3, 1};
  case Type::U32x4:
    return {GL_UNSIGNED_INT, sizeof(Uint32), 4, 1};
  case Type::F32x2:
    return {GL_FLOAT,        sizeof(Float32), 2, 1};
  case Type::F32x3:
    return {GL_FLOAT,        sizeof(Float32), 3, 1};
  case Type::F32x4:
    return {GL_FLOAT,        sizeof(Float32), 4, 1};

  // Matrices.
  case Type::F32x4x4:
    return {GL_FLOAT,        sizeof(Float32), 4, 4};
  }

  RX_HINT_UNREACHABLE();
}

Optional<String> generate_glsl(const Vector<Frontend::Uniform>& _uniforms,
  const Frontend::Shader& _shader, int _version, bool _es)
{
  String contents;
  auto version = String::format("#version %d %s\n", _version, _es ? "es" : "core");
  if (!contents.append(version)) {
    return nullopt;
  }

  // When ES force highp.
  if (_es) {
    static constexpr const char* PRECISION_PRELUDE =
      "precision highp float;\n"
      "precision highp int;\n"
      "precision highp sampler1D;\n"
      "precision highp sampler2D;\n"
      "precision highp sampler3D;\n"
      "precision highp samplerCube;\n";

    if (!contents.append(PRECISION_PRELUDE)) {
      return nullopt;
    }
  }

  if (!contents.append(GLSL_PRELUDE)) {
    return nullopt;
  }

  auto emit_vs_input = [&](const String& _name, const Frontend::Shader::InOut& _inout) {
    return contents.append(String::format("layout(location = %zu) in %s %s;\n",
      _inout.index, inout_to_string(_inout.kind), _name));
  };

  auto emit_vs_output = [&](const String& _name, const Frontend::Shader::InOut& _inout) {
    return contents.append(String::format("out %s %s;\n",
      inout_to_string(_inout.kind), _name));
  };

  auto emit_fs_input = [&](const String& _name, const Frontend::Shader::InOut& _inout) {
    return contents.append(String::format("in %s %s;\n", inout_to_string(_inout.kind), _name));
  };

  auto emit_fs_output = [&](const String& _name, const Frontend::Shader::InOut& _inout) {
    return contents.append(String::format("layout(location = %zu) out %s %s;\n",
      _inout.index, inout_to_string(_inout.kind), _name));
  };

  switch (_shader.kind) {
  case Frontend::Shader::Type::VERTEX:
    if (!_shader.inputs.each_pair(emit_vs_input) || !_shader.outputs.each_pair(emit_vs_output)) {
      return nullopt;
    }
    break;
  case Frontend::Shader::Type::FRAGMENT:
    if (!_shader.inputs.each_pair(emit_fs_input) || !_shader.outputs.each_pair(emit_fs_output)) {
      return nullopt;
    }
    break;
  }

  // Emit uniforms.
  auto emit_uniform = [&](const Frontend::Uniform& _uniform) {
    // Don't emit padding uniforms.
    if (_uniform.is_padding()) {
      return true;
    }
    return contents.append(String::format("uniform %s %s;\n",
      uniform_to_string(_uniform.type()), _uniform.name()));
  };

  if (!_uniforms.each_fwd(emit_uniform)) {
    return nullopt;
  }

  // Emit the shader source now.
  if (!contents.append(_shader.source)) {
    return nullopt;
  }

  return contents;
}

} // namespace Rx::Render
