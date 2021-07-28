#include "rx/render/backend/gl.h"
#include "rx/render/backend/es3.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"

#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"

#include "rx/core/profiler.h"
#include "rx/core/log.h"
#include "rx/core/abort.h"

#include "rx/core/memory/zero.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
#include <emscripten/html5.h>
#endif

namespace Rx::Render::Backend {

RX_LOG("render/es3", logger);

// 16MiB buffer slab size for unspecified buffer sizes
static constexpr const Size BUFFER_SLAB_SIZE = 16 << 20;

// buffers
static void (GLAPIENTRYP pglGenBuffers)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteBuffers)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
static void (GLAPIENTRYP pglBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
static void (GLAPIENTRYP pglBindBuffer)(GLenum, GLuint);

// vertex arrays
static void (GLAPIENTRYP pglGenVertexArrays)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteVertexArrays)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglEnableVertexAttribArray)(GLuint);
static void (GLAPIENTRYP pglVertexAttribPointer)(GLuint, GLuint, GLenum, GLboolean, GLsizei, const GLvoid*);
static void (GLAPIENTRYP pglVertexAttribIPointer)(GLuint, GLuint, GLenum, GLsizei, const GLvoid*);
static void (GLAPIENTRYP pglBindVertexArray)(GLuint);
static void (GLAPIENTRYP pglVertexAttribDivisor)(GLuint, GLuint);

