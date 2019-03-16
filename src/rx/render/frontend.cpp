#include <stdarg.h> // va_list, va_start, va_end
#include <string.h> // strlen

#include <rx/render/frontend.h>
#include <rx/render/backend.h>
#include <rx/render/buffer.h>
#include <rx/render/target.h>
#include <rx/render/program.h>
#include <rx/render/texture.h>

#include <rx/core/concurrency/scope_lock.h>
#include <rx/core/log.h>

#include <rx/console/variable.h>

RX_CONSOLE_IVAR(max_buffers, "render.max_buffers","maximum buffers", 16, 128, 64);
RX_CONSOLE_IVAR(max_targets, "render.max_targets", "maximum targets", 16, 128, 16);
RX_CONSOLE_IVAR(max_programs, "render.max_programs", "maximum programs", 128, 4096, 512);
RX_CONSOLE_IVAR(max_texture1D, "render.max_texture1D", "maximum 1D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_texture2D, "render.max_texture2D", "maximum 2D textures", 16, 4096, 1024);
RX_CONSOLE_IVAR(max_texture3D, "render.max_texture3D", "maximum 3D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_textureCM, "render.max_textureCM", "maximum CM textures", 16, 128, 16);
RX_CONSOLE_IVAR(command_memory, "render.command_memory", "memory for command buffer in MiB", 16, 64, 16);

RX_LOG("render", render_log);

namespace rx::render {

#define allocate_command(data_type, type) \
  m_command_buffer.allocate(sizeof(data_type), (type), _info)

frontend::frontend(memory::allocator* _allocator, backend* _backend, const allocation_info& _allocation_info)
  : m_buffer_pool{_allocator, _allocation_info.buffer_size + sizeof(buffer), static_cast<rx_size>(*max_buffers)}
  , m_target_pool{_allocator, _allocation_info.target_size + sizeof(target), static_cast<rx_size>(*max_targets)}
  , m_program_pool{_allocator, _allocation_info.program_size + sizeof(program), static_cast<rx_size>(*max_programs)}
  , m_texture1D_pool{_allocator, _allocation_info.texture1D_size + sizeof(texture1D), static_cast<rx_size>(*max_texture1D)}
  , m_texture2D_pool{_allocator, _allocation_info.texture2D_size + sizeof(texture2D), static_cast<rx_size>(*max_texture2D)}
  , m_texture3D_pool{_allocator, _allocation_info.texture3D_size + sizeof(texture3D), static_cast<rx_size>(*max_texture3D)}
  , m_textureCM_pool{_allocator, _allocation_info.textureCM_size + sizeof(textureCM), static_cast<rx_size>(*max_textureCM)}
  , m_destroy_buffers{_allocator}
  , m_destroy_targets{_allocator}
  , m_destroy_textures1D{_allocator}
  , m_destroy_textures2D{_allocator}
  , m_destroy_textures3D{_allocator}
  , m_destroy_texturesCM{_allocator}
  , m_commands{_allocator}
  , m_command_buffer{_allocator, static_cast<rx_size>(*command_memory * 1024 * 1024)}
  , m_backend{_backend}
{
}

frontend::~frontend() {
}

// create_*
buffer* frontend::create_buffer(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_buffer;
  command->as_buffer = m_buffer_pool.allocate_and_construct<buffer>();
  m_commands.push_back(command_base);
  return command->as_buffer;
}

target* frontend::create_target(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_target;
  command->as_target = m_target_pool.allocate_and_construct<target>(this);
  m_commands.push_back(command_base);
  return command->as_target;
}

program* frontend::create_program(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_program;
  command->as_program = m_program_pool.allocate_and_construct<program>();
  m_commands.push_back(command_base);
  return command->as_program;
}

texture1D* frontend::create_texture1D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture1D;
  command->as_texture1D = m_texture1D_pool.allocate_and_construct<texture1D>();
  m_commands.push_back(command_base);
  return command->as_texture1D;
}

texture2D* frontend::create_texture2D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture2D;
  command->as_texture2D = m_texture2D_pool.allocate_and_construct<texture2D>();
  m_commands.push_back(command_base);
  return command->as_texture2D;
}

texture3D* frontend::create_texture3D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture3D;
  command->as_texture3D = m_texture3D_pool.allocate_and_construct<texture3D>();
  m_commands.push_back(command_base);
  return command->as_texture3D;
}

