#include <stdarg.h> // va_list, va_start, va_end
#include <string.h> // strlen

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/material.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/filesystem/directory.h"

#include "rx/core/profiler.h"
#include "rx/core/log.h"

#include "rx/console/variable.h"
#include "rx/console/interface.h"

RX_CONSOLE_IVAR(max_buffers, "render.max_buffers","maximum buffers", 16, 128, 64);
RX_CONSOLE_IVAR(max_targets, "render.max_targets", "maximum targets", 16, 128, 16);
RX_CONSOLE_IVAR(max_programs, "render.max_programs", "maximum programs", 128, 4096, 512);
RX_CONSOLE_IVAR(max_texture1D, "render.max_texture1D", "maximum 1D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_texture2D, "render.max_texture2D", "maximum 2D textures", 16, 4096, 1024);
RX_CONSOLE_IVAR(max_texture3D, "render.max_texture3D", "maximum 3D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_textureCM, "render.max_textureCM", "maximum CM textures", 16, 128, 16);
RX_CONSOLE_IVAR(command_memory, "render.command_memory", "memory for command buffer in MiB", 1, 4, 2);

RX_LOG("render", render_log);

static constexpr const char* k_technique_path{"base/renderer/techniques"};

namespace rx::render::frontend {

#define allocate_command(data_type, type) \
  m_command_buffer.allocate(sizeof(data_type), (type), _info)

interface::interface(memory::allocator* _allocator, backend::interface* _backend)
  : m_allocator{_allocator}
  , m_backend{_backend}
  , m_allocation_info{m_backend->query_allocation_info()}
  , m_buffer_pool{m_allocator, m_allocation_info.buffer_size + sizeof(buffer), static_cast<rx_size>(*max_buffers)}
  , m_target_pool{m_allocator, m_allocation_info.target_size + sizeof(target), static_cast<rx_size>(*max_targets)}
  , m_program_pool{m_allocator, m_allocation_info.program_size + sizeof(program), static_cast<rx_size>(*max_programs)}
  , m_texture1D_pool{m_allocator, m_allocation_info.texture1D_size + sizeof(texture1D), static_cast<rx_size>(*max_texture1D)}
  , m_texture2D_pool{m_allocator, m_allocation_info.texture2D_size + sizeof(texture2D), static_cast<rx_size>(*max_texture2D)}
  , m_texture3D_pool{m_allocator, m_allocation_info.texture3D_size + sizeof(texture3D), static_cast<rx_size>(*max_texture3D)}
  , m_textureCM_pool{m_allocator, m_allocation_info.textureCM_size + sizeof(textureCM), static_cast<rx_size>(*max_textureCM)}
  , m_destroy_buffers{m_allocator}
  , m_destroy_targets{m_allocator}
  , m_destroy_textures1D{m_allocator}
  , m_destroy_textures2D{m_allocator}
  , m_destroy_textures3D{m_allocator}
  , m_destroy_texturesCM{m_allocator}
  , m_swapchain_target{nullptr}
  , m_swapchain_texture{nullptr}
  , m_commands{m_allocator}
  , m_command_buffer{m_allocator, static_cast<rx_size>(*command_memory) * 1024 * 1024}
  , m_deferred_process{[this]() { process(); }}
  , m_device_info{m_allocator}
{
  memset(m_resource_usage, 0, sizeof m_resource_usage);

  // Cache the device information from the backend.
  const auto& info{m_backend->query_device_info()};
  m_device_info.vendor = info.vendor;
  m_device_info.renderer = info.renderer;
  m_device_info.version = info.version;

  // load all techniques
  if (filesystem::directory directory{k_technique_path}) {
    directory.each([this](filesystem::directory::item&& _item) {
      if (_item.is_file() && _item.name().ends_with(".json5")) {
        technique new_technique{this};
        const auto path{string::format("%s/%s", k_technique_path,
          utility::move(_item.name()))};
        if (new_technique.load(path)) {
          m_techniques.insert(new_technique.name(), utility::move(new_technique));
        }
      }
    });
  }

  // Generate swapchain target.
  static auto& dimensions{console::interface::get_from_name("display.resolution")->cast<math::vec2i>()->get()};
  static auto& hdr{console::interface::get_from_name("display.hdr")->cast<bool>()->get()};

  m_swapchain_texture = create_texture2D(RX_RENDER_TAG("swapchain"));
  m_swapchain_texture->record_type(texture::type::k_attachment);
  m_swapchain_texture->record_filter({false, false, false});
  m_swapchain_texture->record_format(hdr ? texture::data_format::k_rgba_f16 : texture::data_format::k_rgba_u8);
  m_swapchain_texture->record_dimensions(dimensions.cast<rx_size>());
  m_swapchain_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});

  m_swapchain_target = create_target(RX_RENDER_TAG("swapchain"));
  m_swapchain_target->attach_texture(m_swapchain_texture);
  m_swapchain_target->m_flags |= target::k_swapchain;
}

interface::~interface() {
  destroy_target(RX_RENDER_TAG("swapchain"), m_swapchain_target);
  destroy_texture(RX_RENDER_TAG("swapchain"), m_swapchain_texture);
}

// create_*
buffer* interface::create_buffer(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_buffer;
  command->as_buffer = m_buffer_pool.create<buffer>(this);
  m_commands.push_back(command_base);
  return command->as_buffer;
}

target* interface::create_target(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_target;
  command->as_target = m_target_pool.create<target>(this);
  m_commands.push_back(command_base);
  return command->as_target;
}

program* interface::create_program(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_program;
  command->as_program = m_program_pool.create<program>(this);
  m_commands.push_back(command_base);
  return command->as_program;
}

texture1D* interface::create_texture1D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture1D;
  command->as_texture1D = m_texture1D_pool.create<texture1D>(this);
  m_commands.push_back(command_base);
  return command->as_texture1D;
}