// textures
static void (GLAPIENTRYP pglGenTextures)(GLsizei, GLuint* );
static void (GLAPIENTRYP pglDeleteTextures)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglTexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
static void (GLAPIENTRYP pglTexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
static void (GLAPIENTRYP pglTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
static void (GLAPIENTRYP pglTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
static void (GLAPIENTRYP pglCompressedTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
static void (GLAPIENTRYP pglCompressedTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
static void (GLAPIENTRYP pglTexParameteri)(GLenum, GLenum, GLint);
static void (GLAPIENTRYP pglTexParameteriv)(GLenum, GLenum, const GLint*);
static void (GLAPIENTRYP pglTexParameterf)(GLenum, GLenum, GLfloat);
static void (GLAPIENTRYP pglBindTexture)(GLuint, GLuint);
static void (GLAPIENTRYP pglActiveTexture)(GLenum);
static void (GLAPIENTRYP pglPixelStorei)(GLenum, GLint);

// framebuffers
static void (GLAPIENTRYP pglGenFramebuffers)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteFramebuffers)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
static void (GLAPIENTRYP pglBindFramebuffer)(GLenum, GLuint);
static void (GLAPIENTRYP pglDrawBuffers)(GLsizei, const GLenum*);
static void (GLAPIENTRYP pglReadBuffer)(GLenum);
static void (GLAPIENTRYP pglBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);

// shaders and programs
static void (GLAPIENTRYP pglShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
static GLuint (GLAPIENTRYP pglCreateShader)(GLenum);
static void (GLAPIENTRYP pglDeleteShader)(GLuint);
static void (GLAPIENTRYP pglCompileShader)(GLuint);
static void (GLAPIENTRYP pglGetShaderiv)(GLuint, GLenum, GLint*);
static void (GLAPIENTRYP pglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (GLAPIENTRYP pglGetProgramiv)(GLuint, GLenum, GLint*);
static void (GLAPIENTRYP pglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (GLAPIENTRYP pglAttachShader)(GLuint, GLuint );
static void (GLAPIENTRYP pglLinkProgram)(GLuint);
static void (GLAPIENTRYP pglDetachShader)(GLuint, GLuint);
static GLuint (GLAPIENTRYP pglCreateProgram)();
static void (GLAPIENTRYP pglDeleteProgram)(GLuint);
static void (GLAPIENTRYP pglUseProgram)(GLuint);
static GLuint (GLAPIENTRYP pglGetUniformLocation)(GLuint, const GLchar*);
static void (GLAPIENTRYP pglUniform1i)(GLint, GLint);
static void (GLAPIENTRYP pglUniform2iv)(GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglUniform3iv)(GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglUniform4iv)(GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglUniform1fv)(GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglUniform2fv)(GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglUniform3fv)(GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglUniform4fv)(GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglUniformMatrix3x4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglUniformMatrix2x4fv)(GLint, GLsizei, GLboolean, const GLfloat*);

// state
static void (GLAPIENTRYP pglEnable)(GLenum);
static void (GLAPIENTRYP pglDisable)(GLenum);
static void (GLAPIENTRYP pglScissor)(GLint, GLint, GLsizei, GLsizei);
static void (GLAPIENTRYP pglColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
static void (GLAPIENTRYP pglBlendFuncSeparate)( GLenum, GLenum, GLenum, GLenum);
static void (GLAPIENTRYP pglDepthFunc)(GLenum);
static void (GLAPIENTRYP pglDepthMask)(GLboolean);
static void (GLAPIENTRYP pglFrontFace)(GLenum);
static void (GLAPIENTRYP pglCullFace)(GLenum);
static void (GLAPIENTRYP pglStencilMask)(GLuint);
static void (GLAPIENTRYP pglStencilFunc)(GLenum, GLint, GLuint);
static void (GLAPIENTRYP pglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
static void (GLAPIENTRYP pglViewport)(GLint, GLint, GLsizei, GLsizei);
static void (GLAPIENTRYP pglClearBufferfi)(GLenum, GLint, GLfloat, GLint);
static void (GLAPIENTRYP pglClearBufferfv)(GLenum, GLint, const GLfloat*);
static void (GLAPIENTRYP pglClearBufferiv)(GLenum, GLint, const GLint*);

// query
static void (GLAPIENTRYP pglGetIntegerv)(GLenum, GLint*);
static void (GLAPIENTRYP pglGetFloatv)(GLenum, GLfloat*);
static const GLubyte* (GLAPIENTRYP pglGetString)(GLenum);
static const GLubyte* (GLAPIENTRYP pglGetStringi)(GLenum, GLuint);

// draw calls
static void (GLAPIENTRYP pglDrawArrays)(GLenum, GLint, GLsizei);
static void (GLAPIENTRYP pglDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (GLAPIENTRYP pglDrawElements)(GLenum, GLsizei, GLenum, const GLvoid*);
static void (GLAPIENTRYP pglDrawElementsInstanced)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei);

// flush
static void (GLAPIENTRYP pglFinish)(void);

#define GL(...) __VA_ARGS__

static Size setup_attributes(
  const Vector<Frontend::Buffer::Attribute>& attributes,
  Size _stride, Size _index_offset, bool _instanced, Size _offset = 0)
{
  auto is_int_format = [](GLenum _type) {
    return _type == GL_SHORT || _type == GL_INT || _type == GL_UNSIGNED_INT;
  };

  const auto n_attributes = attributes.size();

  Size count = 0;
  for (Size i = 0; i < n_attributes; i++) {
    const auto& attribute = attributes[i];
    const auto index = static_cast<GLuint>(i + _index_offset);
    const auto result = convert_attribute(attribute);

    Size offset = _offset + attribute.offset;
    for (GLsizei j = 0; j < result.instances; j++) {
      GL(pglEnableVertexAttribArray(index + j));
      if (is_int_format(result.type_enum)) {
        GL(pglVertexAttribIPointer(
          index + j,
          result.components,
          result.type_enum,
          _stride,
          reinterpret_cast<const GLvoid*>(offset)));
      } else {
        GL(pglVertexAttribPointer(
          index + j,
          result.components,
          result.type_enum,
          GL_TRUE,
          _stride,
          reinterpret_cast<const GLvoid*>(offset)));
      }
      if (_instanced) {
        GL(pglVertexAttribDivisor(index + j, 1));
      }
      offset += result.type_size * result.components;
      count++;
    }
  }
  return count;
}

void setup_attachments(Frontend::Target* _target, const Frontend::Buffers* _draw_buffers);

template<typename F>
static void fetch(const char* _name, F& function_) {
  auto address = SDL_GL_GetProcAddress(_name);
  logger->verbose("loaded %p '%s'", address, _name);
  *reinterpret_cast<void**>(&function_) = address;
}

namespace detail_es3 {
  struct buffer {
    buffer() {
      GL(pglGenBuffers(3, bo));
      GL(pglGenVertexArrays(1, &va));
    }

    ~buffer() {
      GL(pglDeleteBuffers(3, bo));
      GL(pglDeleteVertexArrays(1, &va));
    }

    GLuint bo[3];
    GLuint va;
    Size elements_size;
    Size vertices_size;
    Size instances_size;
  };

  struct target {
    target()
      : owned{true}
    {
      GL(pglGenFramebuffers(1, &fbo));
    }

    target(GLuint _fbo)
      : fbo{_fbo}
      , owned{false}
    {
    }

    ~target() {
      if (owned) {
        GL(pglDeleteFramebuffers(1, &fbo));
      }
    }

    GLuint fbo;
    bool owned;
    Frontend::Buffers draw_buffers;
    Frontend::Buffers read_buffers;
  };

  struct program {
    program() {
      handle = pglCreateProgram();
    }

    ~program() {
      GL(pglDeleteProgram(handle));
    }

    GLuint handle;
    Vector<GLint> uniforms;
  };

  struct texture1D {
    texture1D() {
      GL(pglGenTextures(1, &tex));
    }

    ~texture1D() {
      GL(pglDeleteTextures(1, &tex));
    }

    GLuint tex;
  };

  struct texture2D {
    texture2D() {
      GL(pglGenTextures(1, &tex));
    }

    ~texture2D() {
      GL(pglDeleteTextures(1, &tex));
    }

    GLuint tex;
  };

  struct texture3D {
    texture3D() {
      GL(pglGenTextures(1, &tex));
    }

    ~texture3D() {
      GL(pglDeleteTextures(1, &tex));
    }

    GLuint tex;
  };

  struct textureCM {
    textureCM() {
      GL(pglGenTextures(1, &tex));
    }

    ~textureCM() {
      GL(pglDeleteTextures(1, &tex));
    }

    GLuint tex;
  };

  struct state
    : Frontend::State
  {
    state(SDL_GLContext _context)
      : m_color_mask{0xff}
      , m_empty_vao{0}
      , m_bound_vbo{0}
      , m_bound_ebo{0}
      , m_bound_vao{0}
      , m_bound_draw_fbo{0}
      , m_bound_read_fbo{0}
      , m_bound_program{0}
      , m_swap_chain_fbo{0}
      , m_active_texture{0}
      , m_context{_context}
    {
      Memory::zero(m_texture_units);

      // There's no unsigned variant of glGetIntegerv
      GLint swap_chain_fbo;
      GL(pglGetIntegerv(GL_FRAMEBUFFER_BINDING, &swap_chain_fbo));
      m_swap_chain_fbo = static_cast<GLuint>(swap_chain_fbo);

      GL(pglEnable(GL_CULL_FACE));

      // These are default in ES 3.0.
      // pglEnable(GL_PROGRAM_POINT_SIZE);
      // pglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      GL(pglCullFace(GL_BACK));
      GL(pglFrontFace(GL_CW));

      GL(pglDepthFunc(GL_LEQUAL));
      // Not supported in ES 3.0
      // GL(pglDisable(GL_MULTISAMPLE));

      GL(pglGenVertexArrays(1, &m_empty_vao));

      use_pixel_store({1, 0, 0});

      const auto vendor{reinterpret_cast<const char*>(pglGetString(GL_VENDOR))};
      const auto renderer{reinterpret_cast<const char*>(pglGetString(GL_RENDERER))};
      const auto version{reinterpret_cast<const char*>(pglGetString(GL_VERSION))};

      logger->info("GL %s %s %s", vendor, version, renderer);

      GLint extensions{0};
      GL(pglGetIntegerv(GL_NUM_EXTENSIONS, &extensions));

      for (GLint i{0}; i < extensions; i++) {
        const auto name{reinterpret_cast<const char*>(pglGetStringi(GL_EXTENSIONS, i))};
        logger->verbose("extension '%s' supported", name);
      }
    }

    ~state() {
      GL(pglDeleteVertexArrays(1, &m_empty_vao));

      SDL_GL_DeleteContext(m_context);
    }

    void use_enable(GLenum _thing, bool _enable) {
      if (_enable) {
        GL(pglEnable(_thing));
      } else {
        GL(pglDisable(_thing));
      }
    }

    void use_pixel_store(const PixelStore& _pixel_store) {
      if (m_pixel_store.unpack_alignment != _pixel_store.unpack_alignment) {
        GL(pglPixelStorei(GL_UNPACK_ALIGNMENT, _pixel_store.unpack_alignment));
      }

      if (m_pixel_store.unpack_row_length != _pixel_store.unpack_row_length) {
        GL(pglPixelStorei(GL_UNPACK_ROW_LENGTH, _pixel_store.unpack_row_length));
      }

      if (m_pixel_store.unpack_image_height != _pixel_store.unpack_image_height) {
        GL(pglPixelStorei(GL_UNPACK_IMAGE_HEIGHT, _pixel_store.unpack_image_height));
      }

      m_pixel_store = _pixel_store;
    }

    void use_state(const Frontend::State* _render_state) {
      RX_PROFILE_CPU("use_state");

      const auto& scissor{_render_state->scissor};
      const auto& blend{_render_state->blend};
      const auto& cull{_render_state->cull};
      const auto& stencil{_render_state->stencil};
      const auto& depth{_render_state->depth};
      const auto& viewport{_render_state->viewport};

      if (this->scissor != scissor) {
        const auto enabled{scissor.enabled()};
        const auto offset{scissor.offset()};
        const auto size{scissor.size()};

        if (this->scissor.enabled() != enabled) {
          use_enable(GL_SCISSOR_TEST, enabled);
          this->scissor.record_enable(enabled);
        }

        if (enabled) {
          if (this->scissor.offset() != offset || this->scissor.size() != size) {
            GL(pglScissor(offset.x, offset.y, size.w, size.h));

            this->scissor.record_offset(offset);
            this->scissor.record_size(size);
          }
        }
      }

      if (this->blend != blend) {
        const auto enabled{blend.enabled()};
        const auto color_src_factor{blend.color_src_factor()};
        const auto color_dst_factor{blend.color_dst_factor()};
        const auto alpha_src_factor{blend.alpha_src_factor()};
        const auto alpha_dst_factor{blend.alpha_dst_factor()};
        const auto write_mask{blend.write_mask()};

        if (this->blend.enabled() != enabled) {
          use_enable(GL_BLEND, enabled);
          this->blend.record_enable(enabled);
        }

        // Write mask can be changed regardless if GL_BLEND is enabled.
        if (this->blend.write_mask() != write_mask) {
          if (write_mask != m_color_mask) {
            const bool r{!!(write_mask & (1 << 0))};
            const bool g{!!(write_mask & (1 << 1))};
            const bool b{!!(write_mask & (1 << 2))};
            const bool a{!!(write_mask & (1 << 3))};
            GL(pglColorMask(r, g, b, a));
            m_color_mask = write_mask;
            this->blend.record_write_mask(write_mask);
          }
        }

        if (enabled) {
          if (this->blend.color_src_factor() != color_src_factor ||
              this->blend.color_dst_factor() != color_dst_factor ||
              this->blend.alpha_src_factor() != alpha_src_factor ||
              this->blend.alpha_dst_factor() != alpha_dst_factor)
          {
            GL(pglBlendFuncSeparate(
              convert_blend_factor(color_src_factor),
              convert_blend_factor(color_dst_factor),
              convert_blend_factor(alpha_src_factor),
              convert_blend_factor(alpha_dst_factor)));

            this->blend.record_color_blend_factors(color_src_factor, color_dst_factor);
            this->blend.record_alpha_blend_factors(alpha_src_factor, alpha_dst_factor);
          }
        }
      }

      if (this->depth != depth) {
        const auto test{depth.test()};
        const auto write{depth.write()};

        if (this->depth.test() != test) {
          use_enable(GL_DEPTH_TEST, test);
          this->depth.record_test(test);
        }

        if (test) {
          if (this->depth.write() != write) {
            GL(pglDepthMask(write ? GL_TRUE : GL_FALSE));
            this->depth.record_write(write);
          }
        }
      }

      if (this->cull != cull) {
        const auto front_face{cull.front_face()};
        const auto cull_face{cull.cull_face()};
        const auto enabled{cull.enabled()};

        if (this->cull.enabled() != enabled) {
          use_enable(GL_CULL_FACE, enabled);
          this->cull.record_enable(enabled);
        }

        if (enabled) {
          if (this->cull.front_face() != front_face) {
            switch (front_face) {
            case Frontend::CullState::FrontFaceType::CLOCK_WISE:
              GL(pglFrontFace(GL_CW));
              break;
            case Frontend::CullState::FrontFaceType::COUNTER_CLOCK_WISE:
              GL(pglFrontFace(GL_CCW));
              break;
            }
            this->cull.record_front_face(front_face);
          }

          if (this->cull.cull_face() != cull_face) {
            switch (cull_face) {
            case Frontend::CullState::CullFaceType::FRONT:
              GL(pglCullFace(GL_FRONT));
              break;
            case Frontend::CullState::CullFaceType::BACK:
              GL(pglCullFace(GL_BACK));
              break;
            }
            this->cull.record_cull_face(cull_face);
          }
        }
      }

      if (this->stencil != stencil) {
        const auto enabled{stencil.enabled()};
        const auto write_mask{stencil.write_mask()};
        const auto function{stencil.function()};
        const auto reference{stencil.reference()};
        const auto mask{stencil.mask()};
        const auto front_fail_action{stencil.front_fail_action()};
        const auto front_depth_fail_action{stencil.front_depth_fail_action()};
        const auto front_depth_pass_action{stencil.front_depth_pass_action()};
        const auto back_fail_action{stencil.back_fail_action()};
        const auto back_depth_fail_action{stencil.back_depth_fail_action()};
        const auto back_depth_pass_action{stencil.back_depth_pass_action()};

        if (this->stencil.enabled() != enabled) {
          use_enable(GL_STENCIL_TEST, enabled);
          this->stencil.record_enable(enabled);
        }

        if (enabled) {
          if (this->stencil.write_mask() != write_mask) {
            GL(pglStencilMask(write_mask));
            this->stencil.record_write_mask(write_mask);
          }

          if (this->stencil.function() != function ||
              this->stencil.reference() != reference ||
              this->stencil.mask() != mask)
          {
            GL(pglStencilFunc(
              convert_stencil_function(function),
              static_cast<GLint>(reference),
              static_cast<GLuint>(mask)));

            this->stencil.record_function(function);
            this->stencil.record_reference(reference);
            this->stencil.record_mask(mask);
          }

          if (this->stencil.front_fail_action() != front_fail_action ||
              this->stencil.front_depth_fail_action() != front_depth_fail_action ||
              this->stencil.front_depth_pass_action() != front_depth_pass_action)
          {
            GL(pglStencilOpSeparate(
              GL_FRONT,
              convert_stencil_operation(front_fail_action),
              convert_stencil_operation(front_depth_fail_action),
              convert_stencil_operation(front_depth_pass_action)));

            this->stencil.record_front_fail_action(front_fail_action);
            this->stencil.record_front_depth_fail_action(front_depth_fail_action);
            this->stencil.record_front_depth_pass_action(front_depth_pass_action);
          }

          if (this->stencil.back_fail_action() != back_fail_action ||
              this->stencil.back_depth_fail_action() != back_depth_fail_action ||
              this->stencil.back_depth_pass_action() != back_depth_pass_action)
          {
            GL(pglStencilOpSeparate(
              GL_BACK,
              convert_stencil_operation(back_fail_action),
              convert_stencil_operation(back_depth_fail_action),
              convert_stencil_operation(back_depth_pass_action)));

            this->stencil.record_back_fail_action(back_fail_action);
            this->stencil.record_back_depth_fail_action(back_depth_fail_action);
            this->stencil.record_back_depth_pass_action(back_depth_pass_action);
          }
        }
      }

      if (this->viewport != viewport) {
        const auto& offset{viewport.offset().cast<GLuint>()};
        const auto& dimensions{viewport.dimensions().cast<GLsizei>()};
        GL(pglViewport(offset.x, offset.y, dimensions.w, dimensions.h));
        this->viewport.record_offset(viewport.offset());
        this->viewport.record_dimensions(viewport.dimensions());
      }

      // flush all changes to this for updated hash
      flush();
    }

    void use_draw_target(Frontend::Target* _render_target, const Frontend::Buffers* _draw_buffers) {
      RX_PROFILE_CPU("use_draw_target");

      auto this_target{reinterpret_cast<target*>(_render_target + 1)};
      if (m_bound_draw_fbo != this_target->fbo) {
        GL(pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, this_target->fbo));
        m_bound_draw_fbo = this_target->fbo;
      }

      // Changing draw buffers?
      if (_draw_buffers && !_render_target->is_swapchain()) {
        // The draw buffers changed.
        if (this_target->draw_buffers != *_draw_buffers) {
          if (_draw_buffers->is_empty()) {
            // Calling DrawBuffers with 0 is the same as setting all draw
            // buffers to GL_NONE.
            GL(pglDrawBuffers(0, nullptr));
          } else {
            // Change FBO attachments around to be the same as the order inside
            // |_draw_buffers|. This is only necessary in ES 3.0 since only
            // iota'd DrawBuffers is allowed.
            setup_attachments(_render_target, _draw_buffers);

            GLenum draw_buffers[Frontend::Buffers::MAX_BUFFERS];
            const auto n_attachments = _draw_buffers->size();
            for (Size i = 0; i < n_attachments; i++) {
              draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
            }

            GL(pglDrawBuffers(n_attachments, draw_buffers));
          }
          this_target->draw_buffers = *_draw_buffers;
        }
      }
    }

    void use_read_target(Frontend::Target* _render_target, const Frontend::Buffers* _read_buffers) {
      RX_PROFILE_CPU("use_read_target");

      auto this_target{reinterpret_cast<target*>(_render_target + 1)};
      if (m_bound_read_fbo != this_target->fbo) {
        GL(pglBindFramebuffer(GL_READ_FRAMEBUFFER, this_target->fbo));
        m_bound_read_fbo = this_target->fbo;
      }

      // Changing read buffer?
      if (_read_buffers && !_render_target->is_swapchain()) {
        // The read buffer changed.
        if (this_target->read_buffers != *_read_buffers) {
          if (_read_buffers->is_empty()) {
            GL(pglReadBuffer(GL_NONE));
          } else {
            GL(pglReadBuffer(GL_COLOR_ATTACHMENT0 + _read_buffers->last()));
          }
        }
        this_target->read_buffers = *_read_buffers;
      }
    }

    void use_program(const Frontend::Program* _render_program) {
      RX_PROFILE_CPU("use_program");

      const auto this_program{reinterpret_cast<const program*>(_render_program + 1)};
      if (this_program->handle != m_bound_program) {
        GL(pglUseProgram(this_program->handle));
        m_bound_program = this_program->handle;
      }
    }

    void use_buffer(const Frontend::Buffer* _render_buffer) {
      RX_PROFILE_CPU("use_buffer");
      if (_render_buffer) {
        const auto this_buffer{reinterpret_cast<const buffer*>(_render_buffer + 1)};
        if (this_buffer->va != m_bound_vao) {
          GL(pglBindVertexArray(this_buffer->va));
          m_bound_vao = this_buffer->va;
        }
      } else if (!m_bound_vao) {
        GL(pglBindVertexArray(m_empty_vao));
        m_bound_vao = m_empty_vao;
      }
    }

    void use_vbo(GLuint _vbo) {
      RX_PROFILE_CPU("use_vbo");

      if (m_bound_vbo != _vbo) {
        GL(pglBindBuffer(GL_ARRAY_BUFFER, _vbo));
        m_bound_vbo = _vbo;
      }
    }

    void use_ebo(GLuint _ebo) {
      RX_PROFILE_CPU("use_ebo");

      if (m_bound_ebo != _ebo) {
        GL(pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo));
        m_bound_ebo = _ebo;
      }
    }

    struct texture_unit {
      GLuint texture1D;
      GLuint texture2D;
      GLuint texture3D;
      GLuint textureCM;
    };

    template<GLuint texture_unit::*name, typename Bt, typename Ft>
    void use_texture_template(GLenum _type, const Ft* _render_texture) {
      RX_PROFILE_CPU("use_texture");

      const auto this_texture{reinterpret_cast<const Bt*>(_render_texture + 1)};
      auto& texture_unit{m_texture_units[m_active_texture]};
      if (texture_unit.*name != this_texture->tex) {
        texture_unit.*name = this_texture->tex;
        GL(pglBindTexture(_type, this_texture->tex));
      }
    }

    template<GLuint texture_unit::*name, typename Bt, typename Ft>
    void use_active_texture_template(GLenum _type, const Ft* _render_texture, Size _unit) {
      const auto this_texture{reinterpret_cast<const Bt*>(_render_texture + 1)};
      auto& texture_unit{m_texture_units[_unit]};
      if (texture_unit.*name != this_texture->tex) {
        if (m_active_texture != _unit) {
          GL(pglActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + _unit)));
          m_active_texture = _unit;
        }
        texture_unit.*name = this_texture->tex;
        GL(pglBindTexture(_type, this_texture->tex));
      }
    }

    template<typename Ft, typename Bt, GLuint texture_unit::*name>
    void invalidate_texture_template(const Ft* _render_texture) {
      const auto this_texture{reinterpret_cast<const Bt*>(_render_texture + 1)};
      for (auto& texture_unit : m_texture_units) {
        if (texture_unit.*name == this_texture->tex) {
          texture_unit.*name = 0;
        }
      }
    }

    void use_active_texture(const Frontend::Texture1D* _render_texture, Size _unit) {
      use_active_texture_template<&texture_unit::texture1D, detail_es3::texture1D>(GL_TEXTURE_1D, _render_texture, _unit);
    }

    void use_active_texture(const Frontend::Texture2D* _render_texture, Size _unit) {
      use_active_texture_template<&texture_unit::texture2D, detail_es3::texture2D>(GL_TEXTURE_2D, _render_texture, _unit);
    }

    void use_active_texture(const Frontend::Texture3D* _render_texture, Size _unit) {
      use_active_texture_template<&texture_unit::texture1D, detail_es3::texture1D>(GL_TEXTURE_3D, _render_texture, _unit);
    }

    void use_active_texture(const Frontend::TextureCM* _render_texture, Size _unit) {
      use_active_texture_template<&texture_unit::textureCM, detail_es3::textureCM>(GL_TEXTURE_CUBE_MAP, _render_texture, _unit);
    }

    void use_texture(const Frontend::Texture1D* _render_texture) {
      use_texture_template<&texture_unit::texture1D, detail_es3::texture1D>(GL_TEXTURE_1D, _render_texture);
    }

    void use_texture(const Frontend::Texture2D* _render_texture) {
      use_texture_template<&texture_unit::texture2D, detail_es3::texture2D>(GL_TEXTURE_2D, _render_texture);
    }

    void use_texture(const Frontend::Texture3D* _render_texture) {
      use_texture_template<&texture_unit::texture3D, detail_es3::texture3D>(GL_TEXTURE_3D, _render_texture);
    }

    void use_texture(const Frontend::TextureCM* _render_texture) {
      use_texture_template<&texture_unit::textureCM, detail_es3::textureCM>(GL_TEXTURE_CUBE_MAP, _render_texture);
    }

    void invalidate_texture(const Frontend::Texture1D* _render_texture) {
      invalidate_texture_template<Frontend::Texture1D, texture1D, &texture_unit::texture1D>(_render_texture);
    }

    void invalidate_texture(const Frontend::Texture2D* _render_texture) {
      invalidate_texture_template<Frontend::Texture2D, texture2D, &texture_unit::texture2D>(_render_texture);
    }

    void invalidate_texture(const Frontend::Texture3D* _render_texture) {
      invalidate_texture_template<Frontend::Texture3D, texture3D, &texture_unit::texture3D>(_render_texture);
    }

    void invalidate_texture(const Frontend::TextureCM* _render_texture) {
      invalidate_texture_template<Frontend::TextureCM, textureCM, &texture_unit::textureCM>(_render_texture);
    }

    Uint8 m_color_mask;

    GLuint m_empty_vao;

    GLuint m_bound_vbo;
    GLuint m_bound_ebo;
    GLuint m_bound_vao;
    GLuint m_bound_draw_fbo;
    GLuint m_bound_read_fbo;
    GLuint m_bound_program;

    GLuint m_swap_chain_fbo;
    texture_unit m_texture_units[Frontend::Textures::MAX_TEXTURES];
    Size m_active_texture;

    SDL_GLContext m_context;

    PixelStore m_pixel_store;
  };
};

// ES 3.0 requires draw buffers are iota, so use |_draw_buffers| to select
// attachments and wire them into the FBO in iota order.
//
// TODO(dweiler): Consider creating an FBO LRU cache so this doesn't need to
// occur everytime |_draw_buffers| differes for the last configuration of
// |_target|.
void setup_attachments(Frontend::Target* _target, const Frontend::Buffers* _draw_buffers) {
  const auto& attachments = _target->attachments();

  // Don't configure more than the number of draw buffers given.
  const Size n_attachments = attachments.size();
  const Size n_count = _draw_buffers
    ? Algorithm::min(n_attachments, _draw_buffers->size()) : n_attachments;

  for (Size i = 0; i < n_count; i++) {
    // Select attachments based on |_draw_buffers| but always put them in
    // iota'd GL_COLOR_ATTACHMENT order.
    const auto& attachment = attachments[_draw_buffers ? (*_draw_buffers)[i] : i];
    const auto attachment_enum = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
    switch (attachment.kind) {
    case Frontend::Target::Attachment::Type::TEXTURE2D:
      GL(pglFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        attachment_enum,
        GL_TEXTURE_2D,
        reinterpret_cast<detail_es3::texture2D*>(attachment.as_texture2D.texture + 1)->tex,
        static_cast<GLint>(attachment.level)));
      break;
    case Frontend::Target::Attachment::Type::TEXTURECM:
      GL(pglFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        attachment_enum,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(attachment.as_textureCM.face),
        reinterpret_cast<detail_es3::textureCM*>(attachment.as_textureCM.texture + 1)->tex,
        static_cast<GLint>(attachment.level)));
      break;
    }
  }

  // Remainder attachments should be detached from the FBO to prevent feedback.
  for (Size i = n_count; i < n_attachments; i++) {
    const auto& attachment = attachments[i];
    const auto attachment_enum = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
    switch (attachment.kind) {
    case Frontend::Target::Attachment::Type::TEXTURE2D:
      GL(pglFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        attachment_enum,
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(attachment.level)));
      break;
    case Frontend::Target::Attachment::Type::TEXTURECM:
      GL(pglFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        attachment_enum,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(attachment.as_textureCM.face),
        0,
        static_cast<GLint>(attachment.level)));
      break;
    }
  }

  // check_target(_target, _draw_buffers);
}

static GLuint compile_shader(Memory::Allocator& _allocator,
  Vector<Frontend::Uniform>& _uniforms, const Frontend::Shader& _shader)
{
  auto contents = generate_glsl(_allocator, _uniforms, _shader, 300, true);

  auto data = static_cast<const GLchar*>(contents->data());
  auto size = static_cast<GLint>(contents->size());

  GLuint handle = pglCreateShader(convert_shader_type(_shader.kind));
  GL(pglShaderSource(handle, 1, &data, &size));
  GL(pglCompileShader(handle));

  GLint status{0};
  GL(pglGetShaderiv(handle, GL_COMPILE_STATUS, &status));
  if (status != GL_TRUE) {
    GLint log_size{0};
    GL(pglGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_size));

    logger->error("failed compiling shader");

    if (log_size) {
      Vector<char> error_log{_allocator};
      if (!error_log.resize(log_size)) {
        logger->error("out of memory");
      } else {
        GL(pglGetShaderInfoLog(handle, log_size, &log_size, error_log.data()));
        logger->error("\n%s\n%s", error_log.data(), contents->data());
      }
    }

    GL(pglDeleteShader(handle));
    return 0;
  }

  return handle;
}

