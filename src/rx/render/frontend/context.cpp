#include <stdarg.h> // va_list, va_start, va_end
#include <stddef.h> // offsetof
#include <string.h> // strlen

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/downloader.h"

#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/module.h"
// #include "rx/render/frontend/routine.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/filesystem/directory.h"

#include "rx/core/profiler.h"
#include "rx/core/log.h"
#include "rx/core/time/stop_watch.h"

#include "rx/console/variable.h"

RX_CONSOLE_IVAR(max_buffers, "render.max_buffers","maximum buffers", 16, 128, 64);
RX_CONSOLE_IVAR(max_targets, "render.max_targets", "maximum targets", 16, 1024, 512);
RX_CONSOLE_IVAR(max_programs, "render.max_programs", "maximum programs", 128, 4096, 512);
RX_CONSOLE_IVAR(max_texture1D, "render.max_texture1D", "maximum 1D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_texture2D, "render.max_texture2D", "maximum 2D textures", 16, 4096, 1024);
RX_CONSOLE_IVAR(max_texture3D, "render.max_texture3D", "maximum 3D textures", 16, 128, 16);
RX_CONSOLE_IVAR(max_textureCM, "render.max_textureCM", "maximum CM textures", 16, 256, 128);
RX_CONSOLE_IVAR(max_downloaders, "render.max_downloaders", "maximum downloaders", 2, 16, 8);
RX_CONSOLE_IVAR(command_memory, "render.command_memory", "memory for command buffer in MiB", 1, 4, 2);

RX_CONSOLE_V2IVAR(
  max_texture_dimensions,
  "render.max_texture_dimensions",
  "hard limit on the maximum texture dimension for all textures",
  Rx::Math::Vec2i(4, 4),
  Rx::Math::Vec2i(4096, 4096),
  Rx::Math::Vec2i(2048, 2048));

RX_LOG("render", logger);

static constexpr const char* TECHNIQUES_PATH = "base/renderer/techniques";
static constexpr const char* MODULES_PATH = "base/renderer/modules";
static constexpr const char* ROUTINES_PATH = "base/renderer/routines";

namespace Rx::Render::Frontend {

// Limit the caches for render frontend caches to a maximum of one, this
// models a static pool with a fixed-capacity.
static Optional<Memory::Slab>
create_slab(Memory::Allocator& _allocator, Size _object_size, Size _object_count) {
  return Memory::Slab::create(_allocator, _object_size, _object_count, 1, 1);
}

Context::Context(Memory::Allocator& _allocator, Backend::Context* _backend, const Math::Vec2z& _dimensions, bool _hdr)
  : m_allocator{_allocator}
  , m_backend{_backend}
  , m_allocation_info{m_backend->query_allocation_info()}
  , m_buffer_pool{Utility::move(*create_slab(allocator(), m_allocation_info.buffer_size + sizeof(Buffer), static_cast<Size>(*max_buffers)))}
  , m_target_pool{Utility::move(*create_slab(allocator(), m_allocation_info.target_size + sizeof(Target), static_cast<Size>(*max_targets)))}
  , m_program_pool{Utility::move(*create_slab(allocator(), m_allocation_info.program_size + sizeof(Program), static_cast<Size>(*max_programs)))}
  , m_texture1D_pool{Utility::move(*create_slab(allocator(), m_allocation_info.texture1D_size + sizeof(Texture1D), static_cast<Size>(*max_texture1D)))}
  , m_texture2D_pool{Utility::move(*create_slab(allocator(), m_allocation_info.texture2D_size + sizeof(Texture2D), static_cast<Size>(*max_texture2D)))}
  , m_texture3D_pool{Utility::move(*create_slab(allocator(), m_allocation_info.texture3D_size + sizeof(Texture3D), static_cast<Size>(*max_texture3D)))}
  , m_textureCM_pool{Utility::move(*create_slab(allocator(), m_allocation_info.textureCM_size + sizeof(TextureCM), static_cast<Size>(*max_textureCM)))}
  , m_downloader_pool{Utility::move(*create_slab(allocator(), m_allocation_info.downloader_size + sizeof(Downloader), static_cast<Size>(*max_downloaders)))}
  , m_destroy_buffers{allocator()}
  , m_destroy_targets{allocator()}
  , m_destroy_textures1D{allocator()}
  , m_destroy_textures2D{allocator()}
  , m_destroy_textures3D{allocator()}
  , m_destroy_texturesCM{allocator()}
  , m_destroy_downloaders{allocator()}
  , m_edit_buffers{allocator()}
  , m_edit_textures1D{allocator()}
  , m_edit_textures2D{allocator()}
  , m_edit_textures3D{allocator()}
  , m_edit_texturesCM{allocator()}
  , m_swapchain_target{nullptr}
  , m_swapchain_texture{nullptr}
  , m_commands{allocator()}
  , m_command_buffer{allocator(), static_cast<Size>(*command_memory) * 1024 * 1024}
  , m_cached_buffers{allocator()}
  , m_cached_targets{allocator()}
  , m_cached_textures1D{allocator()}
  , m_cached_textures2D{allocator()}
  , m_cached_textures3D{allocator()}
  , m_cached_texturesCM{allocator()}
  , m_techniques{allocator()}
  , m_modules{allocator()}
  // , m_routines{allocator()}
  , m_arenas{allocator()}
  , m_draw_calls{0, 0}
  , m_instanced_draw_calls{0, 0}
  , m_clear_calls{0, 0}
  , m_blit_calls{0, 0}
  , m_vertices{0, 0}
  , m_triangles{0, 0}
  , m_lines{0, 0}
  , m_points{0, 0}
  , m_commands_recorded{0, 0}
  , m_footprint{0, 0}
  , m_frame{0}
  , m_resource_usage{}
  , m_device_info{allocator()}
  , m_timer{}
{
  RX_ASSERT(_backend, "expected valid backend");

  Time::StopWatch time;

  for (Size i = 0; i < sizeof m_resource_usage / sizeof *m_resource_usage; i++) {
    m_resource_usage[i] = 0;
  }

  // Cache the device information from the backend.
  const auto& info{m_backend->query_device_info()};
  m_device_info.vendor = info.vendor;
  m_device_info.renderer = info.renderer;
  m_device_info.version = info.version;

  // Load all modules.
  time.start();
  if (auto directory = Filesystem::Directory::open(m_allocator, MODULES_PATH)) {
    directory->each([this](Filesystem::Directory::Item&& item_) {
      if (item_.is_file() && item_.name().ends_with(".json5")) {
        Module new_module{allocator()};
        if (auto path = item_.full_name(); new_module.load(*path)) {
          m_modules.insert(new_module.name(), Utility::move(new_module));
        }
      }
    });
  }
  time.stop();
  logger->info("Loaded %zu modules in %s", m_modules.size(), time.elapsed());

  // Load all the techniques.
  time.start();
  if (auto directory = Filesystem::Directory::open(m_allocator, TECHNIQUES_PATH)) {
    directory->each([this](Filesystem::Directory::Item&& item_) {
      if (item_.is_file() && item_.name().ends_with(".json5")) {
        Technique new_technique{this};
        if (auto path = item_.full_name(); new_technique.load(*path) && new_technique.compile(m_modules)) {
          m_techniques.insert(new_technique.name(), Utility::move(new_technique));
        }
      }
    });
  }
  time.stop();
  logger->info("Loaded %zu techniques in %s", m_techniques.size(), time.elapsed());

#if 0
  // Load all the routines.
  time.start();
  if (auto directory = Filesystem::Directory::open(m_allocator, ROUTINES_PATH)) {
    directory->each([this](Filesystem::Directory::Item&& item_) {
      Routine new_routine{this};
      if (auto path = item_.full_name(); new_routine.load(*path)) {
        m_routines.insert(new_routine.name(), Utility::move(new_routine));
      }
    });
  }
  time.stop();
  logger->info("Loaded %zu routines in %s", m_routines.size(), time.elapsed());
#endif

  // Generate swapchain target.
  m_swapchain_texture = create_texture2D(RX_RENDER_TAG("swapchain"));
  m_swapchain_texture->record_format(_hdr ? Texture::DataFormat::RGBA_F16 : Texture::DataFormat::RGBA_U8);
  m_swapchain_texture->record_type(Texture::Type::ATTACHMENT);
  m_swapchain_texture->record_levels(1);
  m_swapchain_texture->record_dimensions(_dimensions);
  m_swapchain_texture->record_filter({false, false, false});
  m_swapchain_texture->record_wrap({Texture::WrapType::CLAMP_TO_EDGE,
                                    Texture::WrapType::CLAMP_TO_EDGE});
  m_swapchain_texture->m_flags |= Texture::SWAPCHAIN;
  initialize_texture(RX_RENDER_TAG("swapchain"), m_swapchain_texture);

  m_swapchain_target = create_target(RX_RENDER_TAG("swapchain"));
  m_swapchain_target->attach_texture(m_swapchain_texture, 0);
  m_swapchain_target->m_flags |= Target::SWAPCHAIN;
  initialize_target(RX_RENDER_TAG("swapchain"), m_swapchain_target);
}

Context::~Context() {
  destroy_target(RX_RENDER_TAG("swapchain"), m_swapchain_target);
  destroy_texture(RX_RENDER_TAG("swapchain"), m_swapchain_texture);

  m_cached_buffers.each_value([this](Buffer* _buffer) {
    destroy_buffer(RX_RENDER_TAG("cached buffer"), _buffer);
  });

  m_cached_targets.each_value([this](Target* _target) {
    destroy_target(RX_RENDER_TAG("cached target"), _target);
  });

  m_cached_textures1D.each_value([this](Texture1D* _texture) {
    destroy_texture(RX_RENDER_TAG("cached texture"), _texture);
  });

  m_cached_textures2D.each_value([this](Texture2D* _texture) {
    destroy_texture(RX_RENDER_TAG("cached texture"), _texture);
  });

  m_cached_textures3D.each_value([this](Texture3D* _texture) {
    destroy_texture(RX_RENDER_TAG("cached texture"), _texture);
  });

  m_cached_texturesCM.each_value([this](TextureCM* _texture) {
    destroy_texture(RX_RENDER_TAG("cached texture"), _texture);
  });

  m_arenas.clear();
  // m_routines.clear();
  m_modules.clear();
  m_techniques.clear();

  process();
}

// create_*
Buffer* Context::create_buffer(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::BUFFER;
  command->as_buffer = m_buffer_pool.create<Buffer>(this);
  m_commands.push_back(command_base);
  return command->as_buffer;
}

Target* Context::create_target(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TARGET;
  command->as_target = m_target_pool.create<Target>(this);
  m_commands.push_back(command_base);
  return command->as_target;
}

Program* Context::create_program(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::PROGRAM;
  command->as_program = m_program_pool.create<Program>(this);
  m_commands.push_back(command_base);
  return command->as_program;
}

Texture1D* Context::create_texture1D(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE1D;
  command->as_texture1D = m_texture1D_pool.create<Texture1D>(this);
  m_commands.push_back(command_base);
  return command->as_texture1D;
}

Texture2D* Context::create_texture2D(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE2D;
  command->as_texture2D = m_texture2D_pool.create<Texture2D>(this);
  m_commands.push_back(command_base);
  return command->as_texture2D;
}

Texture3D* Context::create_texture3D(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE3D;
  command->as_texture3D = m_texture3D_pool.create<Texture3D>(this);
  m_commands.push_back(command_base);
  return command->as_texture3D;
}

TextureCM* Context::create_textureCM(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURECM;
  command->as_textureCM = m_textureCM_pool.create<TextureCM>(this);
  m_commands.push_back(command_base);
  return command->as_textureCM;
}

Downloader* Context::create_downloader(const CommandHeader::Info& _info) {
  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_ALLOCATE, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::DOWNLOADER;
  command->as_downloader = m_downloader_pool.create<Downloader>(this);
  m_commands.push_back(command_base);
  return command->as_downloader;
}

// initialize_*
void Context::initialize_buffer(const CommandHeader::Info& _info, Buffer* _buffer) {
  RX_ASSERT(_buffer, "_buffer is null");
  _buffer->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::BUFFER;
  command->as_buffer = _buffer;
  m_commands.push_back(command_base);
  m_footprint[0] += _buffer->resource_usage();
}

void Context::initialize_target(const CommandHeader::Info& _info, Target* _target) {
  RX_ASSERT(_target, "_target is null");
  _target->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TARGET;
  command->as_target = _target;
  m_commands.push_back(command_base);
  m_footprint[0] += _target->resource_usage();
}

void Context::initialize_program(const CommandHeader::Info& _info, Program* _program) {
  RX_ASSERT(_program, "_program is null");
  _program->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::PROGRAM;
  command->as_program = _program;
  m_commands.push_back(command_base);
  m_footprint[0] += _program->resource_usage();
}

void Context::initialize_texture(const CommandHeader::Info& _info, Texture1D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE1D;
  command->as_texture1D = _texture;
  m_commands.push_back(command_base);
  m_footprint[0] += _texture->resource_usage();
}

void Context::initialize_texture(const CommandHeader::Info& _info, Texture2D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE2D;
  command->as_texture2D = _texture;
  m_commands.push_back(command_base);
  m_footprint[0] += _texture->resource_usage();
}

void Context::initialize_texture(const CommandHeader::Info& _info, Texture3D* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURE3D;
  command->as_texture3D = _texture;
  m_commands.push_back(command_base);
  m_footprint[0] += _texture->resource_usage();
}

void Context::initialize_texture(const CommandHeader::Info& _info, TextureCM* _texture) {
  RX_ASSERT(_texture, "_texture is null");
  _texture->validate();

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::TEXTURECM;
  command->as_textureCM = _texture;
  m_commands.push_back(command_base);
  m_footprint[0] += _texture->resource_usage();
}

void Context::initialize_downloader(const CommandHeader::Info& _info, Downloader* _downloader) {
  RX_ASSERT(_downloader, "_downloader is null");

  Concurrency::ScopeLock lock{m_mutex};
  auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_CONSTRUCT, _info);
  auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
  command->type = ResourceCommand::Type::DOWNLOADER;
  command->as_downloader = _downloader;
  m_commands.push_back(command_base);
}

// update_*
void Context::update_buffer(const CommandHeader::Info& _info, Buffer* _buffer) {
  if (_buffer) {
    Concurrency::ScopeLock lock{m_mutex};

    // Keep track of frame footprint.
    m_footprint[0] += _buffer->bytes_for_edits();

    const auto& edits = _buffer->edits();
    if (edits.is_empty()) {
      // Nothing to update.
      return;
    }

    const auto n_edits = edits.size();
    const Size edit_bytes = n_edits * sizeof(Buffer::Edit);

    auto command_base = m_command_buffer.allocate(sizeof(UpdateCommand) + edit_bytes, CommandType::RESOURCE_UPDATE, _info);
    auto command = reinterpret_cast<UpdateCommand*>(command_base + sizeof(CommandHeader));

    command->edits = n_edits;
    command->type = UpdateCommand::Type::BUFFER;
    command->as_buffer = _buffer;
    memcpy(command->edit(), edits.data(), edit_bytes);
    m_commands.push_back(command_base);

    // So we can clear edit list after processing.
    m_edit_buffers.push_back(_buffer);
  }
}

void Context::update_texture(const CommandHeader::Info& _info, Texture1D* _texture) {
  if (_texture) {
    Concurrency::ScopeLock lock{m_mutex};

    // Keep track of frame footprint.
    m_footprint[0] += _texture->bytes_for_edits();

    const auto& edits = _texture->edits();
    if (edits.is_empty()) {
      // Nothing to update.
      return;
    }

    const auto n_edits = edits.size();
    const Size edit_bytes = n_edits * sizeof(Texture::Edit<Texture1D::DimensionType>);

    auto command_base = m_command_buffer.allocate(sizeof(UpdateCommand) + edit_bytes, CommandType::RESOURCE_UPDATE, _info);
    auto command = reinterpret_cast<UpdateCommand*>(command_base + sizeof(CommandHeader));

    command->edits = n_edits;
    command->type = UpdateCommand::Type::TEXTURE1D;
    command->as_texture1D = _texture;
    memcpy(command->edit(), edits.data(), edit_bytes);
    m_commands.push_back(command_base);

    // So we can clear edit list after processing.
    m_edit_textures1D.push_back(_texture);
  }
}

void Context::update_texture(const CommandHeader::Info& _info, Texture2D* _texture) {
  if (_texture) {
    Concurrency::ScopeLock lock{m_mutex};

    // Keep track of frame footprint.
    m_footprint[0] += _texture->bytes_for_edits();

    const auto& edits = _texture->edits();
    if (edits.is_empty()) {
      // Nothing to update.
      return;
    }

    const auto n_edits = edits.size();
    const Size edit_bytes = n_edits * sizeof(Texture::Edit<Texture2D::DimensionType>);

    auto command_base = m_command_buffer.allocate(sizeof(UpdateCommand) + edit_bytes, CommandType::RESOURCE_UPDATE, _info);
    auto command = reinterpret_cast<UpdateCommand*>(command_base + sizeof(CommandHeader));

    command->edits = n_edits;
    command->type = UpdateCommand::Type::TEXTURE2D;
    command->as_texture2D = _texture;
    memcpy(command->edit(), edits.data(), edit_bytes);
    m_commands.push_back(command_base);

    // So we can clear edit list after processing.
    m_edit_textures2D.push_back(_texture);
  }
}

void Context::update_texture(const CommandHeader::Info& _info, Texture3D* _texture) {
  if (_texture) {
    Concurrency::ScopeLock lock{m_mutex};

    // Keep track of frame footprint.
    m_footprint[0] += _texture->bytes_for_edits();

    const auto& edits = _texture->edits();
    if (edits.is_empty()) {
      // Nothing to update.
      return;
    }

    const auto n_edits = edits.size();
    const Size edit_bytes = n_edits * sizeof(Texture::Edit<Texture3D::DimensionType>);

    auto command_base = m_command_buffer.allocate(sizeof(UpdateCommand) + edit_bytes, CommandType::RESOURCE_UPDATE, _info);
    auto command = reinterpret_cast<UpdateCommand*>(command_base + sizeof(CommandHeader));

    command->edits = n_edits;
    command->type = UpdateCommand::Type::TEXTURE3D;
    command->as_texture3D = _texture;
    memcpy(command->edit(), edits.data(), edit_bytes);
    m_commands.push_back(command_base);

    // So we can clear edit list after processing.
    m_edit_textures3D.push_back(_texture);
  }
}

// destroy_*
void Context::destroy_buffer(const CommandHeader::Info& _info, Buffer* _buffer) {
  if (_buffer && _buffer->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    remove_from_cache(m_cached_buffers, _buffer);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::BUFFER;
    command->as_buffer = _buffer;
    m_commands.push_back(command_base);
    m_destroy_buffers.push_back(_buffer);
  }
}

void Context::destroy_target(const CommandHeader::Info& _info, Target* _target) {
  if (_target && _target->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    remove_from_cache(m_cached_targets, _target);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::TARGET;
    command->as_target = _target;
    m_commands.push_back(command_base);
    m_destroy_targets.push_back(_target);

    // Anything owned by the target will also be queued for destruction at this
    // point. Note that |target::destroy| uses unlocked variants of the destroy
    // functions since |lock| here is held and recursive locking of |m_mutex| is
    // not allowed.
    _target->destroy();
  }
}

void Context::destroy_program(const CommandHeader::Info& _info, Program* _program) {
  if (_program && _program->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    // remove_from_cache(m_cached_programs, _program);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::PROGRAM;
    command->as_program = _program;
    m_commands.push_back(command_base);
    m_destroy_programs.push_back(_program);
  }
}

void Context::destroy_texture(const CommandHeader::Info& _info, Texture1D* _texture) {
  if (_texture && _texture->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    remove_from_cache(m_cached_textures1D, _texture);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::TEXTURE1D;
    command->as_texture1D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures1D.push_back(_texture);
  }
}

void Context::destroy_texture(const CommandHeader::Info& _info, Texture2D* _texture) {
  Concurrency::ScopeLock lock{m_mutex};
  destroy_texture_unlocked(_info, _texture);
}

void Context::destroy_texture(const CommandHeader::Info& _info, Texture3D* _texture) {
  if (_texture && _texture->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    remove_from_cache(m_cached_textures3D, _texture);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::TEXTURE3D;
    command->as_texture3D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures3D.push_back(_texture);
  }
}

void Context::destroy_texture(const CommandHeader::Info& _info, TextureCM* _texture) {
  if (_texture && _texture->release_reference()) {
    Concurrency::ScopeLock lock{m_mutex};
    remove_from_cache(m_cached_texturesCM, _texture);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::TEXTURECM;
    command->as_textureCM = _texture;
    m_commands.push_back(command_base);
    m_destroy_texturesCM.push_back(_texture);
  }
}

void Context::destroy_texture_unlocked(const CommandHeader::Info& _info, Texture2D* _texture) {
  if (_texture && _texture->release_reference()) {
    remove_from_cache(m_cached_textures2D, _texture);
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::TEXTURE2D;
    command->as_texture2D = _texture;
    m_commands.push_back(command_base);
    m_destroy_textures2D.push_back(_texture);
  }
}

void Context::destroy_downloader(const CommandHeader::Info& _info, Downloader* _downloader) {
  // NOTE(dweiler): Do not manage a reference count for downloader resources as they're not shareable.
  if (_downloader) {
    Concurrency::ScopeLock lock{m_mutex};
    auto command_base = m_command_buffer.allocate(sizeof(ResourceCommand), CommandType::RESOURCE_DESTROY, _info);
    auto command = reinterpret_cast<ResourceCommand*>(command_base + sizeof(CommandHeader));
    command->type = ResourceCommand::Type::DOWNLOADER;
    command->as_downloader = _downloader;
    m_commands.push_back(command_base);
    m_destroy_downloaders.push_back(_downloader);
  }
}

void Context::draw(
  const CommandHeader::Info& _info,
  const State& _state,
  Target* _target,
  const Buffers& _draw_buffers,
  Buffer* _buffer,
  Program* _program,
  Size _count,
  Size _offset,
  Size _instances,
  Size _base_vertex,
  Size _base_instance,
  PrimitiveType _primitive_type,
  const Textures& _draw_textures)
{
  RX_ASSERT(_state.viewport.dimensions().area() > 0, "empty viewport");

  RX_ASSERT(!_draw_buffers.is_empty(), "missing draw buffers");

  RX_ASSERT(_program, "expected program");
  RX_ASSERT(_count != 0, "empty draw call");

  Size instances = 1;
  if (!_buffer) {
    RX_ASSERT(_offset == 0, "bufferless draws cannot have an offset");
    RX_ASSERT(_instances == 0, "bufferless draws cannot have instances");
    RX_ASSERT(_base_vertex == 0, "bufferless draws cannot have a base vertex");
    RX_ASSERT(_base_instance == 0, "bufferless draws cannot have a base instance");
  } else if (_instances) {
    RX_ASSERT(_buffer->format().is_instanced(), "instanced draw requires instanced buffer");
    instances = _instances;
  }

  if (_buffer && _base_vertex) {
    RX_ASSERT(_buffer->format().is_indexed(), "base vertex draw requires indexed buffer");
  }

  // Check for feedback loops.
  const auto n_draw_textures = _draw_textures.size();
  for (Size i = 0; i < n_draw_textures; i++) {
    RX_ASSERT(!_target->has_feedback(_draw_textures[i], _draw_buffers),
      "draw call forms texture <=> target feedback loop");
  }

  m_vertices[0] += _count * instances;

  switch (_primitive_type) {
  case PrimitiveType::LINES:
    // Each two vertices forms a single line.
    m_lines[0] += (_count / 2) * instances;
    break;
  case PrimitiveType::POINTS:
    // Each vertex forms a point.
    m_points[0] += _count * instances;
    break;
  case PrimitiveType::TRIANGLE_STRIP:
    // One triangle is defined for each vertex presented after the first two
    // vertices, thus N - 2 triangles are formed.
    m_triangles[0] += (_count - 2) * instances;
    break;
  case PrimitiveType::TRIANGLE_FAN:
    // One triangle is defined for each pair of vertices presented after the
    // first vertex, thus N - 2 triangles are formed.
    m_triangles[0] += (_count - 2) * instances;
    break;
  case PrimitiveType::TRIANGLES:
    // Each three vertices form a triangle.
    m_triangles[0] += (_count / 3) * instances;
    break;
  }

  {
    Concurrency::ScopeLock lock{m_mutex};
    const auto dirty_uniforms_size{_program->dirty_uniforms_size()};

    auto command_base{m_command_buffer.allocate(sizeof(DrawCommand) + dirty_uniforms_size, CommandType::DRAW, _info)};
    auto command{reinterpret_cast<DrawCommand*>(command_base + sizeof(CommandHeader))};

    command->draw_buffers = _draw_buffers;
    command->draw_textures = _draw_textures;

    command->render_state = _state;
    command->render_target = _target;
    command->render_buffer = _buffer;
    command->render_program = _program;

    command->count = _count;
    command->offset = _offset;
    command->instances = _instances;
    command->base_vertex = _base_vertex;
    command->base_instance = _base_instance;
    command->type = _primitive_type;
    command->dirty_uniforms_bitset = _program->dirty_uniforms_bitset();

    command->render_state.flush();

    // Copy the uniforms directly into the command.
    if (dirty_uniforms_size) {
      _program->flush_dirty_uniforms(command->uniforms());
      m_footprint[0] += dirty_uniforms_size;
    }

    m_commands.push_back(command_base);
  }

  m_draw_calls[0]++;

  if (_instances) {
    m_instanced_draw_calls[0]++;
  }
}

void Context::clear(const CommandHeader::Info& _info, const State& _state,
  Target* _target, const Buffers& _draw_buffers, Uint32 _clear_mask, ...)
{
  RX_ASSERT(_state.viewport.dimensions().area() > 0, "empty viewport");

  RX_ASSERT(_target, "expected target");
  RX_ASSERT(!_draw_buffers.is_empty(), "expected draw buffers");
  RX_ASSERT(_clear_mask != 0, "empty clear");

  const bool clear_depth{!!(_clear_mask & RX_RENDER_CLEAR_DEPTH)};
  const bool clear_stencil{!!(_clear_mask & RX_RENDER_CLEAR_STENCIL)};

  _clear_mask >>= 2;

  {
    Concurrency::ScopeLock lock{m_mutex};

    auto command_base = m_command_buffer.allocate(sizeof(ClearCommand), CommandType::CLEAR, _info);
    auto command = reinterpret_cast<ClearCommand*>(command_base + sizeof(CommandHeader));

    command->render_state = _state;
    command->render_target = _target;
    command->clear_depth = clear_depth;
    command->clear_stencil = clear_stencil;
    command->clear_colors = _clear_mask;
    command->draw_buffers = _draw_buffers;

    command->render_state.flush();

    // Decode and copy the clear values into the command.
    va_list va;
    va_start(va, _clear_mask);
    if (clear_depth) {
      command->depth_value = static_cast<Float32>(va_arg(va, Float64));
    }

    if (clear_stencil) {
      command->stencil_value = va_arg(va, Sint32);
    }

    for (Uint32 i{0}; i < Buffers::MAX_BUFFERS; i++) {
      if (_clear_mask & (1 << i)) {
        const Float32* color{va_arg(va, Float32*)};
        command->color_values[i].r = color[0];
        command->color_values[i].g = color[1];
        command->color_values[i].b = color[2];
        command->color_values[i].a = color[3];
      }
    }
    va_end(va);

    m_commands.push_back(command_base);
  }

  m_clear_calls[0]++;
}

void Context::blit(
  const CommandHeader::Info& _info,
  const State& _state,
  Target* _src_target,
  Size _src_attachment,
  Target* _dst_target,
  Size _dst_attachment)
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

  using Attachment = Target::Attachment::Type;
  RX_ASSERT(src_attachments[_src_attachment].kind == Attachment::TEXTURE2D,
    "source attachment not a 2D texture");
  RX_ASSERT(dst_attachments[_dst_attachment].kind == Attachment::TEXTURE2D,
    "destination attachment not a 2D texture");

  Texture2D* src_attachment{src_attachments[_src_attachment].as_texture2D.texture};
  Texture2D* dst_attachment{dst_attachments[_dst_attachment].as_texture2D.texture};

  // It's possible for targets to be configured in a way where attachments are
  // shared between them. Blitting to and from the same attachment doesn't make
  // any sense.
  RX_ASSERT(src_attachment != dst_attachment, "cannot blit to self");

  // It's only valid to blit color attachments.
  RX_ASSERT(src_attachment->is_color_format(),
    "cannot blit with non-color source attachment");
  RX_ASSERT(dst_attachment->is_color_format(),
    "cannot blit with non-color destination attachment");

  const auto is_float_color = [](Texture::DataFormat _format) {
    return _format == Texture::DataFormat::BGRA_F16 ||
           _format == Texture::DataFormat::RGBA_F16;
  };

  // A blit from one target to another is only valid if the source and
  // destination attachments contain similar data formats. That is they both
  // must use floating-point attachments or integer attachments. Mixing is
  // not allowed.
  RX_ASSERT(is_float_color(src_attachment->format()) == is_float_color(dst_attachment->format()),
    "incompatible formats between attachments");

  {
    Concurrency::ScopeLock lock{m_mutex};

    auto command_base = m_command_buffer.allocate(sizeof(BlitCommand), CommandType::BLIT, _info);
    auto command = reinterpret_cast<BlitCommand*>(command_base + sizeof(CommandHeader));

    command->render_state = _state;
    command->src_target = _src_target;
    command->src_attachment = _src_attachment;
    command->dst_target = _dst_target;
    command->dst_attachment = _dst_attachment;

    command->render_state.flush();

    m_commands.push_back(command_base);
  }

  m_blit_calls[0]++;
}