textureCM* frontend::create_textureCM(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_textureCM;
  command->as_textureCM = m_textureCM_pool.allocate_and_construct<textureCM>();
  m_commands.push_back(command_base);
  return command->as_textureCM;
}

// initialize_*
void frontend::initialize_buffer(const command_header::info& _info, buffer* _buffer) {
  RX_ASSERT(_buffer, "_buffer is null");
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_buffer;
  command->as_buffer = _buffer;
  m_commands.push_back(command_base);
}

void frontend::initialize_target(const command_header::info& _info, target* _target) {
  RX_ASSERT(_target, "_target is null");
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_target;
  command->as_target = _target;
  m_commands.push_back(command_base);
}

void frontend::initialize_program(const command_header::info& _info, program* _program) {
  RX_ASSERT(_program->validate(), "_program is not valid");
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_program;
  command->as_program = _program;
  m_commands.push_back(command_base);
}

void frontend::initialize_texture(const command_header::info& _info, texture1D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  RX_ASSERT(_texture->validate(), "_texture is not valid");

  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture1D;
  command->as_texture1D = _texture;
  m_commands.push_back(command_base);
}

void frontend::initialize_texture(const command_header::info& _info, texture2D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  RX_ASSERT(_texture->validate(), "_texture is not valid");

  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture2D;
  command->as_texture2D = _texture;
  m_commands.push_back(command_base);
}

void frontend::initialize_texture(const command_header::info& _info, texture3D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  RX_ASSERT(_texture->validate(), "_texture is not valid");

  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_texture3D;
  command->as_texture3D = _texture;
  m_commands.push_back(command_base);
}

void frontend::initialize_texture(const command_header::info& _info, textureCM* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  RX_ASSERT(_texture->validate(), "_texture is not valid");

  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->type = resource_command::category::k_textureCM;
  command->as_textureCM = _texture;
  m_commands.push_back(command_base);
}

// destroy_*
void frontend::destroy_buffer(const command_header::info& _info, buffer* _buffer) {
  if (_buffer) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_buffer;
    command->as_buffer = _buffer;
    m_commands.push_back(command_base);
    m_destroy_buffers.push_back(_buffer);
  }
}

void frontend::destroy_target(const command_header::info& _info, target* _target) {
  if (_target) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_target;
    command->as_target = _target;
    m_commands.push_back(command_base);
    m_destroy_targets.push_back(_target);
  }
}

void frontend::destroy_program(const command_header::info& _info, program* _program) {
  if (_program) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_program;
    command->as_program = _program;
    m_commands.push_back(command_base);
    m_destroy_programs.push_back(_program);
  }
}

void frontend::destroy_texture(const command_header::info& _info, texture1D* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_texture1D;
    command->as_texture1D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures1D.push_back(_texture);
  }
}

void frontend::destroy_texture(const command_header::info& _info, texture2D* _texture) {
  concurrency::scope_lock lock(m_mutex);
  destroy_texture_unlocked(_info, _texture);
}

void frontend::destroy_texture(const command_header::info& _info, texture3D* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_texture3D;
    command->as_texture3D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures3D.push_back(_texture);
  }
}

void frontend::destroy_texture(const command_header::info& _info, textureCM* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_textureCM;
    command->as_textureCM = _texture;
    m_commands.push_back(command_base);
    m_destroy_texturesCM.push_back(_texture);
  }
}

void frontend::destroy_texture_unlocked(const command_header::info& _info, texture2D* _texture) {
  if (_texture) {
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->type = resource_command::category::k_texture2D;
    command->as_texture2D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures2D.push_back(_texture);
  }
}