AllocationInfo ES3::query_allocation_info() const {
  AllocationInfo info;
  info.buffer_size = sizeof(detail_es3::buffer);
  info.target_size = sizeof(detail_es3::target);
  info.program_size = sizeof(detail_es3::program);
  info.texture1D_size = sizeof(detail_es3::texture1D);
  info.texture2D_size = sizeof(detail_es3::texture2D);
  info.texture3D_size = sizeof(detail_es3::texture3D);
  info.textureCM_size = sizeof(detail_es3::textureCM);
  return info;
}

DeviceInfo ES3::query_device_info() const {
  return {
    reinterpret_cast<const char*>(pglGetString(GL_VENDOR)),
    reinterpret_cast<const char*>(pglGetString(GL_RENDERER)),
    reinterpret_cast<const char*>(pglGetString(GL_VERSION))
  };
}

ES3::ES3(Memory::Allocator& _allocator, void* _data)
  : m_allocator{_allocator}
  , m_data{_data}
{
}

ES3::~ES3() {
  m_allocator.destroy<detail_es3::state>(m_impl);
}

bool ES3::init() {
  SDL_GLContext context =
    SDL_GL_CreateContext(reinterpret_cast<SDL_Window*>(m_data));
  if (!context) {
    return false;
  }

  // Enable some WebGL extensions after the GL context is created.
#if defined(RX_PLATFORM_EMSCRIPTEN)
  auto webgl = emscripten_webgl_get_current_context();
  auto enable = [&](const char* _name) {
    if (emscripten_webgl_enable_extension(webgl, _name)) {
      logger->verbose("WebGL extension: %s ENABLED", _name);
    } else {
      logger->warning("WebGL extension: %s UNSUPPORTED", _name);
    }
  };

  // F32 RTTs
  enable("EXT_color_buffer_float");

  // F16 RTTs
  enable("EXT_color_buffer_half_float");

  // Linear filtering of F16 RTTs
  enable("OES_texture_float_linear");
#endif

  // buffers
  fetch("glGenBuffers", pglGenBuffers);
  fetch("glDeleteBuffers", pglDeleteBuffers);
  fetch("glBufferData", pglBufferData);
  fetch("glBufferSubData", pglBufferSubData);
  fetch("glBindBuffer", pglBindBuffer);

  // vertex arrays
  fetch("glGenVertexArrays", pglGenVertexArrays);
  fetch("glDeleteVertexArrays", pglDeleteVertexArrays);
  fetch("glEnableVertexAttribArray", pglEnableVertexAttribArray);
  fetch("glVertexAttribPointer", pglVertexAttribPointer);
  fetch("glVertexAttribIPointer", pglVertexAttribIPointer);
  fetch("glBindVertexArray", pglBindVertexArray);
  fetch("glVertexAttribDivisor", pglVertexAttribDivisor);

  // textures
  fetch("glGenTextures", pglGenTextures);
  fetch("glDeleteTextures", pglDeleteTextures);
  fetch("glTexStorage2D", pglTexStorage2D);
  fetch("glTexStorage3D", pglTexStorage3D);
  fetch("glTexSubImage2D", pglTexSubImage2D);
  fetch("glTexSubImage3D", pglTexSubImage3D);
  fetch("glCompressedTexSubImage2D", pglCompressedTexSubImage2D);
  fetch("glCompressedTexSubImage3D", pglCompressedTexSubImage3D);
  fetch("glTexParameteri", pglTexParameteri);
  fetch("glTexParameteriv", pglTexParameteriv);
  fetch("glTexParameterf", pglTexParameterf);
  fetch("glBindTexture", pglBindTexture);
  fetch("glActiveTexture", pglActiveTexture);
  fetch("glPixelStorei", pglPixelStorei);

  // frame buffers
  fetch("glGenFramebuffers", pglGenFramebuffers);
  fetch("glDeleteFramebuffers", pglDeleteFramebuffers);
  fetch("glFramebufferTexture2D", pglFramebufferTexture2D);
  fetch("glBindFramebuffer", pglBindFramebuffer);
  fetch("glDrawBuffers", pglDrawBuffers);
  fetch("glReadBuffer", pglReadBuffer);
  fetch("glBlitFramebuffer", pglBlitFramebuffer);
  fetch("glClearBufferfv", pglClearBufferfv);
  fetch("glClearBufferiv", pglClearBufferiv);
  fetch("glClearBufferfi", pglClearBufferfi);

  // shaders and programs
  fetch("glShaderSource", pglShaderSource);
  fetch("glCreateShader", pglCreateShader);
  fetch("glDeleteShader", pglDeleteShader);
  fetch("glCompileShader", pglCompileShader);
  fetch("glGetShaderiv", pglGetShaderiv);
  fetch("glGetShaderInfoLog", pglGetShaderInfoLog);
  fetch("glGetProgramiv", pglGetProgramiv);
  fetch("glGetProgramInfoLog", pglGetProgramInfoLog);
  fetch("glAttachShader", pglAttachShader);
  fetch("glLinkProgram", pglLinkProgram);
  fetch("glDetachShader", pglDetachShader);
  fetch("glCreateProgram", pglCreateProgram);
  fetch("glDeleteProgram", pglDeleteProgram);
  fetch("glUseProgram", pglUseProgram);
  fetch("glGetUniformLocation", pglGetUniformLocation);
  fetch("glUniform1i", pglUniform1i);
  fetch("glUniform2iv", pglUniform2iv);
  fetch("glUniform3iv", pglUniform3iv);
  fetch("glUniform4iv", pglUniform4iv);
  fetch("glUniform1fv", pglUniform1fv);
  fetch("glUniform2fv", pglUniform2fv);
  fetch("glUniform3fv", pglUniform3fv);
  fetch("glUniform4fv", pglUniform4fv);
  fetch("glUniformMatrix3fv", pglUniformMatrix3fv);
  fetch("glUniformMatrix4fv", pglUniformMatrix4fv);
  fetch("glUniformMatrix3x4fv", pglUniformMatrix3x4fv);
  fetch("glUniformMatrix2x4fv", pglUniformMatrix2x4fv);

  // state
  fetch("glEnable", pglEnable);
  fetch("glDisable", pglDisable);
  fetch("glScissor", pglScissor);
  fetch("glColorMask", pglColorMask);
  fetch("glBlendFuncSeparate", pglBlendFuncSeparate);
  fetch("glDepthFunc", pglDepthFunc);
  fetch("glDepthMask", pglDepthMask);
  fetch("glFrontFace", pglFrontFace);
  fetch("glCullFace", pglCullFace);
  fetch("glStencilMask", pglStencilMask);
  fetch("glStencilFunc", pglStencilFunc);
  fetch("glStencilOpSeparate", pglStencilOpSeparate);
  fetch("glViewport", pglViewport);

  // query
  fetch("glGetIntegerv", pglGetIntegerv);
  fetch("glGetFloatv", pglGetFloatv);
  fetch("glGetString", pglGetString);
  fetch("glGetStringi", pglGetStringi);

  // draw calls
  fetch("glDrawArrays", pglDrawArrays);
  fetch("glDrawArraysInstanced", pglDrawArraysInstanced);
  fetch("glDrawElements", pglDrawElements);
  fetch("glDrawElementsInstanced", pglDrawElementsInstanced);

  // flush
  fetch("glFinish", pglFinish);

  m_impl = m_allocator.create<detail_es3::state>(context);

  return m_impl != nullptr;
}