texture2D* interface::create_texture2D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture2D;
  command->as_texture2D = m_texture2D_pool.create<texture2D>(this);
  m_commands.push_back(command_base);
  return command->as_texture2D;
}

texture3D* interface::create_texture3D(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture3D;
  command->as_texture3D = m_texture3D_pool.create<texture3D>(this);
  m_commands.push_back(command_base);
  return command->as_texture3D;
}

textureCM* interface::create_textureCM(const command_header::info& _info) {
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_allocate)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_textureCM;
  command->as_textureCM = m_textureCM_pool.create<textureCM>(this);
  m_commands.push_back(command_base);
  return command->as_textureCM;
}

// initialize_*
void interface::initialize_buffer(const command_header::info& _info, buffer* _buffer) {
  RX_ASSERT(_buffer, "_buffer is null");
  _buffer->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_buffer;
  command->as_buffer = _buffer;
  m_commands.push_back(command_base);
}

void interface::initialize_target(const command_header::info& _info, target* _target) {
  RX_ASSERT(_target, "_target is null");
  _target->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_target;
  command->as_target = _target;
  m_commands.push_back(command_base);
}

void interface::initialize_program(const command_header::info& _info, program* _program) {
  RX_ASSERT(_program, "_program is null");
  _program->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_program;
  command->as_program = _program;
  m_commands.push_back(command_base);
}

void interface::initialize_texture(const command_header::info& _info, texture1D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture1D;
  command->as_texture1D = _texture;
  m_commands.push_back(command_base);
}

void interface::initialize_texture(const command_header::info& _info, texture2D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture2D;
  command->as_texture2D = _texture;
  m_commands.push_back(command_base);
}

void interface::initialize_texture(const command_header::info& _info, texture3D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_texture3D;
  command->as_texture3D = _texture;
  m_commands.push_back(command_base);
}

void interface::initialize_texture(const command_header::info& _info, textureCM* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();
  concurrency::scope_lock lock(m_mutex);
  auto command_base{allocate_command(resource_command, command_type::k_resource_construct)};
  auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
  command->kind = resource_command::type::k_textureCM;
  command->as_textureCM = _texture;
  m_commands.push_back(command_base);
}

// update_*
void interface::update_buffer(const command_header::info& _info, buffer* _buffer) {
  if (_buffer) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_update)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_buffer;
    command->as_buffer = _buffer;
    m_commands.push_back(command_base);
  }
}

// destroy_*
void interface::destroy_buffer(const command_header::info& _info, buffer* _buffer) {
  if (_buffer) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_buffer;
    command->as_buffer = _buffer;
    m_commands.push_back(command_base);
    m_destroy_buffers.push_back(_buffer);
  }
}

void interface::destroy_target(const command_header::info& _info, target* _target) {
  if (_target) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_target;
    command->as_target = _target;
    m_commands.push_back(command_base);
    m_destroy_targets.push_back(_target);
  }
}

void interface::destroy_program(const command_header::info& _info, program* _program) {
  if (_program) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_program;
    command->as_program = _program;
    m_commands.push_back(command_base);
    m_destroy_programs.push_back(_program);
  }
}

void interface::destroy_texture(const command_header::info& _info, texture1D* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_texture1D;
    command->as_texture1D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures1D.push_back(_texture);
  }
}

