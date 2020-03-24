#include "rx/hud/render_stats.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/variable.h"

namespace rx::hud {

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

render_stats::render_stats(render::immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void render_stats::render() {
  const render::frontend::context& frontend{*m_immediate->frontend()};
  const auto &buffer_stats{frontend.stats(render::frontend::resource::type::k_buffer)};
  const auto &program_stats{frontend.stats(render::frontend::resource::type::k_program)};
  const auto &target_stats{frontend.stats(render::frontend::resource::type::k_target)};
  const auto &texture1D_stats{frontend.stats(render::frontend::resource::type::k_texture1D)};
  const auto &texture2D_stats{frontend.stats(render::frontend::resource::type::k_texture2D)};
  const auto &texture3D_stats{frontend.stats(render::frontend::resource::type::k_texture3D)};
  const auto &textureCM_stats{frontend.stats(render::frontend::resource::type::k_textureCM)};

  math::vec2f offset{25.0f, 25.0f};

  auto color_ratio{[](rx_size _used, rx_size _total) -> rx_u32 {
    const math::vec3f bad{1.0f, 0.0f, 0.0f};
    const math::vec3f good{0.0f, 1.0f, 0.0f};
    const rx_f32 scaled{static_cast<rx_f32>(_used) / static_cast<rx_f32>(_total)};
    const math::vec3f color{bad * scaled + good * (1.0f - scaled)};
    return (rx_u32(color.r * 255.0f) << 24) |
           (rx_u32(color.g * 255.0f) << 16) |
           (rx_u32(color.b * 255.0f) << 8) |
           0xFF;
  }};

  auto render_stat{[&](const char *_label, const auto &_stats) {
    const auto format{
      string::format(
        "^w%s: ^[%x]%zu ^wof ^m%zu ^g%s ^w(%zu cached)",
        _label,
        color_ratio(_stats.used, _stats.total),
        _stats.used,
        _stats.total,
        string::human_size_format(_stats.memory),
        _stats.cached)};

    m_immediate->frame_queue().record_text(
      *font_name,
      offset,
      *font_size,
      1.0f,
      render::immediate2D::text_align::k_left,
      format,
      {1.0f, 1.0f, 1.0f, 1.0f});

    offset.y += *font_size;
  }};

  const auto &command_buffer{frontend.get_command_buffer()};
  const rx_size commands_used{command_buffer.used()};
  const rx_size commands_total{command_buffer.size()};
  m_immediate->frame_queue().record_text(
    *font_name,
    offset,
    *font_size,
    1.0f,
    render::immediate2D::text_align::k_left,
    string::format(
      "commands: ^[%x]%s ^wof ^g%s",
      color_ratio(commands_used, commands_total),
      string::human_size_format(commands_used),
      string::human_size_format(commands_total)),
    {1.0f, 1.0f, 1.0f, 1.0f});

  offset.y += *font_size;

  render_stat("texturesCM", textureCM_stats);
  render_stat("textures3D", texture3D_stats);
  render_stat("textures2D", texture2D_stats);
  render_stat("textures1D", texture1D_stats);
  render_stat("programs", program_stats);
  render_stat("buffers", buffer_stats);
  render_stat("targets", target_stats);

  auto render_number{[&](const char* _name, rx_size _number) {
    m_immediate->frame_queue().record_text(
      *font_name,
      offset,
      *font_size,
      1.0f,
      render::immediate2D::text_align::k_left,
      string::format("%s: %zu", _name, _number),
      {1.0f, 1.0f, 1.0f, 1.0f});
    offset.y += *font_size;
  }};

  render_number("points", frontend.points());
  render_number("lines", frontend.lines());
  render_number("triangles", frontend.triangles());
  render_number("vertices", frontend.vertices());
  render_number("blits", frontend.blit_calls());
  render_number("clears", frontend.clear_calls());
  render_number("draws", frontend.draw_calls());

  const math::vec2f &screen_size{frontend.swapchain()->dimensions().cast<rx_f32>()};

  // mspf and fps
  const auto& _timer{frontend.timer()};
  m_immediate->frame_queue().record_text(
    *font_name,
    screen_size - math::vec2f{25, 25},
    *font_size,
    1.0f,
    render::immediate2D::text_align::k_right,
    string::format(
      "MSPF: %.2f | FPS: %d",
      _timer.mspf(),
      _timer.fps()),
    {1.0f, 1.0f, 1.0f, 1.0f});
}

} // namespace rx::hud
