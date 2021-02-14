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
GLenum convert_shader_type(Frontend::Shader::Type _type);

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
  case Frontend::Uniform::Type::F32x3x4:
    return "f32x3x4";
  case Frontend::Uniform::Type::LB_BONES:
    return "lb_bones";
  case Frontend::Uniform::Type::DQ_BONES:
    return "dq_bones";
  }
  return nullptr;
}

// Generate the GLSL
Optional<String> generate_glsl(const Vector<Frontend::Uniform>& _uniforms,
  const Frontend::Shader& _shader, int _version, bool _es);

} // namespace Rx::Render

#endif // RX_RENDER_BACKEND_GL_H