void ES3::process(const Vector<Byte*>& _commands) {
  _commands.each_fwd([this](Byte* _command) {
    process(_command);
  });
}

void ES3::process(Byte* _command) {
  RX_PROFILE_CPU("ES3::process");

  auto state{reinterpret_cast<detail_es3::state*>(m_impl)};
  auto header{reinterpret_cast<Frontend::CommandHeader*>(_command)};
  switch (header->type) {
  case Frontend::CommandType::RESOURCE_ALLOCATE:
    {
      const auto resource{reinterpret_cast<const Frontend::ResourceCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::ResourceCommand::Type::BUFFER:
        Utility::construct<detail_es3::buffer>(resource->as_buffer + 1);
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        {
          const auto render_target = resource->as_target;
          if (render_target->is_swapchain()) {
            Utility::construct<detail_es3::target>(resource->as_target + 1, state->m_swap_chain_fbo);
          } else {
            Utility::construct<detail_es3::target>(resource->as_target + 1);
          }
        }
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        Utility::construct<detail_es3::program>(resource->as_program + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE1D:
        Utility::construct<detail_es3::texture1D>(resource->as_texture1D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE2D:
        if (resource->as_texture2D->is_swapchain()) {
          break;
        }
        Utility::construct<detail_es3::texture2D>(resource->as_texture2D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        Utility::construct<detail_es3::texture3D>(resource->as_texture3D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        Utility::construct<detail_es3::textureCM>(resource->as_textureCM + 1);
        break;
      case Frontend::ResourceCommand::Type::DOWNLOADER:
        // TODO(dweiler): Implement.
        break;
      }
    }
    break;
  case Frontend::CommandType::RESOURCE_DESTROY:
    {
      const auto resource{reinterpret_cast<const Frontend::ResourceCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::ResourceCommand::Type::BUFFER:
        {
          auto buffer = reinterpret_cast<detail_es3::buffer*>(resource->as_buffer + 1);
          if (state->m_bound_vbo == buffer->bo[0]) {
            state->m_bound_vbo = 0;
          }
          if (state->m_bound_ebo == buffer->bo[1]) {
            state->m_bound_ebo = 0;
          }
          if (state->m_bound_vao == buffer->va) {
            state->m_bound_vao = 0;
          }
          Utility::destruct<detail_es3::buffer>(resource->as_buffer + 1);
        }
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        {
          auto target{reinterpret_cast<detail_es3::target*>(resource->as_target + 1)};
          if (state->m_bound_draw_fbo == target->fbo) {
            state->m_bound_draw_fbo = 0;
          }
          if (state->m_bound_read_fbo == target->fbo) {
            state->m_bound_read_fbo = 0;
          }
          Utility::destruct<detail_es3::target>(resource->as_target + 1);
        }
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        Utility::destruct<detail_es3::program>(resource->as_program + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE1D:
        state->invalidate_texture(resource->as_texture1D);
        Utility::destruct<detail_es3::texture1D>(resource->as_texture1D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE2D:
        if (resource->as_texture2D->is_swapchain()) {
          break;
        }
        state->invalidate_texture(resource->as_texture2D);
        Utility::destruct<detail_es3::texture2D>(resource->as_texture2D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        state->invalidate_texture(resource->as_texture3D);
        Utility::destruct<detail_es3::texture3D>(resource->as_texture3D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        state->invalidate_texture(resource->as_textureCM);
        Utility::destruct<detail_es3::textureCM>(resource->as_textureCM + 1);
        break;
      case Frontend::ResourceCommand::Type::DOWNLOADER:
        // TODO(dweiler): Implement.
        break;
      }
    }
    break;
  case Frontend::CommandType::RESOURCE_CONSTRUCT:
    {
      const auto resource{reinterpret_cast<const Frontend::ResourceCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::ResourceCommand::Type::BUFFER:
        {
          auto render_buffer = resource->as_buffer;
          const auto& format = render_buffer->format();
          auto buffer = reinterpret_cast<detail_es3::buffer*>(render_buffer + 1);

          const auto type = render_buffer->type() == Frontend::Buffer::Type::DYNAMIC
            ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

          state->use_buffer(render_buffer);

          Size current_attribute = 0;

          // Setup element buffer.
          if (format.is_indexed()) {
            const auto& elements = render_buffer->elements();
            state->use_ebo(buffer->bo[0]);
            if (elements.is_empty()) {
              GL(pglBufferData(GL_ELEMENT_ARRAY_BUFFER, BUFFER_SLAB_SIZE, nullptr, type));
              buffer->elements_size = BUFFER_SLAB_SIZE;
            } else {
              GL(pglBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size(), elements.data(), type));
              buffer->elements_size = elements.size();
            }
          }

          // Setup vertex buffer and attributes.
          const auto& vertices = render_buffer->vertices();
          state->use_vbo(buffer->bo[1]);
          if (vertices.is_empty()) {
            GL(pglBufferData(GL_ARRAY_BUFFER, BUFFER_SLAB_SIZE, nullptr, type));
            buffer->vertices_size = BUFFER_SLAB_SIZE;
          } else {
            GL(pglBufferData(GL_ARRAY_BUFFER, vertices.size(), vertices.data(), type));
            buffer->vertices_size = vertices.size();
          }
          current_attribute = setup_attributes(
            format.vertex_attributes(),
            format.vertex_stride(),
            current_attribute,
            false);

          // Setup instance buffer and attributes.
          if (format.is_instanced()) {
            const auto& instances = render_buffer->instances();
            state->use_vbo(buffer->bo[2]);
            if (instances.is_empty()) {
              GL(pglBufferData(GL_ARRAY_BUFFER, BUFFER_SLAB_SIZE, nullptr, type));
              buffer->instances_size = BUFFER_SLAB_SIZE;
            } else {
              GL(pglBufferData(GL_ARRAY_BUFFER, instances.size(), instances.data(), type));
              buffer->instances_size = instances.size();
            }
            current_attribute = setup_attributes(
              format.instance_attributes(),
              format.instance_stride(),
              current_attribute,
              true);
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        {
          const auto render_target = resource->as_target;
          if (render_target->is_swapchain()) {
            // Swap chain targets don't have an user-defined attachments.
            break;
          }

          state->use_draw_target(render_target, nullptr);

          if (render_target->has_depth_stencil()) {
            const auto depth_stencil{render_target->depth_stencil()};
            // combined depth stencil format
            const auto texture{reinterpret_cast<const detail_es3::texture2D*>(depth_stencil + 1)};
            GL(pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->tex, 0));
          } else if (render_target->has_depth()) {
            const auto depth{render_target->depth()};
            const auto texture{reinterpret_cast<const detail_es3::texture2D*>(depth + 1)};
            GL(pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->tex, 0));
          } else if (render_target->has_stencil()) {
            const auto stencil{render_target->stencil()};
            const auto texture{reinterpret_cast<const detail_es3::texture2D*>(stencil + 1)};
            GL(pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->tex, 0));
          }
          // NOTE(weiler): We don't setup draw buffers here because ES 3.0
          // requires that draw buffers are an iota of GL_COLOR_ATTACHMENT
          // values which is not possible. Instead we setup the attachments
          // in order of the draw buffers at each draw call.
        }
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        {
          const auto render_program{resource->as_program};
          const auto program{reinterpret_cast<detail_es3::program*>(render_program + 1)};

          const auto& shaders = render_program->shaders();

          Vector<GLuint> shader_handles{m_allocator};
          shaders.each_fwd([&](const Frontend::Shader& _shader) {
            GLuint shader_handle{compile_shader(m_allocator, render_program->uniforms(), _shader)};
            if (shader_handle != 0) {
              GL(pglAttachShader(program->handle, shader_handle));
              shader_handles.push_back(shader_handle);
            }
          });

          GL(pglLinkProgram(program->handle));

          GLint status{0};
          GL(pglGetProgramiv(program->handle, GL_LINK_STATUS, &status));
          if (status != GL_TRUE) {
            GLint log_size{0};
            GL(pglGetProgramiv(program->handle, GL_INFO_LOG_LENGTH, &log_size));

            logger->error("failed linking program");

            if (log_size) {
              Vector<char> error_log{Memory::SystemAllocator::instance()};
              if (!error_log.resize(log_size)) {
                logger->error("out of memory");
              } else {
                GL(pglGetProgramInfoLog(program->handle, log_size, &log_size, error_log.data()));
                logger->error("\n%s", error_log.data());
              }
            }
          }

          shader_handles.each_fwd([&](GLuint _shader) {
            GL(pglDetachShader(program->handle, _shader));
            GL(pglDeleteShader(_shader));
          });

          // fetch uniform locations
          render_program->uniforms().each_fwd([program](const Frontend::Uniform& _uniform) {
            if (_uniform.is_padding()) {
              // Padding uniforms have index -1.
              program->uniforms.push_back(-1);
            } else {
              program->uniforms.push_back(pglGetUniformLocation(program->handle, _uniform.name().data()));
            }
          });
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURE1D:
        {
          const auto render_texture{resource->as_texture1D};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          state->use_texture(render_texture);

          // 1D textures are emulated on 2D.
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter.min));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter.mag));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1));
          if (requires_border_color(wrap_s)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            GL(pglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color.data()));
          }

          GL(pglTexStorage2D(
            GL_TEXTURE_2D,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions),
            1));

          if (data.size()) {
            for (GLint i = 0; i < levels; i++) {
              const auto& level_info = render_texture->info_for_level(i);
              const auto pixels = data.data() + level_info.offset;
              const auto alignment = texture_alignment(pixels, level_info.dimensions, render_texture);

              state->use_pixel_store({alignment, 0, 0});

              if (render_texture->is_compressed_format()) {
                GL(pglCompressedTexSubImage2D(
                  GL_TEXTURE_2D,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions),
                  1,
                  convert_texture_data_format(format),
                  level_info.size,
                  pixels));
              } else {
                GL(pglTexSubImage2D(
                  GL_TEXTURE_2D,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions),
                  1,
                  convert_texture_format(format),
                  convert_texture_data_type(format),
                  pixels));
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURE2D:
        {
          const auto render_texture{resource->as_texture2D};
          if (render_texture->is_swapchain()) {
            break;
          }

          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          state->use_texture(render_texture);

          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter.min));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter.mag));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
          GL(pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1));
          if (requires_border_color(wrap_s, wrap_t)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            GL(pglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color.data()));
          }

          GL(pglTexStorage2D(
            GL_TEXTURE_2D,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h)));

          if (data.size()) {
            for (GLint i = 0; i < levels; i++) {
              const auto& level_info = render_texture->info_for_level(i);
              const auto pixels = data.data() + level_info.offset;
              const auto alignment = texture_alignment(pixels, level_info.dimensions.w, render_texture);

              state->use_pixel_store({alignment, 0, 0});

              if (render_texture->is_compressed_format()) {
                GL(pglCompressedTexSubImage2D(
                  GL_TEXTURE_2D,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  convert_texture_data_format(format),
                  level_info.size,
                  pixels));
              } else {
                GL(pglTexSubImage2D(
                  GL_TEXTURE_2D,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  convert_texture_format(format),
                  convert_texture_data_type(format),
                  pixels));
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        {
          const auto render_texture{resource->as_texture3D};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto wrap_r{convert_texture_wrap(wrap.p)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          state->use_texture(render_texture);

          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter.min));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter.mag));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap_s));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap_t));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap_r));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0));
          GL(pglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, levels - 1));
          if (requires_border_color(wrap_s, wrap_t, wrap_r)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            GL(pglTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color.data()));
          }

          GL(pglTexStorage3D(
            GL_TEXTURE_3D,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h),
            static_cast<GLsizei>(dimensions.d)));

          if (data.size()) {
            for (GLint i = 0; i < levels; i++) {
              const auto& level_info = render_texture->info_for_level(i);
              const auto pixels = data.data() + level_info.offset;
              const auto alignment = texture_alignment(pixels, level_info.dimensions.w, render_texture);

              state->use_pixel_store({alignment, 0, 0});

              if (render_texture->is_compressed_format()) {
                GL(pglCompressedTexSubImage3D(
                  GL_TEXTURE_3D,
                  i,
                  0,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  static_cast<GLsizei>(level_info.dimensions.d),
                  convert_texture_data_format(format),
                  level_info.size,
                  pixels));
              } else {
                GL(pglTexSubImage3D(
                  GL_TEXTURE_3D,
                  i,
                  0,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  static_cast<GLsizei>(level_info.dimensions.d),
                  convert_texture_format(format),
                  convert_texture_data_type(format),
                  pixels));
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        {
          const auto render_texture{resource->as_textureCM};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto wrap_p{convert_texture_wrap(wrap.p)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          state->use_texture(render_texture);

          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filter.min));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filter.mag));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_s));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_t));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_p));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0));
          GL(pglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, levels - 1));
          if (requires_border_color(wrap_s, wrap_t, wrap_p)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            GL(pglTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, color.data()));
          }

          GL(pglTexStorage2D(
            GL_TEXTURE_CUBE_MAP,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h)));

          if (data.size()) {
            for (GLint i = 0; i < levels; i++) {
              const auto& level_info = render_texture->info_for_level(i);
              for (GLint j = 0; j < 6; j++) {
                const auto pixels = data.data() + level_info.offset + level_info.size / 6 * j;
                const auto alignment = texture_alignment(pixels, level_info.dimensions.w, render_texture);

                state->use_pixel_store({alignment, 0, 0});

                if (render_texture->is_compressed_format()) {
                  GL(pglCompressedTexSubImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + j,
                    i,
                    0,
                    0,
                    static_cast<GLsizei>(level_info.dimensions.w),
                    static_cast<GLsizei>(level_info.dimensions.h),
                    convert_texture_data_format(format),
                    level_info.size / 6,
                    pixels));
                } else {
                  GL(pglTexSubImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + j,
                    i,
                    0,
                    0,
                    static_cast<GLsizei>(level_info.dimensions.w),
                    static_cast<GLsizei>(level_info.dimensions.h),
                    convert_texture_format(format),
                    convert_texture_data_type(format),
                    pixels));
                }
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::DOWNLOADER:
        // TODO(dweiler): Implement.
        break;
      }
    }
    break;
  case Frontend::CommandType::RESOURCE_UPDATE:
    {
      const auto resource{reinterpret_cast<const Frontend::UpdateCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::UpdateCommand::Type::BUFFER:
        {
          const auto render_buffer = resource->as_buffer;
          const auto& format = render_buffer->format();
          const auto& vertices = render_buffer->vertices();
          const auto type = render_buffer->type() == Frontend::Buffer::Type::DYNAMIC
              ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

          bool use_vertices_edits = false;
          bool use_elements_edits = false;
          bool use_instances_edits = false;

          auto buffer = reinterpret_cast<detail_es3::buffer*>(render_buffer + 1);

          state->use_buffer(render_buffer);

          // Check for element updates.
          if (format.is_indexed()) {
            const auto& elements = render_buffer->elements();
            if (elements.size() > buffer->elements_size) {
              state->use_ebo(buffer->bo[0]);
              GL(pglBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size(), elements.data(), type));
              buffer->elements_size = elements.size();
            } else {
              use_elements_edits = true;
            }
          }

          if (vertices.size() > buffer->vertices_size) {
            state->use_vbo(buffer->bo[1]);
            GL(pglBufferData(GL_ARRAY_BUFFER, vertices.size(), vertices.data(), type));
            buffer->vertices_size = vertices.size();
          } else {
            use_vertices_edits = true;
          }

          // Check for instance updates.
          if (format.is_instanced()) {
            const auto& instances = render_buffer->instances();
            if (instances.size() > buffer->instances_size) {
              state->use_vbo(buffer->bo[2]);
              GL(pglBufferData(GL_ARRAY_BUFFER, instances.size(), instances.data(), type));
              buffer->instances_size = instances.size();
            } else {
              use_instances_edits = true;
            }
          }

          // Enumerate and apply all buffer edits.
          if (use_vertices_edits || use_elements_edits || use_instances_edits) {
            const auto edits = resource->edit<Frontend::Buffer>();
            for (Size i = 0; i < resource->edits; i++) {
              switch (const auto& edit = edits[i]; edit.sink) {
              case Frontend::Buffer::Sink::ELEMENTS:
                if (use_elements_edits) {
                  const auto& elements = render_buffer->elements();
                  state->use_ebo(buffer->bo[0]);
                  pglBufferSubData(
                    GL_ELEMENT_ARRAY_BUFFER,
                    edit.offset,
                    edit.size,
                    elements.data() + edit.offset);
                }
                break;
              case Frontend::Buffer::Sink::VERTICES:
                if (use_vertices_edits) {
                  state->use_vbo(buffer->bo[1]);
                  pglBufferSubData(
                    GL_ARRAY_BUFFER,
                    edit.offset,
                    edit.size,
                    vertices.data() + edit.offset);
                }
                break;
              case Frontend::Buffer::Sink::INSTANCES:
                if (use_instances_edits) {
                  const auto& instances = render_buffer->instances();
                  state->use_vbo(buffer->bo[2]);
                  pglBufferSubData(
                    GL_ARRAY_BUFFER,
                    edit.offset,
                    edit.size,
                    instances.data() + edit.offset);
                }
                break;
              }
            }
          }
        }
        break;
      case Frontend::UpdateCommand::Type::TEXTURE1D:
        {
          const auto render_texture = resource->as_texture1D;
          const auto edits = resource->edit<Frontend::Texture1D>();
          const auto& format = convert_texture_format(render_texture->format());
          const auto& data_type = convert_texture_data_type(render_texture->format());

          state->use_texture(render_texture);

          for (Size i = 0; i < resource->edits; i++) {
            const auto& edit = edits[i];
            const auto& level_info = render_texture->info_for_level(edit.level);

            auto offset = 0_z;
            offset *= level_info.dimensions;
            offset += edit.offset;
            offset *= render_texture->bits_per_pixel();
            offset /= 8;
            offset += level_info.offset;

            const auto pixels = render_texture->data().data() + offset;
            const auto alignment = texture_alignment(pixels, level_info.dimensions, render_texture);

            state->use_pixel_store({alignment, 0, 0});

            GL(pglTexSubImage2D(
              GL_TEXTURE_2D,
              edit.level,
              edit.offset,
              0,
              edit.size,
              1,
              format,
              data_type,
              pixels));
          }
        }
        break;
      case Frontend::UpdateCommand::Type::TEXTURE2D:
        {
          const auto render_texture = resource->as_texture2D;
          const auto edits = resource->edit<Frontend::Texture2D>();
          const auto& format = convert_texture_format(render_texture->format());
          const auto& data_type = convert_texture_data_type(render_texture->format());

          state->use_texture(render_texture);

          for (Size i = 0; i < resource->edits; i++) {
            const auto& edit = edits[i];
            const auto& level_info = render_texture->info_for_level(edit.level);

            auto offset = 0_z;
            offset *= level_info.dimensions.h;
            offset += edit.offset.y;
            offset *= level_info.dimensions.w;
            offset += edit.offset.x;
            offset *= render_texture->bits_per_pixel();
            offset /= 8;
            offset += level_info.offset;

            const auto pixels = render_texture->data().data() + offset;
            const auto alignment = texture_alignment(pixels, level_info.dimensions.w, render_texture);

            state->use_pixel_store({alignment, level_info.dimensions.w, 0});

            GL(pglTexSubImage2D(
              GL_TEXTURE_2D,
              edit.level,
              edit.offset.x,
              edit.offset.y,
              edit.size.w,
              edit.size.h,
              format,
              data_type,
              pixels));
          }
        }
        break;
      case Frontend::UpdateCommand::Type::TEXTURE3D:
        {
          const auto render_texture = resource->as_texture3D;
          const auto edits = resource->edit<Frontend::Texture3D>();
          const auto& format = convert_texture_format(render_texture->format());
          const auto& data_type = convert_texture_data_type(render_texture->format());

          state->use_texture(render_texture);

          for (Size i = 0; i < resource->edits; i++) {
            const auto& edit = edits[i];
            const auto& level_info = render_texture->info_for_level(edit.level);

            auto offset = 0_z;
            offset *= level_info.dimensions.d;
            offset += edit.offset.z;
            offset *= level_info.dimensions.h;
            offset += edit.offset.y;
            offset *= level_info.dimensions.w;
            offset += edit.offset.x;
            offset *= render_texture->bits_per_pixel();
            offset /= 8;
            offset += level_info.offset;

            const auto pixels = render_texture->data().data() + offset;
            const auto alignment = texture_alignment(pixels, level_info.dimensions.w, render_texture);

            state->use_pixel_store({alignment, level_info.dimensions.w, level_info.dimensions.h});

            GL(pglTexSubImage3D(
              GL_TEXTURE_3D,
              edit.level,
              edit.offset.x,
              edit.offset.y,
              edit.offset.z,
              edit.size.w,
              edit.size.h,
              edit.size.d,
              format,
              data_type,
              pixels));
          }
        }
        break;
      default:
        break;
      }
    }
    break;
  case Frontend::CommandType::CLEAR:
    {
      RX_PROFILE_CPU("clear");

      const auto command{reinterpret_cast<Frontend::ClearCommand*>(header + 1)};
      const auto render_state{&command->render_state};
      const auto render_target{command->render_target};
      const bool clear_depth{command->clear_depth};
      const bool clear_stencil{command->clear_stencil};

      state->use_state(render_state);
      state->use_draw_target(render_target, &command->draw_buffers);

      if (command->clear_colors) {
        for (Uint32 i{0}; i < sizeof command->color_values / sizeof *command->color_values; i++) {
          if (command->clear_colors & (1 << i)) {
            GL(pglClearBufferfv(GL_COLOR, static_cast<GLint>(i),
              command->color_values[i].data()));
          }
        }
      }

      if (clear_depth && clear_stencil) {
        GL(pglClearBufferfi(GL_DEPTH_STENCIL, 0, command->depth_value,
          command->stencil_value));
      } else if (clear_depth) {
        GL(pglClearBufferfv(GL_DEPTH, 0, &command->depth_value));
      } else if (clear_stencil) {
        const GLint value{command->stencil_value};
        GL(pglClearBufferiv(GL_STENCIL, 0, &value));
      }
    }
    break;
  case Frontend::CommandType::DRAW:
    {
      RX_PROFILE_CPU("draw");

      const auto command{reinterpret_cast<Frontend::DrawCommand*>(header + 1)};
      const auto render_state{&command->render_state};
      const auto render_target{command->render_target};
      const auto render_buffer{command->render_buffer};
      const auto render_program{command->render_program};
      const auto this_program{reinterpret_cast<detail_es3::program*>(render_program + 1)};

      state->use_draw_target(render_target, &command->draw_buffers);
      state->use_buffer(render_buffer);
      state->use_program(render_program);
      state->use_state(render_state);

      // check for and apply uniform deltas
      if (command->dirty_uniforms_bitset) {
        const auto& program_uniforms{render_program->uniforms()};
        const Byte* draw_uniforms{command->uniforms()};

        for (Size i{0}; i < 64; i++) {
          if (command->dirty_uniforms_bitset & (1_u64 << i)) {
            const auto& uniform{program_uniforms[i]};
            const auto location{this_program->uniforms[i]};

            if (location == -1) {
              draw_uniforms += uniform.size();
              continue;
            }

            switch (uniform.type()) {
            case Frontend::Uniform::Type::SAMPLER1D:
              [[fallthrough]];
            case Frontend::Uniform::Type::SAMPLER2D:
              [[fallthrough]];
            case Frontend::Uniform::Type::SAMPLER3D:
              [[fallthrough]];
            case Frontend::Uniform::Type::SAMPLERCM:
              pglUniform1i(location,
                *reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32:
              pglUniform1i(location,
                *reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32:
              pglUniform1fv(location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x2:
              pglUniform2iv(location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x3:
              pglUniform3iv(location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x4:
              pglUniform4iv(location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x2:
              pglUniform2fv(location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3:
              pglUniform3fv(location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x4:
              pglUniform4fv(location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3x3:
              pglUniformMatrix3fv(location, 1, GL_FALSE,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3x4:
              pglUniformMatrix3x4fv(location, 1, GL_FALSE,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x4x4:
              pglUniformMatrix4fv(location, 1, GL_FALSE,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::LB_BONES:
              pglUniformMatrix3x4fv(location,
                static_cast<GLsizei>(uniform.size() / sizeof(Math::Mat3x4f)),
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::DQ_BONES:
              pglUniformMatrix2x4fv(location,
                static_cast<GLsizei>(uniform.size() / sizeof(Math::DualQuatf)),
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            }

            draw_uniforms += uniform.size();
          }
        }
      }

      // apply any textures
      for (Size i{0}; i < command->draw_textures.size(); i++) {
        Frontend::Texture* texture{command->draw_textures[i]};
        switch (texture->resource_type()) {
          case Frontend::Resource::Type::TEXTURE1D:
            state->use_active_texture(static_cast<Frontend::Texture1D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURE2D:
            state->use_active_texture(static_cast<Frontend::Texture2D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURE3D:
            state->use_active_texture(static_cast<Frontend::Texture3D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURECM:
            state->use_active_texture(static_cast<Frontend::TextureCM*>(texture), i);
            break;
          default:
            RX_HINT_UNREACHABLE();
        }
      }

      const auto offset = static_cast<GLint>(command->offset);
      const auto count = static_cast<GLsizei>(command->count);
      const auto primitive_type = convert_primitive_type(command->type);

      if (render_buffer) {
        const auto& format = render_buffer->format();
        const auto buffer = reinterpret_cast<detail_es3::buffer*>(render_buffer + 1);
        const auto element_type = convert_element_type(format.element_type());
        const auto indices = reinterpret_cast<const GLvoid*>(format.element_size() * command->offset);

        Size current_attribute = 0;

        state->use_vbo(buffer->bo[1]);
        current_attribute = setup_attributes(
          format.vertex_attributes(),
          format.vertex_stride(),
          current_attribute,
          false,
          format.vertex_stride() * command->base_vertex);

        if (format.is_instanced()) {
          state->use_vbo(buffer->bo[2]);
          current_attribute = setup_attributes(
            format.instance_attributes(),
            format.instance_stride(),
            current_attribute,
            true,
            format.instance_stride() * command->base_instance);
        }

        if (command->instances) {
          if (format.is_indexed()) {
            GL(pglDrawElementsInstanced(
              primitive_type,
              count,
              element_type,
              indices,
              static_cast<GLsizei>(command->instances)));
          } else {
            GL(pglDrawArraysInstanced(
              primitive_type,
              offset,
              count,
              static_cast<GLsizei>(command->instances)));
          }
        } else {
          if (format.is_indexed()) {
            GL(pglDrawElements(primitive_type, count, element_type, indices));
          } else {
            GL(pglDrawArrays(primitive_type, offset, count));
          }
        }
      } else {
        // Bufferless draw calls
        GL(pglDrawArrays(primitive_type, 0, count));
      }
    }
    break;
  case Frontend::CommandType::BLIT:
    {
      RX_PROFILE_CPU("blit");

      const auto command = reinterpret_cast<Frontend::BlitCommand*>(header + 1);
      const auto render_state = &command->render_state;

      // TODO(dweiler): optimize use_state to only consider the things that matter
      // during a blit operation:
      //
      // * scissor test
      // * blend write mask
      state->use_state(render_state);

      auto* src_render_target{command->src_target};
      auto* dst_render_target{command->dst_target};

      const auto src_attachment{command->src_attachment};
      const auto dst_attachment{command->dst_attachment};

      const auto src_dimensions{src_render_target->attachments()[src_attachment].as_texture2D.texture->dimensions().cast<GLint>()};
      const auto dst_dimensions{dst_render_target->attachments()[dst_attachment].as_texture2D.texture->dimensions().cast<GLint>()};

      Frontend::Buffers draw_buffers;
      Frontend::Buffers read_buffers;
      draw_buffers.add(dst_attachment);
      read_buffers.add(src_attachment);

      state->use_read_target(src_render_target, &read_buffers);
      state->use_draw_target(dst_render_target, &draw_buffers);

      GL(pglBlitFramebuffer(
        0,
        0,
        src_dimensions.w,
        src_dimensions.h,
        0,
        0,
        dst_dimensions.w,
        dst_dimensions.h,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST));

      break;
    }
  case Frontend::CommandType::DOWNLOAD:
    // TODO(dweiler): Implement.
    break;
  case Frontend::CommandType::PROFILE:
    // TODO(dweiler): Implement.
    break;
  }
}

void ES3::swap() {
  RX_PROFILE_CPU("swap");
  SDL_GL_SwapWindow(reinterpret_cast<SDL_Window*>(m_data));
}

} // namespace Rx::Render::Backend