void frontend::draw_elements(
  const command_header::info& _info,
  const state& _state,
  target* _target,
  buffer* _buffer,
  program* _program,
  rx_size _count,
  rx_size _offset,
  primitive_type _primitive_type,
  const char* _textures,
  ...)
{
  if (_count == 0) {
    return;
  }

  concurrency::scope_lock<concurrency::mutex> lock(m_mutex);

  // gather all dirty uniforms
  array<rx_byte> uniforms{utility::move(_program->flush())};

  // allocate and fill out command
  auto command_base{m_command_buffer.allocate(sizeof(draw_command) + uniforms.size(), command_type::k_draw_elements, _info)};
  auto command{reinterpret_cast<draw_command*>(command_base + sizeof(command_header))};
  *reinterpret_cast<state*>(command) = _state;
  command->render_target = _target;
  command->render_buffer = _buffer;
  command->render_program = _program;
  command->count = _count;
  command->offset = _offset;
  command->type = _primitive_type;

  // copy the uniforms directly into the command structure, if any
  if (!uniforms.is_empty()) {
    memcpy(command->uniforms(), uniforms.data(), uniforms.size());
  }

  rx_size textures{strlen(_textures)};
  if (textures) {
    va_list va;
    va_start(va, _textures);
    for (rx_size i{0}; i < textures; i++) {
      const char ch{_textures[i]};
      command->texture_types[i] = ch;
      command->texture_binds[i] = va_arg(va, void*);
    }
    va_end(va);
  }

  m_commands.push_back(command_base);
}

void frontend::clear(
  const command_header::info& _info,
  target* _target,
  rx_u32 _clear_mask,
  const math::vec4f& _clear_color)
{
  RX_ASSERT(_clear_mask, "empty clear");
  concurrency::scope_lock<concurrency::mutex> lock(m_mutex);

  auto command_base{allocate_command(draw_command, command_type::k_clear)};
  auto command{reinterpret_cast<clear_command*>(command_base + sizeof(command_header))};

  command->render_target = _target;
  command->clear_mask = _clear_mask;
  command->clear_color = _clear_color;

  m_commands.push_back(command_base);
}

bool frontend::process() {
  concurrency::scope_lock<concurrency::mutex> lock(m_mutex);

  if (m_commands.is_empty()) {
    return false;
  }

  // consume all commands
  if (m_backend) {
    m_commands.each_fwd([this](rx_byte* _command) {
      m_backend->process(_command);
    });
  }

  // cleanup unreferenced resources
  m_destroy_buffers.each_fwd([this](buffer* _buffer) {
    m_buffer_pool.destruct_and_deallocate<buffer>(_buffer);
  });

  m_destroy_targets.each_fwd([this](target* _target) {
    m_target_pool.destruct_and_deallocate<target>(_target);
  });

  m_destroy_textures1D.each_fwd([this](texture1D* _texture) {
    m_texture1D_pool.destruct_and_deallocate<texture1D>(_texture);
  });

  m_destroy_textures2D.each_fwd([this](texture2D* _texture) {
    m_texture2D_pool.destruct_and_deallocate<texture2D>(_texture);
  });

  m_destroy_textures3D.each_fwd([this](texture3D* _texture) {
    m_texture3D_pool.destruct_and_deallocate<texture3D>(_texture);
  });

  m_destroy_texturesCM.each_fwd([this](textureCM* _texture) {
    m_texture1D_pool.destruct_and_deallocate<textureCM>(_texture);
  });

  // clear lists
  m_commands.clear();

  m_destroy_buffers.clear();
  m_destroy_targets.clear();
  m_destroy_programs.clear();
  m_destroy_textures1D.clear();
  m_destroy_textures2D.clear();
  m_destroy_textures3D.clear();
  m_destroy_texturesCM.clear();

  m_command_buffer.reset();

  return true;
}

void frontend::swap() {
  if (m_backend) {
    m_backend->swap();
  }
}

} // namespace rx::render
