#ifndef RX_RENDER_BACKEND_GL_H
#define RX_RENDER_BACKEND_GL_H
#include "rx/render/frontend/state.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/command.h"
#include "rx/render/frontend/program.h"

#include <SDL_video.h> // SDL_GL_GetProcAddress
#include <SDL_opengl.h>

#undef ERROR

namespace Rx::Render {

GLenum convert_blend_factor(Frontend::BlendState::FactorType _factor_type);
GLenum convert_stencil_operation(Frontend::StencilState::OperationType _operation_type);
GLenum convert_stencil_function(Frontend::StencilState::FunctionType _function_type);
GLenum convert_polygon_mode(Frontend::PolygonState::ModeType _mode_type);
GLenum convert_texture_data_format(Frontend::Texture::DataFormat _data_format);
GLenum convert_texture_data_type(Frontend::Texture::DataFormat _data_format);
GLenum convert_texture_format(Frontend::Texture::DataFormat _data_format);
GLenum convert_primitive_type(Frontend::PrimitiveType _primitive_type);
GLenum convert_texture_wrap(const Frontend::Texture::WrapType _type);
GLenum convert_element_type(Frontend::Buffer::ElementType _element_type);

struct Filter {
  GLuint min;
  GLuint mag;
};

Filter convert_texture_filter(const Frontend::Texture::FilterOptions& _filter_options);

template<typename... Ts>
bool requires_border_color(Ts... _types) {
  return ((_types == GL_CLAMP_TO_BORDER) || ...);
}

struct Attribute {
  GLenum type_enum;
  GLsizei type_size;
  GLsizei components;
  GLsizei instances;
};

Attribute convert_attribute(const Frontend::Buffer::Attribute& _attribute);

constexpr const char* inout_to_string(Frontend::Shader::InOutType _type) {
  switch (_type) {
  case Frontend::Shader::InOutType::F32x4X4:
    return "f32x4x4";
  case Frontend::Shader::InOutType::F32x3X3:
    return "f32x3x3";
  case Frontend::Shader::InOutType::F32x2:
    return "f32x2";
  case Frontend::Shader::InOutType::F32x3:
    return "f32x3";
  case Frontend::Shader::InOutType::F32x4:
    return "f32x4";
  case Frontend::Shader::InOutType::S32x2:
    return "s32x2";
  case Frontend::Shader::InOutType::S32x3:
    return "s32x3";
  case Frontend::Shader::InOutType::S32x4:
    return "s32x4";
  case Frontend::Shader::InOutType::F32:
    return "f32";
  }
  return nullptr;
}

constexpr const char* uniform_to_string(Frontend::Uniform::Type _type) {
  switch (_type) {
  case Frontend::Uniform::Type::SAMPLER1D:
    return "rx_sampler1D";
  case Frontend::Uniform::Type::SAMPLER2D:
    return "rx_sampler2D";
  case Frontend::Uniform::Type::SAMPLER3D:
    return "rx_sampler3D";
  case Frontend::Uniform::Type::SAMPLERCM:
    return "rx_samplerCM";
  case Frontend::Uniform::Type::S32:
    return "s32";
  case Frontend::Uniform::Type::F32:
    return "f32";
  case Frontend::Uniform::Type::S32x2:
    return "s32x2";
  case Frontend::Uniform::Type::S32x3:
    return "s32x3";
  case Frontend::Uniform::Type::S32x4:
    return "s32x4";
  case Frontend::Uniform::Type::F32x2:
    return "f32x2";
  case Frontend::Uniform::Type::F32x3:
    return "f32x3";
  case Frontend::Uniform::Type::F32x4:
    return "f32x4";
  case Frontend::Uniform::Type::F32x4x4:
    return "f32x4x4";
  case Frontend::Uniform::Type::F32x3x3:
    return "f32x3x3";
  case Frontend::Uniform::Type::LB_BONES:
    return "lb_bones";
  case Frontend::Uniform::Type::DQ_BONES:
    return "dq_bones";
  }
  return nullptr;
}

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

} // namespace Rx::Render

#endif // RX_RENDER_BACKEND_GL_H