void Context::download(
  const CommandHeader::Info& _info,
  Target* _src_target,
  Size _src_attachment,
  const Math::Vec2z& _offset,
  Downloader* _downloader)
{
  Concurrency::ScopeLock lock{m_mutex};

  auto command_base = m_command_buffer.allocate(sizeof(DownloadCommand), CommandType::DOWNLOAD, _info);
  auto command = reinterpret_cast<DownloadCommand*>(command_base + sizeof(CommandHeader));

  command->src_target = _src_target;
  command->src_attachment = _src_attachment;
  command->offset = _offset;
  command->downloader = _downloader;

  m_commands.push_back(command_base);
}

void Context::profile(const char* _tag) {
  Concurrency::ScopeLock lock{m_mutex};

  auto command_base = m_command_buffer.allocate(sizeof(ProfileCommand), CommandType::PROFILE, RX_RENDER_TAG("profile"));
  auto command = reinterpret_cast<ProfileCommand*>(command_base + sizeof(CommandHeader));

  command->tag = _tag;

  m_commands.push_back(command_base);
}

void Context::resize(const Math::Vec2z& _resolution) {
  // Resizing the swapchain is just a matter of updating these fields.
  m_swapchain_texture->m_dimensions = _resolution;
  m_swapchain_target->m_dimensions = _resolution;
}

