#ifndef RX_RENDER_BACKEND_GL_H
#define RX_RENDER_BACKEND_GL_H
#include "rx/render/frontend/state.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"

#include <SDL_opengl.h>

namespace rx::render {

GLenum convert_blend_factor(frontend::blend_state::factor_type _factor_type);
GLenum convert_stencil_operation(frontend::stencil_state::operation_type _operation_type);
GLenum convert_stencil_function(frontend::stencil_state::function_type _function_type);
GLenum convert_polygon_mode(frontend::polygon_state::mode_type _mode_type);
GLenum convert_texture_data_format(frontend::texture::data_format _data_format);
GLenum convert_texture_data_type(frontend::texture::data_format _data_format);
GLenum convert_texture_format(frontend::texture::data_format _data_format);
GLenum convert_primitive_type(frontend::primitive_type _primitive_type);
GLenum convert_texture_wrap(const frontend::texture::wrap_type _type);

struct filter {
  GLuint min;
  GLuint mag;
};

filter convert_texture_filter(const frontend::texture::filter_options& _filter_options);

} // namespace rx::render

#endif // RX_RENDER_BACKEND_GL_H