#include "rx/render/backend/gl.h"
#include "rx/render/backend/gl4.h"

#include "rx/render/frontend/target.h"

#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"

#include "rx/core/profiler.h"
#include "rx/core/log.h"

#include "rx/console/variable.h"

#include "rx/core/memory/zero.h"

namespace Rx::Render::Backend {

RX_LOG("render/gl4", logger);

RX_CONSOLE_IVAR(
  anisotropy,
  "gl4.anisotropy",
  "anisotropy value (if supported)",
  0,
  16,
  0);

// 16MiB buffer slab size for unspecified buffer sizes
static constexpr const Size BUFFER_SLAB_SIZE = 16 << 20;

// buffers
static void (GLAPIENTRYP pglCreateBuffers)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteBuffers)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglNamedBufferData)(GLuint, GLsizeiptr, const void*, GLenum);
static void (GLAPIENTRYP pglNamedBufferSubData)(GLuint, GLintptr, GLsizeiptr, const void*);

// vertex arrays
static void (GLAPIENTRYP pglCreateVertexArrays)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteVertexArrays)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglVertexArrayVertexBuffer)(GLuint, GLuint, GLuint, GLintptr, GLsizei);
static void (GLAPIENTRYP pglVertexArrayElementBuffer)(GLuint, GLuint);
static void (GLAPIENTRYP pglEnableVertexArrayAttrib)(GLuint, GLuint);
static void (GLAPIENTRYP pglVertexArrayAttribFormat)(GLuint, GLuint, GLint, GLenum, GLboolean, GLuint);
static void (GLAPIENTRYP pglVertexArrayAttribIFormat)(GLuint, GLuint, GLint, GLenum, GLuint);
static void (GLAPIENTRYP pglVertexArrayAttribBinding)(GLuint, GLuint, GLuint);
static void (GLAPIENTRYP pglVertexArrayBindingDivisor)(GLuint, GLuint, GLuint);
static void (GLAPIENTRYP pglBindVertexArray)(GLuint);