bool Context::process() {
  RX_PROFILE_CPU("process");

  if (m_commands.is_empty()) {
    return false;
  }

  m_commands_recorded[0] = m_commands.size();

  {
    Concurrency::ScopeLock lock{m_mutex};

    // Consume all recorded commands on the backend.
    m_backend->process(m_commands);

    // Clear edit lists
    m_edit_buffers.each_fwd([](Buffer* _buffer) { _buffer->clear_edits(); });
    m_edit_textures1D.each_fwd([](Texture1D* _texture) { _texture->clear_edits(); });
    m_edit_textures2D.each_fwd([](Texture2D* _texture) { _texture->clear_edits(); });
    m_edit_textures3D.each_fwd([](Texture3D* _texture) { _texture->clear_edits(); });

    // Cleanup unreferenced frontend resources.
    m_destroy_buffers.each_fwd([this](Buffer* _buffer) { m_buffer_pool.destroy<Buffer>(_buffer); });
    m_destroy_targets.each_fwd([this](Target* _target) { m_target_pool.destroy<Target>(_target); });
    m_destroy_programs.each_fwd([this](Program* _program) { m_program_pool.destroy<Program>(_program); });
    m_destroy_textures1D.each_fwd([this](Texture1D* _texture) { m_texture1D_pool.destroy<Texture1D>(_texture); });
    m_destroy_textures2D.each_fwd([this](Texture2D* _texture) { m_texture2D_pool.destroy<Texture2D>(_texture); });
    m_destroy_textures3D.each_fwd([this](Texture3D* _texture) { m_texture3D_pool.destroy<Texture3D>(_texture); });
    m_destroy_texturesCM.each_fwd([this](TextureCM* _texture) { m_textureCM_pool.destroy<TextureCM>(_texture); });
    m_destroy_downloaders.each_fwd([this](Downloader* _downloader) { m_downloader_pool.destroy<Downloader>(_downloader); });

    // Reset the command buffer.
    m_commands.clear();
    m_command_buffer.reset();

    // Cleanup edit lists.
    m_edit_buffers.clear();
    m_edit_textures1D.clear();
    m_edit_textures2D.clear();
    m_edit_textures3D.clear();

    // Cleanup destroyed resources list.
    m_destroy_buffers.clear();
    m_destroy_targets.clear();
    m_destroy_programs.clear();
    m_destroy_textures1D.clear();
    m_destroy_textures2D.clear();
    m_destroy_textures3D.clear();
    m_destroy_texturesCM.clear();
    m_destroy_downloaders.clear();
  }

  // Update all rendering stats for the last frame.
  auto swap = [](Concurrency::Atomic<Size> (&value_)[2]) { value_[1] = value_[0].exchange(0); };

  swap(m_draw_calls);
  swap(m_instanced_draw_calls);
  swap(m_clear_calls);
  swap(m_blit_calls);
  swap(m_vertices);
  swap(m_points);
  swap(m_lines);
  swap(m_triangles);
  swap(m_commands_recorded);
  swap(m_footprint);

  return true;
}

