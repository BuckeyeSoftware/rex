#include "rx/hud/render_stats.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/variable.h"

namespace Rx::hud {

RX_CONSOLE_SVAR(
  font_name,
  "hud.render_stats.font_name",
  "font name of render stats hud",
  "Inconsolata-Regular");

RX_CONSOLE_IVAR(
  font_size,
  "hud.render_stats.font_size",
  "font size of render stats hud",
  16,
  64,
  25);

RenderStats::RenderStats(Render::Immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void RenderStats::render() {
  auto& allocator = Memory::SystemAllocator::instance();

  const Render::Frontend::Context& frontend = *m_immediate->frontend();
  const auto& buffer_stats = frontend.stats(Render::Frontend::Resource::Type::BUFFER);
  const auto& program_stats = frontend.stats(Render::Frontend::Resource::Type::PROGRAM);
  const auto& target_stats = frontend.stats(Render::Frontend::Resource::Type::TARGET);
  const auto& texture1D_stats = frontend.stats(Render::Frontend::Resource::Type::TEXTURE1D);
  const auto& texture2D_stats = frontend.stats(Render::Frontend::Resource::Type::TEXTURE2D);
  const auto& texture3D_stats = frontend.stats(Render::Frontend::Resource::Type::TEXTURE3D);
  const auto& textureCM_stats = frontend.stats(Render::Frontend::Resource::Type::TEXTURECM);
  const auto& downloader_stats = frontend.stats(Render::Frontend::Resource::Type::DOWNLOADER);

  Math::Vec2f offset{25.0f, 25.0f};

  auto color_ratio = [](Size _used, Size _total) -> Uint32 {
    const Math::Vec3f bad{1.0f, 0.0f, 0.0f};
    const Math::Vec3f good{0.0f, 1.0f, 0.0f};
    const Float32 scaled{static_cast<Float32>(_used) / static_cast<Float32>(_total)};
    const Math::Vec3f color{bad * scaled + good * (1.0f - scaled)};
    return (Uint32(color.r * 255.0f) << 24) |
           (Uint32(color.g * 255.0f) << 16) |
           (Uint32(color.b * 255.0f) << 8) |
           0xFF;
  };

  auto render_stat = [&](const char *_label, const auto &_stats) {
    const auto format =
      String::format(
        allocator,
        "^w%s: ^[%x]%zu ^wof ^m%zu ^g%s ^w(%zu cached)",
        _label,
        color_ratio(_stats.used, _stats.total),
        _stats.used,
        _stats.total,
        String::human_size_format(_stats.memory),
        _stats.cached);

    m_immediate->frame_queue().record_text(
      *font_name,
      offset,
      *font_size,
      1.0f,
      Render::Immediate2D::TextAlign::LEFT,
      format,
      {1.0f, 1.0f, 1.0f, 1.0f});

    offset.y += *font_size;
  };

  const auto &command_buffer = frontend.get_command_buffer();
  const Size commands_used = command_buffer.used();
  const Size commands_total = command_buffer.size();
  m_immediate->frame_queue().record_text(
    *font_name,
    offset,
    *font_size,
    1.0f,
    Render::Immediate2D::TextAlign::LEFT,
    String::format(
      allocator,
      "commands: ^[%x]%s ^wof ^g%s ^w(%zu total)",
      color_ratio(commands_used, commands_total),
      String::human_size_format(commands_used),
      String::human_size_format(commands_total),
      frontend.commands()),
    {1.0f, 1.0f, 1.0f, 1.0f});

  offset.y += *font_size;

  render_stat("downloaders", downloader_stats);
  render_stat("texturesCM", textureCM_stats);
  render_stat("textures3D", texture3D_stats);
  render_stat("textures2D", texture2D_stats);
  render_stat("textures1D", texture1D_stats);
  render_stat("programs", program_stats);
  render_stat("buffers", buffer_stats);
  render_stat("targets", target_stats);

  auto render_number = [&](const char* _name, Size _number) {
    m_immediate->frame_queue().record_text(
      *font_name,
      offset,
      *font_size,
      1.0f,
      Render::Immediate2D::TextAlign::LEFT,
      String::format(allocator, "%s: %zu", _name, _number),
      {1.0f, 1.0f, 1.0f, 1.0f});
    offset.y += *font_size;
  };

  render_number("points", frontend.points());
  render_number("lines", frontend.lines());
  render_number("triangles", frontend.triangles());
  render_number("vertices", frontend.vertices());
  render_number("blits", frontend.blit_calls());
  render_number("clears", frontend.clear_calls());

  m_immediate->frame_queue().record_text(
    *font_name,
    offset,
    *font_size,
    1.0f,
    Render::Immediate2D::TextAlign::LEFT,
    String::format(
      allocator,
      "draws: %zu (%zu instanced)",
      frontend.draw_calls(),
      frontend.instanced_draw_calls()),
    {1.0f, 1.0f, 1.0f, 1.0f});
  offset.y += *font_size;

  const Math::Vec2f& screen_size =
    frontend.swapchain()->dimensions().cast<Float32>();

  // mspf and fps
  const auto& _timer = frontend.timer();
  m_immediate->frame_queue().record_text(
    *font_name,
    screen_size - Math::Vec2f{25, 25},
    *font_size,
    1.0f,
    Render::Immediate2D::TextAlign::RIGHT,
    String::format(
      allocator,
      "MSPF: %.2f | FPS: %d",
      _timer.mspf(),
      _timer.fps()),
    {1.0f, 1.0f, 1.0f, 1.0f});
}

} // namespace rx::hud