// textures
static void (GLAPIENTRYP pglCreateTextures)(GLenum, GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteTextures)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglTextureStorage1D)(GLuint, GLsizei, GLenum, GLsizei);
static void (GLAPIENTRYP pglTextureStorage2D)(GLuint, GLsizei, GLenum, GLsizei, GLsizei);
static void (GLAPIENTRYP pglTextureStorage3D)(GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
static void (GLAPIENTRYP pglTextureSubImage1D)(GLuint, GLint, GLint, GLsizei, GLenum, GLenum, const void*);
static void (GLAPIENTRYP pglTextureSubImage2D)(GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
static void (GLAPIENTRYP pglTextureSubImage3D)(GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*);
static void (GLAPIENTRYP pglCompressedTextureSubImage1D)(GLuint, GLint, GLint, GLsizei, GLenum, GLsizei, const void*);
static void (GLAPIENTRYP pglCompressedTextureSubImage2D)(GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*);
static void (GLAPIENTRYP pglCompressedTextureSubImage3D)(GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void*);
static void (GLAPIENTRYP pglTextureParameteri)(GLuint, GLenum, GLint);
static void (GLAPIENTRYP pglTextureParameteriv)(GLuint, GLenum, const GLint*);
static void (GLAPIENTRYP pglTextureParameterf)(GLuint, GLenum, GLfloat);
static void (GLAPIENTRYP pglGenerateTextureMipmap)(GLuint);
static void (GLAPIENTRYP pglBindTextureUnit)(GLuint, GLuint);
static void (GLAPIENTRYP pglPixelStorei)(GLenum, GLint);

// frame buffers
static void (GLAPIENTRYP pglCreateFramebuffers)(GLsizei, GLuint*);
static void (GLAPIENTRYP pglDeleteFramebuffers)(GLsizei, const GLuint*);
static void (GLAPIENTRYP pglNamedFramebufferTexture)(GLuint, GLenum, GLuint, GLint);
static void (GLAPIENTRYP pglNamedFramebufferTextureLayer)(GLuint, GLenum, GLuint, GLint, GLint);
static void (GLAPIENTRYP pglBindFramebuffer)(GLenum, GLuint);
static void (GLAPIENTRYP pglClearNamedFramebufferfv)(GLuint, GLenum, GLint, const GLfloat*);
static void (GLAPIENTRYP pglClearNamedFramebufferiv)(GLuint, GLenum, GLint, const GLint*);
static void (GLAPIENTRYP pglClearNamedFramebufferfi)(GLuint, GLenum, GLint, GLfloat, GLint);
static void (GLAPIENTRYP pglNamedFramebufferDrawBuffers)(GLuint, GLsizei, const GLenum*);
static void (GLAPIENTRYP pglBlitNamedFramebuffer)(GLuint, GLuint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
static void (GLAPIENTRYP pglNamedFramebufferDrawBuffer)(GLuint, GLenum);
static void (GLAPIENTRYP pglNamedFramebufferReadBuffer)(GLuint, GLenum);

// shaders and programs
static void (GLAPIENTRYP pglShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
static GLuint (GLAPIENTRYP pglCreateShader)(GLenum);
static void (GLAPIENTRYP pglDeleteShader)(GLuint);
static void (GLAPIENTRYP pglCompileShader)(GLuint);
static void (GLAPIENTRYP pglGetShaderiv)(GLuint, GLenum, GLint*);
static void (GLAPIENTRYP pglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (GLAPIENTRYP pglGetProgramiv)(GLuint, GLenum, GLint*);
static void (GLAPIENTRYP pglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void (GLAPIENTRYP pglAttachShader)(GLuint, GLuint);
static void (GLAPIENTRYP pglLinkProgram)(GLuint);
static void (GLAPIENTRYP pglDetachShader)(GLuint, GLuint);
static GLuint (GLAPIENTRYP pglCreateProgram)();
static void (GLAPIENTRYP pglDeleteProgram)(GLuint);
static void (GLAPIENTRYP pglUseProgram)(GLuint);
static GLint (GLAPIENTRYP pglGetUniformLocation)(GLuint, const GLchar*);
static void (GLAPIENTRYP pglProgramUniform1i)(GLuint, GLint, GLint);
static void (GLAPIENTRYP pglProgramUniform1iv)(GLuint, GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglProgramUniform2iv)(GLuint, GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglProgramUniform3iv)(GLuint, GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglProgramUniform4iv)(GLuint, GLint, GLsizei, const GLint*);
static void (GLAPIENTRYP pglProgramUniform1fv)(GLuint, GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniform2fv)(GLuint, GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniform3fv)(GLuint, GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniform4fv)(GLuint, GLint, GLsizei, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniformMatrix3fv)(GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniformMatrix4fv)(GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniformMatrix3x4fv)(GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
static void (GLAPIENTRYP pglProgramUniformMatrix2x4fv)(GLuint, GLint, GLsizei, GLboolean, const GLfloat*);

// state
static void (GLAPIENTRYP pglEnable)(GLenum);
static void (GLAPIENTRYP pglDisable)(GLenum);
static void (GLAPIENTRYP pglScissor)(GLint, GLint, GLsizei, GLsizei);
static void (GLAPIENTRYP pglColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
static void (GLAPIENTRYP pglBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);
static void (GLAPIENTRYP pglDepthFunc)(GLenum);
static void (GLAPIENTRYP pglDepthMask)(GLboolean);
static void (GLAPIENTRYP pglFrontFace)(GLenum);
static void (GLAPIENTRYP pglCullFace)(GLenum);
static void (GLAPIENTRYP pglStencilMask)(GLuint);
static void (GLAPIENTRYP pglStencilFunc)(GLenum, GLint, GLuint);
static void (GLAPIENTRYP pglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
static void (GLAPIENTRYP pglViewport)(GLint, GLint, GLsizei, GLsizei);

// query
static void (GLAPIENTRYP pglGetIntegerv)(GLenum, GLint*);
static void (GLAPIENTRYP pglGetFloatv)(GLenum, GLfloat*);
static const GLubyte* (GLAPIENTRYP pglGetString)(GLenum);
static const GLubyte* (GLAPIENTRYP pglGetStringi)(GLenum, GLuint);

// draw calls
static void (GLAPIENTRYP pglDrawArrays)(GLenum, GLint, GLsizei);
static void (GLAPIENTRYP pglDrawArraysInstanced)(GLenum, GLint, GLsizei, GLsizei);
static void (GLAPIENTRYP pglDrawArraysInstancedBaseInstance)(GLenum, GLint, GLsizei, GLsizei, GLuint);
static void (GLAPIENTRYP pglDrawElements)(GLenum, GLsizei, GLenum, const GLvoid*);
static void (GLAPIENTRYP pglDrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid*, GLint);
static void (GLAPIENTRYP pglDrawElementsInstanced)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei);
static void (GLAPIENTRYP pglDrawElementsInstancedBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei, GLint);
static void (GLAPIENTRYP pglDrawElementsInstancedBaseInstance)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei, GLuint);
static void (GLAPIENTRYP pglDrawElementsInstancedBaseVertexBaseInstance)(GLenum, GLsizei, GLenum, const GLvoid*, GLsizei, GLint, GLuint);

// flush
static void (GLAPIENTRYP pglFinish)(void);

// ARB_texture_filter_anisotropic
//
// Supported by our version of OpenGL here, but we have to define the enum
// values if not defined because SDL_opengl.h
#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY              0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY          0x84FF
#endif

template<typename F>
static void fetch(const char* _name, F& function_) {
  auto address = SDL_GL_GetProcAddress(_name);
  logger->verbose("loaded %p '%s'", address, _name);
  *reinterpret_cast<void**>(&function_) = address;
}

namespace detail_gl4 {
  struct buffer {
    buffer() {
      pglCreateBuffers(3, bo);
      pglCreateVertexArrays(1, &va);
    }

    ~buffer() {
      pglDeleteBuffers(3, bo);
      pglDeleteVertexArrays(1, &va);
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
      pglCreateFramebuffers(1, &fbo);
    }

    target(GLuint _fbo)
      : fbo{_fbo}
      , owned{false}
    {
    }

    ~target() {
      if (owned) {
        pglDeleteFramebuffers(1, &fbo);
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
      pglDeleteProgram(handle);
    }

    GLuint handle;
    Vector<GLint> uniforms;
  };

  struct texture1D {
    texture1D() {
      pglCreateTextures(GL_TEXTURE_1D, 1, &tex);
    }

    ~texture1D() {
      pglDeleteTextures(1, &tex);
    }

    GLuint tex;
  };

  struct texture2D {
    texture2D() {
      pglCreateTextures(GL_TEXTURE_2D, 1, &tex);
    }

    ~texture2D() {
      pglDeleteTextures(1, &tex);
    }

    GLuint tex;
  };

  struct texture3D {
    texture3D() {
      pglCreateTextures(GL_TEXTURE_3D, 1, &tex);
    }

    ~texture3D() {
      pglDeleteTextures(1, &tex);
    }

    GLuint tex;
  };

  struct textureCM {
    textureCM() {
      pglCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);
    }

    ~textureCM() {
      pglDeleteTextures(1, &tex);
    }

    GLuint tex;
  };

  struct state
    : Frontend::State
  {
    state(SDL_GLContext _context)
      : m_color_mask{0xff}
      , m_empty_vao{0}
      , m_bound_vao{0}
      , m_bound_fbo{0}
      , m_bound_program{0}
      , m_swap_chain_fbo{0}
      , m_context{_context}
    {
      Memory::zero(m_texture_units);

      // There's no unsigned variant of glGetIntegerv
      GLint swap_chain_fbo;
      pglGetIntegerv(GL_FRAMEBUFFER_BINDING, &swap_chain_fbo);
      m_swap_chain_fbo = static_cast<GLuint>(swap_chain_fbo);

      pglEnable(GL_CULL_FACE);
      pglEnable(GL_PROGRAM_POINT_SIZE);
      pglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      pglEnable(GL_FRAMEBUFFER_SRGB);
      pglCullFace(GL_BACK);
      pglFrontFace(GL_CW);

      pglDepthFunc(GL_LEQUAL);
      pglDisable(GL_MULTISAMPLE);
      pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      pglCreateVertexArrays(1, &m_empty_vao);

      const auto vendor{reinterpret_cast<const char*>(pglGetString(GL_VENDOR))};
      const auto renderer{reinterpret_cast<const char*>(pglGetString(GL_RENDERER))};
      const auto version{reinterpret_cast<const char*>(pglGetString(GL_VERSION))};

      logger->info("GL %s %s %s", vendor, version, renderer);

      bool texture_filter_anisotropic{false};
      GLint extensions{0};
      pglGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
      for (GLint i{0}; i < extensions; i++) {
        const auto name{reinterpret_cast<const char*>(pglGetStringi(GL_EXTENSIONS, i))};
        logger->verbose("extension '%s' supported", name);

        if (!strcmp(name, "GL_ARB_texture_filter_anisotropic")) {
          Float32 max_aniso{0.0f};
          pglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
          anisotropy->set(static_cast<Sint32>(max_aniso));
          texture_filter_anisotropic = true;
        }
      }

      if (!texture_filter_anisotropic) {
        anisotropy->set(0);
      }
    }

    ~state() {
      pglDeleteVertexArrays(1, &m_empty_vao);

      SDL_GL_DeleteContext(m_context);
    }

    void use_enable(GLenum _thing, bool _enable) {
      if (_enable) {
        pglEnable(_thing);
      } else {
        pglDisable(_thing);
      }
    }

    void use_state(const Frontend::State* _render_state) {
      RX_PROFILE_CPU("use_state");

      const auto& scissor{_render_state->scissor};
      const auto& blend{_render_state->blend};
      const auto& cull{_render_state->cull};
      const auto& stencil{_render_state->stencil};
      const auto& depth{_render_state->depth};
      const auto& viewport(_render_state->viewport);

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
            pglScissor(offset.x, offset.y, size.w, size.h);

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
            pglColorMask(r, g, b, a);
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
            pglBlendFuncSeparate(
              convert_blend_factor(color_src_factor),
              convert_blend_factor(color_dst_factor),
              convert_blend_factor(alpha_src_factor),
              convert_blend_factor(alpha_dst_factor));

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
            pglDepthMask(write ? GL_TRUE : GL_FALSE);
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
              pglFrontFace(GL_CW);
              break;
            case Frontend::CullState::FrontFaceType::COUNTER_CLOCK_WISE:
              pglFrontFace(GL_CCW);
              break;
            }
            this->cull.record_front_face(front_face);
          }

          if (this->cull.cull_face() != cull_face) {
            switch (cull_face) {
            case Frontend::CullState::CullFaceType::FRONT:
              pglCullFace(GL_FRONT);
              break;
            case Frontend::CullState::CullFaceType::BACK:
              pglCullFace(GL_BACK);
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
            pglStencilMask(write_mask);
            this->stencil.record_write_mask(write_mask);
          }

          if (this->stencil.function() != function ||
              this->stencil.reference() != reference ||
              this->stencil.mask() != mask)
          {
            pglStencilFunc(
              convert_stencil_function(function),
              static_cast<GLint>(reference),
              static_cast<GLuint>(mask));

            this->stencil.record_function(function);
            this->stencil.record_reference(reference);
            this->stencil.record_mask(mask);
          }

          if (this->stencil.front_fail_action() != front_fail_action ||
              this->stencil.front_depth_fail_action() != front_depth_fail_action ||
              this->stencil.front_depth_pass_action() != front_depth_pass_action)
          {
            pglStencilOpSeparate(
              GL_FRONT,
              convert_stencil_operation(front_fail_action),
              convert_stencil_operation(front_depth_fail_action),
              convert_stencil_operation(front_depth_pass_action));

            this->stencil.record_front_fail_action(front_fail_action);
            this->stencil.record_front_depth_fail_action(front_depth_fail_action);
            this->stencil.record_front_depth_pass_action(front_depth_pass_action);
          }

          if (this->stencil.back_fail_action() != back_fail_action ||
              this->stencil.back_depth_fail_action() != back_depth_fail_action ||
              this->stencil.back_depth_pass_action() != back_depth_pass_action)
          {
            pglStencilOpSeparate(
              GL_BACK,
              convert_stencil_operation(back_fail_action),
              convert_stencil_operation(back_depth_fail_action),
              convert_stencil_operation(back_depth_pass_action));

            this->stencil.record_back_fail_action(back_fail_action);
            this->stencil.record_back_depth_fail_action(back_depth_fail_action);
            this->stencil.record_back_depth_pass_action(back_depth_pass_action);
          }
        }
      }

      if (this->viewport != viewport) {
        const auto& offset{viewport.offset().cast<GLuint>()};
        const auto& dimensions{viewport.dimensions().cast<GLsizei>()};
        pglViewport(offset.x, offset.y, dimensions.w, dimensions.h);
        this->viewport.record_offset(viewport.offset());
        this->viewport.record_dimensions(viewport.dimensions());
      }

      // flush all changes to this for updated hash
      flush();
    }

    void use_draw_target(Frontend::Target* _render_target, const Frontend::Buffers* _draw_buffers) {
      RX_PROFILE_CPU("use_draw_target");

      const auto this_target{reinterpret_cast<const target*>(_render_target + 1)};
      if (this_target->fbo != m_bound_fbo) {
        pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, this_target->fbo);
        m_bound_fbo = this_target->fbo;
      }

      // Changing draw buffers?
      if (_draw_buffers && !_render_target->is_swapchain()) {
        auto this_target{reinterpret_cast<target*>(_render_target + 1)};
        // The draw buffers changed.
        if (this_target->draw_buffers != *_draw_buffers) {
          if (_draw_buffers->is_empty()) {
            pglNamedFramebufferDrawBuffer(this_target->fbo, GL_NONE);
          } else {
            Vector<GLenum> draw_buffers;
            for (Size i{0}; i < _draw_buffers->size(); i++) {
              draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + (*_draw_buffers)[i]);
            }
            pglNamedFramebufferDrawBuffers(
              this_target->fbo,
              static_cast<GLsizei>(draw_buffers.size()),
              draw_buffers.data());
          }

          this_target->draw_buffers = *_draw_buffers;
        }
      }
    }

    void use_program(const Frontend::Program* _render_program) {
      RX_PROFILE_CPU("use_program");
      const auto this_program{reinterpret_cast<const program*>(_render_program + 1)};
      if (this_program->handle != m_bound_program) {
        pglUseProgram(this_program->handle);
        m_bound_program = this_program->handle;
      }
    }

    void use_buffer(const Frontend::Buffer* _render_buffer) {
      RX_PROFILE_CPU("use_buffer");
      if (_render_buffer) {
        const auto this_buffer{reinterpret_cast<const buffer*>(_render_buffer + 1)};
        if (this_buffer->va != m_bound_vao) {
          pglBindVertexArray(this_buffer->va);
          m_bound_vao = this_buffer->va;
        }
      } else if (!m_bound_vao) {
        pglBindVertexArray(m_empty_vao);
        m_bound_vao = m_empty_vao;
      }
    }

    struct texture_unit {
      GLuint texture1D;
      GLuint texture2D;
      GLuint texture3D;
      GLuint textureCM;
    };

    template<typename Ft, typename Bt, GLuint texture_unit::*name>
    void use_texture_template(const Ft* _render_texture, GLuint unit) {
      RX_PROFILE_CPU("use_texture");

      const auto this_texture{reinterpret_cast<const Bt*>(_render_texture + 1)};
      auto& texture_unit{m_texture_units[unit]};
      if (texture_unit.*name != this_texture->tex) {
        texture_unit.*name = this_texture->tex;
        pglBindTextureUnit(unit, this_texture->tex);
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

    void use_texture(const Frontend::Texture1D* _render_texture, Size _unit) {
      use_texture_template<Frontend::Texture1D, texture1D, &texture_unit::texture1D>(_render_texture, _unit);
    }

    void use_texture(const Frontend::Texture2D* _render_texture, Size _unit) {
      use_texture_template<Frontend::Texture2D, texture2D, &texture_unit::texture2D>(_render_texture, _unit);
    }

    void use_texture(const Frontend::Texture3D* _render_texture, Size _unit) {
      use_texture_template<Frontend::Texture3D, texture3D, &texture_unit::texture3D>(_render_texture, _unit);
    }

    void use_texture(const Frontend::TextureCM* _render_texture, Size _unit) {
      use_texture_template<Frontend::TextureCM, textureCM, &texture_unit::textureCM>(_render_texture, _unit);
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

    GLuint m_bound_vao;
    GLuint m_bound_fbo;
    GLuint m_bound_program;

    GLuint m_swap_chain_fbo;
    texture_unit m_texture_units[Frontend::Textures::MAX_TEXTURES];

    SDL_GLContext m_context;
  };
}

static GLuint compile_shader(Memory::Allocator& _allocator,
  const Vector<Frontend::Uniform>& _uniforms, const Frontend::Shader& _shader)
{
  auto contents = generate_glsl(_allocator, _uniforms, _shader, 440, false);

  auto data = static_cast<const GLchar*>(contents->data());
  auto size = static_cast<GLint>(contents->size());

  GLuint handle = pglCreateShader(convert_shader_type(_shader.kind));
  pglShaderSource(handle, 1, &data, &size);
  pglCompileShader(handle);

  GLint status{0};
  pglGetShaderiv(handle, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_size{0};
    pglGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_size);

    logger->error("failed compiling shader");

    if (log_size) {
      Vector<char> error_log{_allocator};
      if (!error_log.resize(log_size)) {
        logger->error("out of memory");
      } else {
        pglGetShaderInfoLog(handle, log_size, &log_size, error_log.data());
        logger->error("\n%s\n%s", error_log.data(), contents->data());
      }
    }

    pglDeleteShader(handle);
    return 0;
  }

  return handle;
}

AllocationInfo GL4::query_allocation_info() const {
  AllocationInfo info;
  info.buffer_size = sizeof(detail_gl4::buffer);
  info.target_size = sizeof(detail_gl4::target);
  info.program_size = sizeof(detail_gl4::program);
  info.texture1D_size = sizeof(detail_gl4::texture1D);
  info.texture2D_size = sizeof(detail_gl4::texture2D);
  info.texture3D_size = sizeof(detail_gl4::texture3D);
  info.textureCM_size = sizeof(detail_gl4::textureCM);
  return info;
}

DeviceInfo GL4::query_device_info() const {
  return {
    reinterpret_cast<const char*>(pglGetString(GL_VENDOR)),
    reinterpret_cast<const char*>(pglGetString(GL_RENDERER)),
    reinterpret_cast<const char*>(pglGetString(GL_VERSION))
  };
}

GL4::GL4(Memory::Allocator& _allocator, void* _data)
  : m_allocator{_allocator}
  , m_data{_data}
{
}

GL4::~GL4() {
  m_allocator.destroy<detail_gl4::state>(m_impl);
}

bool GL4::init() {
  SDL_GLContext context{SDL_GL_CreateContext(reinterpret_cast<SDL_Window*>(m_data))};
  if (!context) {
    return false;
  }

  // buffers
  fetch("glCreateBuffers", pglCreateBuffers);
  fetch("glDeleteBuffers", pglDeleteBuffers);
  fetch("glNamedBufferData", pglNamedBufferData);
  fetch("glNamedBufferSubData", pglNamedBufferSubData);

  // vertex arrays
  fetch("glCreateVertexArrays", pglCreateVertexArrays);
  fetch("glDeleteVertexArrays", pglDeleteVertexArrays);
  fetch("glVertexArrayVertexBuffer", pglVertexArrayVertexBuffer);
  fetch("glVertexArrayElementBuffer", pglVertexArrayElementBuffer);
  fetch("glEnableVertexArrayAttrib", pglEnableVertexArrayAttrib);
  fetch("glVertexArrayAttribFormat", pglVertexArrayAttribFormat);
  fetch("glVertexArrayAttribIFormat", pglVertexArrayAttribIFormat);
  fetch("glVertexArrayAttribBinding", pglVertexArrayAttribBinding);
  fetch("glVertexArrayBindingDivisor", pglVertexArrayBindingDivisor);
  fetch("glBindVertexArray", pglBindVertexArray);

  // textures
  fetch("glCreateTextures", pglCreateTextures);
  fetch("glDeleteTextures", pglDeleteTextures);
  fetch("glTextureStorage1D", pglTextureStorage1D);
  fetch("glTextureStorage2D", pglTextureStorage2D);
  fetch("glTextureStorage3D", pglTextureStorage3D);
  fetch("glTextureSubImage1D", pglTextureSubImage1D);
  fetch("glTextureSubImage2D", pglTextureSubImage2D);
  fetch("glTextureSubImage3D", pglTextureSubImage3D);
  fetch("glCompressedTextureSubImage1D", pglCompressedTextureSubImage1D);
  fetch("glCompressedTextureSubImage2D", pglCompressedTextureSubImage2D);
  fetch("glCompressedTextureSubImage3D", pglCompressedTextureSubImage3D);
  fetch("glTextureParameteri", pglTextureParameteri);
  fetch("glTextureParameteriv", pglTextureParameteriv);
  fetch("glTextureParameterf", pglTextureParameterf);
  fetch("glGenerateTextureMipmap", pglGenerateTextureMipmap);
  fetch("glBindTextureUnit", pglBindTextureUnit);
  fetch("glPixelStorei", pglPixelStorei);

  // frame buffers
  fetch("glCreateFramebuffers", pglCreateFramebuffers);
  fetch("glDeleteFramebuffers", pglDeleteFramebuffers);
  fetch("glNamedFramebufferTexture", pglNamedFramebufferTexture);
  fetch("glNamedFramebufferTextureLayer", pglNamedFramebufferTextureLayer);
  fetch("glBindFramebuffer", pglBindFramebuffer);
  fetch("glClearNamedFramebufferfv", pglClearNamedFramebufferfv);
  fetch("glClearNamedFramebufferiv", pglClearNamedFramebufferiv);
  fetch("glClearNamedFramebufferfi", pglClearNamedFramebufferfi);
  fetch("glNamedFramebufferDrawBuffers", pglNamedFramebufferDrawBuffers);
  fetch("glBlitNamedFramebuffer", pglBlitNamedFramebuffer);
  fetch("glNamedFramebufferDrawBuffer", pglNamedFramebufferDrawBuffer);
  fetch("glNamedFramebufferReadBuffer", pglNamedFramebufferReadBuffer);

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
  fetch("glProgramUniform1i", pglProgramUniform1i);
  fetch("glProgramUniform1iv", pglProgramUniform1iv);
  fetch("glProgramUniform2iv", pglProgramUniform2iv);
  fetch("glProgramUniform3iv", pglProgramUniform3iv);
  fetch("glProgramUniform4iv", pglProgramUniform4iv);
  fetch("glProgramUniform1fv", pglProgramUniform1fv);
  fetch("glProgramUniform2fv", pglProgramUniform2fv);
  fetch("glProgramUniform3fv", pglProgramUniform3fv);
  fetch("glProgramUniform4fv", pglProgramUniform4fv);
  fetch("glProgramUniformMatrix3fv", pglProgramUniformMatrix3fv);
  fetch("glProgramUniformMatrix4fv", pglProgramUniformMatrix4fv);
  fetch("glProgramUniformMatrix3x4fv", pglProgramUniformMatrix3x4fv);
  fetch("glProgramUniformMatrix2x4fv", pglProgramUniformMatrix2x4fv);

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
  fetch("glDrawArraysInstancedBaseInstance", pglDrawArraysInstancedBaseInstance);
  fetch("glDrawElements", pglDrawElements);
  fetch("glDrawElementsBaseVertex", pglDrawElementsBaseVertex);
  fetch("glDrawElementsInstanced", pglDrawElementsInstanced);
  fetch("glDrawElementsInstancedBaseVertex", pglDrawElementsInstancedBaseVertex);
  fetch("glDrawElementsInstancedBaseInstance", pglDrawElementsInstancedBaseInstance);
  fetch("glDrawElementsInstancedBaseVertexBaseInstance", pglDrawElementsInstancedBaseVertexBaseInstance);

  // flush
  fetch("glFinish", pglFinish);

  m_impl = m_allocator.create<detail_gl4::state>(context);

  return m_impl != nullptr;
}

void GL4::process(const Vector<Byte*>& _commands) {
  _commands.each_fwd([this](Byte* _command) {
    process(_command);
  });
}

void GL4::process(Byte* _command) {
  RX_PROFILE_CPU("gl4::process");

  auto state{reinterpret_cast<detail_gl4::state*>(m_impl)};
  auto header{reinterpret_cast<Frontend::CommandHeader*>(_command)};
  switch (header->type) {
  case Frontend::CommandType::RESOURCE_ALLOCATE:
    {
      const auto resource{reinterpret_cast<const Frontend::ResourceCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::ResourceCommand::Type::BUFFER:
        Utility::construct<detail_gl4::buffer>(resource->as_buffer + 1);
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        {
          const auto render_target{resource->as_target};
          if (render_target->is_swapchain()) {
            Utility::construct<detail_gl4::target>(resource->as_target + 1, state->m_swap_chain_fbo);
          } else {
            Utility::construct<detail_gl4::target>(resource->as_target + 1);
          }
        }
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        Utility::construct<detail_gl4::program>(resource->as_program + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE1D:
        Utility::construct<detail_gl4::texture1D>(resource->as_texture1D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE2D:
        if (resource->as_texture2D->is_swapchain()) {
          break;
        }
        Utility::construct<detail_gl4::texture2D>(resource->as_texture2D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        Utility::construct<detail_gl4::texture3D>(resource->as_texture3D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        Utility::construct<detail_gl4::textureCM>(resource->as_textureCM + 1);
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
        if (state->m_bound_vao == reinterpret_cast<detail_gl4::buffer*>(resource->as_buffer + 1)->va) {
          state->m_bound_vao = 0;
        }
        Utility::destruct<detail_gl4::buffer>(resource->as_buffer + 1);
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        if (state->m_bound_fbo == reinterpret_cast<detail_gl4::target*>(resource->as_target + 1)->fbo) {
          state->m_bound_fbo = 0;
        }
        Utility::destruct<detail_gl4::target>(resource->as_target + 1);
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        Utility::destruct<detail_gl4::program>(resource->as_program + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE1D:
        state->invalidate_texture(resource->as_texture1D);
        Utility::destruct<detail_gl4::texture1D>(resource->as_texture1D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE2D:
        if (resource->as_texture2D->is_swapchain()) {
          break;
        }
        state->invalidate_texture(resource->as_texture2D);
        Utility::destruct<detail_gl4::texture2D>(resource->as_texture2D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        state->invalidate_texture(resource->as_texture3D);
        Utility::destruct<detail_gl4::texture3D>(resource->as_texture3D + 1);
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        state->invalidate_texture(resource->as_textureCM);
        Utility::destruct<detail_gl4::textureCM>(resource->as_textureCM + 1);
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
          auto buffer = reinterpret_cast<detail_gl4::buffer*>(render_buffer + 1);

          const auto type = format.type() == Frontend::Buffer::Type::DYNAMIC
            ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

          auto setup_attributes = [](GLuint _vao,
                                     const Vector<Frontend::Buffer::Attribute>& attributes,
                                     Size _index_offset,
                                     bool _instanced)
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

              Size offset = attribute.offset;
              for (GLsizei j = 0; j < result.instances; j++) {
                pglEnableVertexArrayAttrib(_vao, index + j);
                pglVertexArrayAttribBinding(_vao, index + j, _instanced ? 1 : 0);
                if (is_int_format(result.type_enum)) {
                  pglVertexArrayAttribIFormat(
                    _vao,
                    index + j,
                    result.components,
                    result.type_enum,
                    static_cast<GLuint>(offset));
                } else {
                  pglVertexArrayAttribFormat(
                    _vao,
                    index + j,
                    result.components,
                    result.type_enum,
                    GL_FALSE,
                    static_cast<GLuint>(offset));
                }
                offset += result.type_size * result.components;
                count++;
              }
            }

            if (_instanced) {
              pglVertexArrayBindingDivisor(_vao, 1, 1);
            }
            return count;
          };

          Size current_attribute = 0;

          // Setup element buffer.
          if (format.is_indexed()) {
            const auto& elements = render_buffer->elements();
            if (elements.is_empty()) {
              pglNamedBufferData(buffer->bo[0], BUFFER_SLAB_SIZE, nullptr, type);
              buffer->elements_size = BUFFER_SLAB_SIZE;
            } else {
              pglNamedBufferData(buffer->bo[0], elements.size(), elements.data(), type);
              buffer->elements_size = elements.size();
            }
            pglVertexArrayElementBuffer(buffer->va, buffer->bo[0]);
          }

          // Setup vertex buffer and attributes.
          const auto& vertices = render_buffer->vertices();
          if (vertices.is_empty()) {
            pglNamedBufferData(buffer->bo[1], BUFFER_SLAB_SIZE, nullptr, type);
            buffer->vertices_size = BUFFER_SLAB_SIZE;
          } else {
            pglNamedBufferData(buffer->bo[1], vertices.size(), vertices.data(), type);
            buffer->vertices_size = vertices.size();
          }
          pglVertexArrayVertexBuffer(
            buffer->va,
            0,
            buffer->bo[1],
            0,
            static_cast<GLsizei>(format.vertex_stride()));
          current_attribute = setup_attributes(
            buffer->va,
            format.vertex_attributes(),
            current_attribute,
            false);

          // Setup instance buffer and attributes.
          if (format.is_instanced()) {
            const auto& instances = render_buffer->instances();
            if (instances.is_empty()) {
              pglNamedBufferData(buffer->bo[2], BUFFER_SLAB_SIZE, nullptr, type);
              buffer->instances_size = BUFFER_SLAB_SIZE;
            } else {
              pglNamedBufferData(buffer->bo[2], instances.size(), instances.data(), type);
              buffer->instances_size = instances.size();
            }
            pglVertexArrayVertexBuffer(
              buffer->va,
              1,
              buffer->bo[2],
              0,
              static_cast<GLsizei>(format.instance_stride()));
            current_attribute = setup_attributes(
              buffer->va,
              format.instance_attributes(),
              current_attribute,
              true);
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TARGET:
        {
          const auto render_target{resource->as_target};
          if (render_target->is_swapchain()) {
            // Swap chain targets don't have an user-defined attachments.
            break;
          }

          const auto target{reinterpret_cast<const detail_gl4::target*>(render_target + 1)};
          if (render_target->has_depth_stencil()) {
            const auto depth_stencil{render_target->depth_stencil()};
            // combined depth stencil format
            const auto texture{reinterpret_cast<const detail_gl4::texture2D*>(depth_stencil + 1)};
            pglNamedFramebufferTexture(target->fbo, GL_DEPTH_STENCIL_ATTACHMENT, texture->tex, 0);
          } else {
            if (render_target->has_depth()) {
              const auto depth{render_target->depth()};
              const auto texture{reinterpret_cast<const detail_gl4::texture2D*>(depth + 1)};
              pglNamedFramebufferTexture(target->fbo, GL_DEPTH_ATTACHMENT, texture->tex, 0);
            } else if (render_target->has_stencil()) {
              const auto stencil{render_target->stencil()};
              const auto texture{reinterpret_cast<const detail_gl4::texture2D*>(stencil + 1)};
              pglNamedFramebufferTexture(target->fbo, GL_STENCIL_ATTACHMENT, texture->tex, 0);
            }
          }

          // color attachments
          const auto& attachments{render_target->attachments()};
          for (Size i{0}; i < attachments.size(); i++) {
            const auto& attachment{attachments[i]};
            const auto attachment_enum{static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i)};
            switch (attachment.kind) {
            case Frontend::Target::Attachment::Type::TEXTURE2D:
              pglNamedFramebufferTexture(
                target->fbo,
                attachment_enum,
                reinterpret_cast<detail_gl4::texture2D*>(attachment.as_texture2D.texture + 1)->tex,
                static_cast<GLint>(attachment.level));
              break;
            case Frontend::Target::Attachment::Type::TEXTURECM:
              pglNamedFramebufferTextureLayer(
                target->fbo,
                attachment_enum,
                reinterpret_cast<detail_gl4::textureCM*>(attachment.as_textureCM.texture + 1)->tex,
                static_cast<GLint>(attachment.level),
                static_cast<GLint>(attachment.as_textureCM.face));
              break;
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::PROGRAM:
        {
          const auto render_program{resource->as_program};
          const auto program{reinterpret_cast<detail_gl4::program*>(render_program + 1)};

          const auto& shaders = render_program->shaders();

          Vector<GLuint> shader_handles{m_allocator};
          shaders.each_fwd([&](const Frontend::Shader& _shader) {
            GLuint shader_handle{compile_shader(m_allocator, render_program->uniforms(), _shader)};
            if (shader_handle != 0) {
              pglAttachShader(program->handle, shader_handle);
              shader_handles.push_back(shader_handle);
            }
          });

          pglLinkProgram(program->handle);

          GLint status{0};
          pglGetProgramiv(program->handle, GL_LINK_STATUS, &status);
          if (status != GL_TRUE) {
            GLint log_size{0};
            pglGetProgramiv(program->handle, GL_INFO_LOG_LENGTH, &log_size);

            logger->error("failed linking program");

            if (log_size) {
              Vector<char> error_log{Memory::SystemAllocator::instance()};
              if (!error_log.resize(log_size)) {
                logger->error("out of memory");
              } else {
                pglGetProgramInfoLog(program->handle, log_size, &log_size, error_log.data());
                logger->error("\n%s", error_log.data());
              }
            }
          }

          shader_handles.each_fwd([&](GLuint _shader) {
            pglDetachShader(program->handle, _shader);
            pglDeleteShader(_shader);
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
          const auto texture{reinterpret_cast<const detail_gl4::texture1D*>(render_texture + 1)};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          pglTextureParameteri(texture->tex, GL_TEXTURE_MIN_FILTER, filter.min);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAG_FILTER, filter.mag);
          if (*anisotropy) {
            pglTextureParameterf(texture->tex, GL_TEXTURE_MAX_ANISOTROPY, static_cast<Float32>(*anisotropy));
          }

          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_S, wrap_s);
          pglTextureParameteri(texture->tex, GL_TEXTURE_BASE_LEVEL, 0);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAX_LEVEL, levels - 1);
          if (requires_border_color(wrap_s)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            pglTextureParameteriv(texture->tex, GL_TEXTURE_BORDER_COLOR, color.data());
          }

          pglTextureStorage1D(
            texture->tex,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions));

          if (data.size()) {
            for (GLint i{0}; i < levels; i++) {
              const auto level_info{render_texture->info_for_level(i)};
              if (render_texture->is_compressed_format()) {
                pglCompressedTextureSubImage1D(
                  texture->tex,
                  i,
                  0,
                  static_cast<GLsizei>(level_info.dimensions),
                  convert_texture_data_format(format),
                  static_cast<GLsizei>(level_info.size),
                  data.data() + level_info.offset);
              } else {
                pglTextureSubImage1D(
                  texture->tex,
                  i,
                  0,
                  static_cast<GLsizei>(level_info.dimensions),
                  convert_texture_format(format),
                  GL_UNSIGNED_BYTE,
                  data.data() + level_info.offset);
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

          const auto texture{reinterpret_cast<const detail_gl4::texture2D*>(render_texture + 1)};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          pglTextureParameteri(texture->tex, GL_TEXTURE_MIN_FILTER, filter.min);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAG_FILTER, filter.mag);
          if (*anisotropy) {
            pglTextureParameterf(texture->tex, GL_TEXTURE_MAX_ANISOTROPY, static_cast<Float32>(*anisotropy));
          }

          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_S, wrap_s);
          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_T, wrap_t);
          pglTextureParameteri(texture->tex, GL_TEXTURE_BASE_LEVEL, 0);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAX_LEVEL, levels - 1);
          if (requires_border_color(wrap_s, wrap_t)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            pglTextureParameteriv(texture->tex, GL_TEXTURE_BORDER_COLOR, color.data());
          }

          pglTextureStorage2D(
            texture->tex,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h));

          if (data.size()) {
            for (GLint i{0}; i < levels; i++) {
              const auto level_info{render_texture->info_for_level(i)};
              if (render_texture->is_compressed_format()) {
                pglCompressedTextureSubImage2D(
                  texture->tex,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  convert_texture_data_format(format),
                  static_cast<GLsizei>(level_info.size),
                  data.data() + level_info.offset);
              } else {
                pglTextureSubImage2D(
                  texture->tex,
                  i,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  convert_texture_format(format),
                  convert_texture_data_type(format),
                  data.data() + level_info.offset);
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURE3D:
        {
          const auto render_texture{resource->as_texture3D};
          const auto texture{reinterpret_cast<const detail_gl4::texture3D*>(render_texture + 1)};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto wrap_r{convert_texture_wrap(wrap.p)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          pglTextureParameteri(texture->tex, GL_TEXTURE_MIN_FILTER, filter.min);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAG_FILTER, filter.mag);
          if (*anisotropy) {
            pglTextureParameterf(texture->tex, GL_TEXTURE_MAX_ANISOTROPY, static_cast<Float32>(*anisotropy));
          }

          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_S, wrap_s);
          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_T, wrap_t);
          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_R, wrap_r);
          pglTextureParameteri(texture->tex, GL_TEXTURE_BASE_LEVEL, 0);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAX_LEVEL, levels - 1);
          if (requires_border_color(wrap_s, wrap_t, wrap_r)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            pglTextureParameteriv(texture->tex, GL_TEXTURE_BORDER_COLOR, color.data());
          }

          pglTextureStorage3D(
            texture->tex,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h),
            static_cast<GLsizei>(dimensions.d));

          if (data.size()) {
            for (GLint i{0}; i < levels; i++) {
              const auto level_info{render_texture->info_for_level(i)};
              if (render_texture->is_compressed_format()) {
                pglCompressedTextureSubImage3D(
                  texture->tex,
                  i,
                  0,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  static_cast<GLsizei>(level_info.dimensions.d),
                  convert_texture_data_format(format),
                  static_cast<GLsizei>(level_info.size),
                  data.data() + level_info.offset);
              } else {
                pglTextureSubImage3D(
                  texture->tex,
                  i,
                  0,
                  0,
                  0,
                  static_cast<GLsizei>(level_info.dimensions.w),
                  static_cast<GLsizei>(level_info.dimensions.h),
                  static_cast<GLsizei>(level_info.dimensions.d),
                  convert_texture_format(format),
                  convert_texture_data_type(format),
                  data.data() + level_info.offset);
              }
            }
          }
        }
        break;
      case Frontend::ResourceCommand::Type::TEXTURECM:
        {
          const auto render_texture{resource->as_textureCM};
          const auto texture{reinterpret_cast<const detail_gl4::textureCM*>(render_texture + 1)};
          const auto wrap{render_texture->wrap()};
          const auto wrap_s{convert_texture_wrap(wrap.s)};
          const auto wrap_t{convert_texture_wrap(wrap.t)};
          const auto wrap_p{convert_texture_wrap(wrap.p)};
          const auto dimensions{render_texture->dimensions()};
          const auto format{render_texture->format()};
          const auto filter{convert_texture_filter(render_texture->filter())};
          const auto& data{render_texture->data()};

          const auto levels{static_cast<GLint>(render_texture->levels())};

          pglTextureParameteri(texture->tex, GL_TEXTURE_MIN_FILTER, filter.min);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAG_FILTER, filter.mag);
          if (*anisotropy) {
            pglTextureParameterf(texture->tex, GL_TEXTURE_MAX_ANISOTROPY, static_cast<Float32>(*anisotropy));
          }

          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_S, wrap_s);
          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_T, wrap_t);
          pglTextureParameteri(texture->tex, GL_TEXTURE_WRAP_R, wrap_p);
          pglTextureParameteri(texture->tex, GL_TEXTURE_BASE_LEVEL, 0);
          pglTextureParameteri(texture->tex, GL_TEXTURE_MAX_LEVEL, levels - 1);
          if (requires_border_color(wrap_s, wrap_t, wrap_p)) {
            const Math::Vec4i color{(render_texture->border() * 255.0f).cast<Sint32>()};
            pglTextureParameteriv(texture->tex, GL_TEXTURE_BORDER_COLOR, color.data());
          }

          pglTextureStorage2D(
            texture->tex,
            levels,
            convert_texture_data_format(format),
            static_cast<GLsizei>(dimensions.w),
            static_cast<GLsizei>(dimensions.h));

          if (data.size()) {
            for (GLint i{0}; i < levels; i++) {
              const auto level_info{render_texture->info_for_level(i)};
              for (GLint j{0}; j < 6; j++) {
                if (render_texture->is_compressed_format()) {
                  pglCompressedTextureSubImage3D(
                    texture->tex,
                    i,
                    0,
                    0,
                    j,
                    static_cast<GLsizei>(level_info.dimensions.w),
                    static_cast<GLsizei>(level_info.dimensions.h),
                    1,
                    convert_texture_format(format),
                    static_cast<GLsizei>(level_info.size),
                    data.data() + level_info.offset + level_info.size / 6 * j);
                } else {
                  pglTextureSubImage3D(
                    texture->tex,
                    i,
                    0,
                    0,
                    j,
                    static_cast<GLsizei>(level_info.dimensions.w),
                    static_cast<GLsizei>(level_info.dimensions.h),
                    1,
                    convert_texture_format(format),
                    convert_texture_data_type(format),
                    data.data() + level_info.offset + level_info.size / 6 * j);
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
      RX_PROFILE_CPU("update");

      const auto resource{reinterpret_cast<const Frontend::UpdateCommand*>(header + 1)};
      switch (resource->type) {
      case Frontend::UpdateCommand::Type::BUFFER:
        {
          const auto render_buffer = resource->as_buffer;
          const auto& format = render_buffer->format();
          const auto& vertices = render_buffer->vertices();
          const auto type = format.type() == Frontend::Buffer::Type::DYNAMIC
              ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

          bool use_vertices_edits = false;
          bool use_elements_edits = false;
          bool use_instances_edits = false;

          auto buffer = reinterpret_cast<detail_gl4::buffer*>(render_buffer + 1);

          // Check for element updates.
          if (format.is_indexed()) {
            const auto& elements = render_buffer->elements();
            if (elements.size() > buffer->elements_size) {
              pglNamedBufferData(buffer->bo[0], elements.size(), elements.data(), type);
              buffer->elements_size = elements.size();
            } else {
              use_elements_edits = true;
            }
          }

          if (vertices.size() > buffer->vertices_size) {
            pglNamedBufferData(buffer->bo[1], vertices.size(), vertices.data(), type);
            buffer->vertices_size = vertices.size();
          } else {
            use_vertices_edits = true;
          }

          // Check for instance updates.
          if (format.is_instanced()) {
            const auto& instances = render_buffer->instances();
            if (instances.size() > buffer->instances_size) {
              pglNamedBufferData(buffer->bo[2], instances.size(), instances.data(), type);
              buffer->instances_size = instances.size();
            } else {
              use_instances_edits = true;
            }
          }

          // Enumerate and apply all buffer edits.
          if (use_vertices_edits || use_elements_edits || use_instances_edits) {
            const Size* edit = resource->edit<Size>();
            for (Size i{0}; i < resource->edits; i++) {
              switch (edit[0]) {
              case 0:
                if (use_elements_edits) {
                  const auto& elements = render_buffer->elements();
                  pglNamedBufferSubData(buffer->bo[0], edit[1], edit[2], elements.data() + edit[1]);
                }
                break;
              case 1:
                if (use_vertices_edits) {
                  pglNamedBufferSubData(buffer->bo[1], edit[1], edit[2], vertices.data() + edit[1]);
                }
                break;
              case 2:
                if (use_instances_edits) {
                  const auto& instances = render_buffer->instances();
                  pglNamedBufferSubData(buffer->bo[2], edit[1], edit[2], instances.data() + edit[1]);
                }
              }
              edit += 3;
            }
          }
        }
        break;
      case Frontend::UpdateCommand::Type::TEXTURE1D:
        // TODO(dweiler): Implement.
        break;
      case Frontend::UpdateCommand::Type::TEXTURE2D:
        {
          const auto render_texture = resource->as_texture2D;
          const auto texture = reinterpret_cast<detail_gl4::texture2D*>(render_texture + 1);
          const Size* edit = resource->edit<Size>();

          for (Size i = 0; i < resource->edits; i++) {
            const auto x_offset = edit[1];
            const auto y_offset = edit[2];
            const auto bpp = render_texture->bits_per_pixel() / 8;
            const auto pitch = render_texture->dimensions().w * bpp;
            const auto ptr = render_texture->data().data()
              + y_offset * pitch
              + x_offset * bpp;

            pglTextureSubImage2D(
              texture->tex,
              edit[0],
              x_offset,
              y_offset,
              edit[3], // width
              edit[4], // height
              convert_texture_format(render_texture->format()),
              convert_texture_data_type(render_texture->format()),
              ptr);

            edit += 5;
          }
        }
        break;
      case Frontend::UpdateCommand::Type::TEXTURE3D:
        {
          const auto render_texture = resource->as_texture3D;
          const auto texture = reinterpret_cast<detail_gl4::texture3D*>(render_texture + 1);
          const Size* edit = resource->edit<Size>();

          for (Size i = 0; i < resource->edits; i++) {
            const auto x_offset = edit[1];
            const auto y_offset = edit[2];
            const auto z_offset = edit[3];
            const auto bpp = render_texture->bits_per_pixel() / 8;
            const auto pitch = render_texture->dimensions().w * bpp;
            const auto ptr = render_texture->data().data()
              + z_offset * pitch * render_texture->dimensions().h
              + y_offset * pitch
              + x_offset * bpp;

            pglTextureSubImage3D(
              texture->tex,
              edit[0],
              x_offset,
              y_offset,
              z_offset,
              edit[4], // width
              edit[5], // height
              edit[6], // depth
              convert_texture_format(render_texture->format()),
              convert_texture_data_type(render_texture->format()),
              ptr);

            edit += 7;
          }
        }
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
      const auto this_target{reinterpret_cast<detail_gl4::target*>(render_target + 1)};
      const bool clear_depth{command->clear_depth};
      const bool clear_stencil{command->clear_stencil};

      // TODO(dweiler): optimize use_state to only consider the following
      // pieces of state that interact with a clear:
      //
      // * depth writes
      // * stencil writes
      // * scissor test
      // * blend write mask
      state->use_state(render_state);
      state->use_draw_target(render_target, &command->draw_buffers);

      const GLuint fbo{this_target->fbo};

      if (command->clear_colors) {
        for (Uint32 i{0}; i < 32; i++) {
          if (command->clear_colors & (1 << i)) {
            pglClearNamedFramebufferfv(fbo, GL_COLOR, static_cast<GLint>(i),
              command->color_values[i].data());
          }
        }
      }

      if (clear_depth && clear_stencil) {
        pglClearNamedFramebufferfi(fbo, GL_DEPTH_STENCIL, 0,
          command->depth_value, static_cast<GLint>(command->stencil_value));
      } else if (clear_depth) {
        pglClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &command->depth_value);
      } else if (clear_stencil) {
        const GLint stencil{command->stencil_value};
        pglClearNamedFramebufferiv(fbo, GL_STENCIL, 0, &stencil);
      }
    }
    break;
  case Frontend::CommandType::DRAW:
    {
      RX_PROFILE_CPU("draw");

      const auto command = reinterpret_cast<Frontend::DrawCommand*>(header + 1);
      const auto render_state = &command->render_state;
      const auto render_target = command->render_target;
      const auto render_buffer = command->render_buffer;
      const auto render_program = command->render_program;
      const auto this_program = reinterpret_cast<detail_gl4::program*>(render_program + 1);

      state->use_draw_target(render_target, &command->draw_buffers);
      state->use_buffer(render_buffer);
      state->use_program(render_program);
      state->use_state(render_state);

      // check for and apply uniform deltas
      if (command->dirty_uniforms_bitset) {
        const auto& program_uniforms{render_program->uniforms()};
        const Byte* draw_uniforms{command->uniforms()};

        for (Size i = 0; i < 64; i++) {
          if (command->dirty_uniforms_bitset & (1_u64 << i)) {
            const auto& uniform = program_uniforms[i];
            const auto location = this_program->uniforms[i];

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
              pglProgramUniform1i(this_program->handle, location,
                *reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32:
              pglProgramUniform1i(this_program->handle, location,
                *reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32:
              pglProgramUniform1fv(this_program->handle, location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x2:
              pglProgramUniform2iv(this_program->handle, location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x3:
              pglProgramUniform3iv(this_program->handle, location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::S32x4:
              pglProgramUniform4iv(this_program->handle, location, 1,
                reinterpret_cast<const Sint32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x2:
              pglProgramUniform2fv(this_program->handle, location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3:
              pglProgramUniform3fv(this_program->handle, location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x4:
              pglProgramUniform4fv(this_program->handle, location, 1,
                reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3x3:
              pglProgramUniformMatrix3fv(this_program->handle, location, 1,
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x3x4:
              pglProgramUniformMatrix3x4fv(this_program->handle, location, 1,
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::F32x4x4:
              pglProgramUniformMatrix4fv(this_program->handle, location, 1,
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::LB_BONES:
              pglProgramUniformMatrix3x4fv(this_program->handle, location,
                static_cast<GLsizei>(uniform.size() / sizeof(Math::Mat3x4f)),
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            case Frontend::Uniform::Type::DQ_BONES:
              pglProgramUniformMatrix2x4fv(this_program->handle, location,
                static_cast<GLsizei>(uniform.size() / sizeof(Math::DualQuatf)),
                GL_FALSE, reinterpret_cast<const Float32*>(draw_uniforms));
              break;
            }

            draw_uniforms += uniform.size();
          }
        }
      }

      // apply any textures
      for (Size i = 0; i < command->draw_textures.size(); i++) {
        Frontend::Texture* texture = command->draw_textures[i];
        switch (texture->resource_type()) {
          case Frontend::Resource::Type::TEXTURE1D:
            state->use_texture(static_cast<Frontend::Texture1D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURE2D:
            state->use_texture(static_cast<Frontend::Texture2D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURE3D:
            state->use_texture(static_cast<Frontend::Texture3D*>(texture), i);
            break;
          case Frontend::Resource::Type::TEXTURECM:
            state->use_texture(static_cast<Frontend::TextureCM*>(texture), i);
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
        const auto element_type = convert_element_type(format.element_type());
        const auto indices = reinterpret_cast<const GLvoid*>(format.element_size() * command->offset);
        if (command->instances) {
          const bool base_instance = command->base_instance != 0;
          if (format.is_indexed()) {
            const bool base_vertex = command->base_vertex != 0;
            if (base_vertex) {
              if (base_instance) {
                pglDrawElementsInstancedBaseVertexBaseInstance(
                  primitive_type,
                  count,
                  element_type,
                  indices,
                  static_cast<GLsizei>(command->instances),
                  static_cast<GLint>(command->base_vertex),
                  static_cast<GLuint>(command->base_instance));
              } else {
                pglDrawElementsInstancedBaseVertex(
                  primitive_type,
                  count,
                  element_type,
                  indices,
                  static_cast<GLsizei>(command->instances),
                  static_cast<GLint>(command->base_vertex));
              }
            } else if (base_instance) {
              pglDrawElementsInstancedBaseInstance(
                primitive_type,
                count,
                element_type,
                indices,
                static_cast<GLsizei>(command->instances),
                static_cast<GLint>(command->base_instance));
            } else {
              pglDrawElementsInstanced(
                primitive_type,
                count,
                element_type,
                indices,
                static_cast<GLsizei>(command->instances));
            }
          } else {
            if (base_instance) {
              pglDrawArraysInstancedBaseInstance(
                primitive_type,
                offset,
                count,
                static_cast<GLsizei>(command->instances),
                static_cast<GLuint>(command->base_instance));
            } else {
              pglDrawArraysInstanced(
                primitive_type,
                offset,
                count,
                static_cast<GLsizei>(command->instances));
            }
          }
        } else {
          if (format.is_indexed()) {
            if (command->base_vertex) {
              pglDrawElementsBaseVertex(
                primitive_type,
                count,
                element_type,
                indices,
                static_cast<GLint>(command->base_vertex));
            } else {
              pglDrawElements(primitive_type, count, element_type, indices);
            }
          } else {
            pglDrawArrays(primitive_type, offset, count);
          }
        }
      } else {
        // Bufferless draw calls
        pglDrawArrays(primitive_type, 0, count);
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

      const auto* src_render_target{command->src_target};
      const auto* dst_render_target{command->dst_target};

      const auto src_attachment{command->src_attachment};
      const auto dst_attachment{command->dst_attachment};

      const auto src_dimensions{src_render_target->attachments()[src_attachment].as_texture2D.texture->dimensions().cast<GLint>()};
      const auto dst_dimensions{dst_render_target->attachments()[dst_attachment].as_texture2D.texture->dimensions().cast<GLint>()};

      const GLuint src_fbo{reinterpret_cast<const detail_gl4::target*>(src_render_target + 1)->fbo};
      const GLuint dst_fbo{reinterpret_cast<const detail_gl4::target*>(dst_render_target + 1)->fbo};

      pglNamedFramebufferReadBuffer(
        src_fbo,
        static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + src_attachment));

      // Can't change the draw buffer on the swapchain.
      if (!dst_render_target->is_swapchain()) {
        pglNamedFramebufferDrawBuffer(
          dst_fbo,
          static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + dst_attachment));
      }

      pglBlitNamedFramebuffer(
        src_fbo,
        dst_fbo,
        0,
        0,
        src_dimensions.w,
        src_dimensions.h,
        0,
        0,
        dst_dimensions.w,
        dst_dimensions.h,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST);

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

void GL4::swap() {
  RX_PROFILE_CPU("swap");
  SDL_GL_SwapWindow(reinterpret_cast<SDL_Window*>(m_data));
}

} // namespace Rx::Render::Backend