Context::Statistics Context::stats(Resource::Type _type) const {
  Concurrency::ScopeLock lock(m_mutex);

  const auto index{static_cast<Size>(_type)};
  switch (_type) {
  case Resource::Type::BUFFER:
    return {m_buffer_pool.capacity(), m_buffer_pool.size(), m_cached_buffers.size(), m_resource_usage[index]};
  case Resource::Type::PROGRAM:
    return {m_program_pool.capacity(), m_program_pool.size(), 0, m_resource_usage[index]};
  case Resource::Type::TARGET:
    return {m_target_pool.capacity(), m_target_pool.size(), m_cached_targets.size(), m_resource_usage[index]};
  case Resource::Type::TEXTURE1D:
    return {m_texture1D_pool.capacity(), m_texture1D_pool.size(), m_cached_textures1D.size(), m_resource_usage[index]};
  case Resource::Type::TEXTURE2D:
    return {m_texture2D_pool.capacity(), m_texture2D_pool.size(), m_cached_textures2D.size(), m_resource_usage[index]};
  case Resource::Type::TEXTURE3D:
    return {m_texture3D_pool.capacity(), m_texture3D_pool.size(), m_cached_textures3D.size(), m_resource_usage[index]};
  case Resource::Type::TEXTURECM:
    return {m_textureCM_pool.capacity(), m_textureCM_pool.size(), m_cached_texturesCM.size(), m_resource_usage[index]};
  case Resource::Type::DOWNLOADER:
    return {m_downloader_pool.capacity(), m_downloader_pool.size(), 0, m_resource_usage[index]};
  }

  RX_HINT_UNREACHABLE();
}

