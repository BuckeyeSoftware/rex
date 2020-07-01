#ifndef RX_RENDER_BACKEND_GL_H
#define RX_RENDER_BACKEND_GL_H
#include "rx/render/frontend/state.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/command.h"

#include <SDL_video.h> // SDL_GL_GetProcAddress
#include <SDL_opengl.h>

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

struct Filter {
  GLuint min;
  GLuint mag;
};

Filter convert_texture_filter(const Frontend::Texture::FilterOptions& _filter_options);

template<typename... Ts>
inline bool requires_border_color(Ts... _types) {
  return ((_types == GL_CLAMP_TO_BORDER) || ...);
}

struct Attribute {
  GLenum type;
  GLsizei count;
};

Attribute convert_attribute(const Frontend::Buffer::Attribute& _attribute);

} // namespace rx::render

#endif // RX_RENDER_BACKEND_GL_H