void interface::destroy_texture(const command_header::info& _info, texture2D* _texture) {
  concurrency::scope_lock lock(m_mutex);
  destroy_texture_unlocked(_info, _texture);
}

void interface::destroy_texture(const command_header::info& _info, texture3D* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_texture3D;
    command->as_texture3D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures3D.push_back(_texture);
  }
}

void interface::destroy_texture(const command_header::info& _info, textureCM* _texture) {
  if (_texture) {
    concurrency::scope_lock lock(m_mutex);
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_textureCM;
    command->as_textureCM = _texture;
    m_commands.push_back(command_base);
    m_destroy_texturesCM.push_back(_texture);
  }
}

void interface::destroy_texture_unlocked(const command_header::info& _info, texture2D* _texture) {
  if (_texture) {
    auto command_base{allocate_command(resource_command, command_type::k_resource_destroy)};
    auto command{reinterpret_cast<resource_command*>(command_base + sizeof(command_header))};
    command->kind = resource_command::type::k_texture2D;
    command->as_texture2D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures2D.push_back(_texture);
  }
}

void interface::draw(
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

  m_vertices[0] += _count;

  switch (_primitive_type) {
  case primitive_type::k_lines:
    m_lines[0] += _count / 2;
    break;
  case primitive_type::k_points:
    m_points[0] += _count;
    break;
  case primitive_type::k_triangle_strip:
    m_triangles[0] += _count - 2;
    break;
  case primitive_type::k_triangles:
    m_triangles[0] += _count / 3;
    break;
  }

  concurrency::scope_lock lock(m_mutex);

  const auto dirty_uniforms_size{_program->dirty_uniforms_size()};

  // allocate and fill out command
  auto command_base{m_command_buffer.allocate(sizeof(draw_command) + dirty_uniforms_size, command_type::k_draw, _info)};
  auto command{reinterpret_cast<draw_command*>(command_base + sizeof(command_header))};
  *reinterpret_cast<state*>(command) = _state;
  reinterpret_cast<state*>(command)->flush();
  command->render_target = _target;
  command->render_buffer = _buffer;
  command->render_program = _program;
  command->count = _count;
  command->offset = _offset;
  command->type = _primitive_type;
  command->dirty_uniforms_bitset = _program->dirty_uniforms_bitset();

  // copy the uniforms directly into the command structure, if any
  if (dirty_uniforms_size) {
    _program->flush_dirty_uniforms(command->uniforms());
  }

  memset(command->texture_types, 0, sizeof command->texture_types);

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

  m_draw_calls[0]++;
}

void interface::clear(
  const command_header::info& _info,
  const state& _state,
  target* _target,
  rx_u32 _clear_mask,
  const math::vec4f& _clear_color)
{
  RX_ASSERT(_clear_mask, "empty clear");

  {
    concurrency::scope_lock lock(m_mutex);

    auto command_base{allocate_command(draw_command, command_type::k_clear)};
    auto command{reinterpret_cast<clear_command*>(command_base + sizeof(command_header))};
    *reinterpret_cast<state*>(command) = _state;
    reinterpret_cast<state*>(command)->flush();

    command->render_target = _target;
    command->clear_mask = _clear_mask;
    command->clear_color = _clear_color;

    m_commands.push_back(command_base);
  }

  m_clear_calls[0]++;
}