bool Context::swap() {
  RX_PROFILE_CPU("swap");

  m_backend->swap();

  m_frame++;

  return m_timer.update();
}

Buffer* Context::cached_buffer(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find = m_cached_buffers.find(_key)) {
    auto result = *find;
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

Target* Context::cached_target(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find{m_cached_targets.find(_key)}) {
    auto result{*find};
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

Texture1D* Context::cached_texture1D(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find{m_cached_textures1D.find(_key)}) {
    auto result{*find};
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

Texture2D* Context::cached_texture2D(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find{m_cached_textures2D.find(_key)}) {
    auto result{*find};
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

Texture3D* Context::cached_texture3D(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find{m_cached_textures3D.find(_key)}) {
    auto result{*find};
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

TextureCM* Context::cached_textureCM(const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find{m_cached_texturesCM.find(_key)}) {
    auto result{*find};
    result->acquire_reference();
    return result;
  }
  return nullptr;
}

bool Context::cache_buffer(Buffer* _buffer, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_buffers.insert(_key, _buffer) != nullptr;
}

bool Context::cache_target(Target* _target, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_targets.insert(_key, _target) != nullptr;
}

bool Context::cache_texture(Texture1D* _texture, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_textures1D.insert(_key, _texture) != nullptr;
}

bool Context::cache_texture(Texture2D* _texture, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_textures2D.insert(_key, _texture) != nullptr;
}

bool Context::cache_texture(Texture3D* _texture, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_textures3D.insert(_key, _texture) != nullptr;
}

bool Context::cache_texture(TextureCM* _texture, const String& _key) {
  Concurrency::ScopeLock lock{m_mutex};
  return m_cached_texturesCM.insert(_key, _texture) != nullptr;
}

Technique* Context::find_technique_by_name(const char* _name) {
  return m_techniques.find(_name);
}

Arena* Context::arena(const Buffer::Format& _format) {
  // Check if an arena for this buffer |_format| already exists.
  Concurrency::ScopeLock lock{m_mutex};
  if (auto find = m_arenas.find(_format)) {
    return find;
  }

  // Create a new arena for this buffer |_format|.
  //
  // Unlock the mutex here because |Arena::Arena| will need to acquire.
  m_mutex.unlock();
  Frontend::Arena arena{this, _format};
  m_mutex.lock();

  return m_arenas.insert(_format, Utility::move(arena));
}

} // namespace Rx::Render::Frontend