void interface::blit(
  const command_header::info& _info,
  const state& _state,
  target* _src_target,
  rx_size _src_attachment,
  target* _dst_target,
  rx_size _dst_attachment)
{
  // Blitting from an attachment in a target to another attachment in the same
  // target is not allowed.
  RX_ASSERT(_src_target != _dst_target, "cannot blit to self");

  // It's not valid to source the swapchain in a blit. The swapchain is only
  // allowed to be a destination.
  RX_ASSERT(!_src_target->is_swapchain(), "cannot use swapchain as source");

  const auto& src_attachments{_src_target->attachments()};
  RX_ASSERT(_src_attachment < src_attachments.size(),
    "source attachment out of bounds");
  const auto& dst_attachments{_dst_target->attachments()};
  RX_ASSERT(_dst_attachment < dst_attachments.size(),
    "destination attachment out of bounds");

  texture2D* src_attachment{src_attachments[_src_attachment]};
  texture2D* dst_attachment{dst_attachments[_dst_attachment]};

  // It's possible for targets to be configured in a way where attachments are
  // shared between them. Blitting to and from the same attachment doesn't make
  // any sense.
  RX_ASSERT(src_attachment != dst_attachment, "cannot blit to self");

  // It's only valid to blit color attachments.
  RX_ASSERT(src_attachment->is_color_format(),
    "cannot blit with non-color source attachment");
  RX_ASSERT(dst_attachment->is_color_format(),
    "cannot blit with non-color destination attachment");

  const auto is_float_color{[](texture::data_format _format) {
    return  _format == texture::data_format::k_bgra_f16 ||
            _format == texture::data_format::k_rgba_f16;
  }};

  // A blit from one target to another is only valid if the source and
  // destination attachments contain similar data formats. That is they both
  // must use floating-point attachments or integer attachments. Mixing is
  // not allowed.
  RX_ASSERT(is_float_color(src_attachment->format()) == is_float_color(dst_attachment->format()),
    "incompatible formats between attachments");

  {
    concurrency::scope_lock lock{m_mutex};

    auto command_base{allocate_command(blit_command, command_type::k_blit)};
    auto command{reinterpret_cast<blit_command*>(command_base + sizeof(command_header))};
    *reinterpret_cast<state*>(command) = _state;
    reinterpret_cast<state*>(command)->flush();

    command->src_target = _src_target;
    command->src_attachment = _src_attachment;
    command->dst_target = _dst_target;
    command->dst_attachment = _dst_attachment;

    m_commands.push_back(command_base);
  }

  m_blit_calls[0]++;
}

void interface::profile(const char* _tag) {
  concurrency::scope_lock lock{m_mutex};

  auto command_base{m_command_buffer.allocate(sizeof(profile_command),
    command_type::k_profile, RX_RENDER_TAG("profile"))};
  auto command{reinterpret_cast<profile_command*>(command_base + sizeof(command_header))};
  command->tag = _tag;

  m_commands.push_back(command_base);
}

void interface::resize(const math::vec2z& _resolution) {
  // Resizing the swapchain is just a matter of updating these fields.
  m_swapchain_texture->m_dimensions = _resolution;
  m_swapchain_target->m_dimensions = _resolution;
}

bool interface::process() {
  profiler::cpu_sample sample{"frontend::process"};

  // concurrency::scope_lock lock(m_mutex);

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
    m_buffer_pool.destroy<buffer>(_buffer);
  });

  m_destroy_targets.each_fwd([this](target* _target) {
    m_target_pool.destroy<target>(_target);
  });

  m_destroy_programs.each_fwd([this](program* _program) {
    m_program_pool.destroy<program>(_program);
  });

  m_destroy_textures1D.each_fwd([this](texture1D* _texture) {
    m_texture1D_pool.destroy<texture1D>(_texture);
  });

  m_destroy_textures2D.each_fwd([this](texture2D* _texture) {
    m_texture2D_pool.destroy<texture2D>(_texture);
  });

  m_destroy_textures3D.each_fwd([this](texture3D* _texture) {
    m_texture3D_pool.destroy<texture3D>(_texture);
  });

  m_destroy_texturesCM.each_fwd([this](textureCM* _texture) {
    m_textureCM_pool.destroy<textureCM>(_texture);
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

  auto swap{[](concurrency::atomic<rx_size> (&_value)[2]) {
    _value[1] = _value[0].load();
    _value[0] = 0;
  }};

  swap(m_draw_calls);
  swap(m_clear_calls);
  swap(m_blit_calls);
  swap(m_vertices);
  swap(m_points);
  swap(m_lines);
  swap(m_triangles);

  return true;
}

interface::statistics interface::stats(resource::type _type) const {
  concurrency::scope_lock lock(m_mutex);

  const auto index{static_cast<rx_size>(_type)};
  switch (_type) {
  case resource::type::k_buffer:
    return {m_buffer_pool.capacity(), m_buffer_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_program:
    return {m_program_pool.capacity(), m_program_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_target:
    return {m_target_pool.capacity(), m_target_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_texture1D:
    return {m_texture1D_pool.capacity(), m_texture1D_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_texture2D:
    return {m_texture2D_pool.capacity(), m_texture2D_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_texture3D:
    return {m_texture3D_pool.capacity(), m_texture3D_pool.size(), 0, m_resource_usage[index]};
  case resource::type::k_textureCM:
    return {m_textureCM_pool.capacity(), m_textureCM_pool.size(), 0, m_resource_usage[index]};
  }

  RX_HINT_UNREACHABLE();
}

bool interface::swap() {
  profiler::cpu_sample sample{"frontend::swap"};

  m_backend->swap();
  return m_timer.update();
}

technique* interface::find_technique_by_name(const char* _name) {
  return m_techniques.find(_name);
}

} // namespace rx::render::frontend
