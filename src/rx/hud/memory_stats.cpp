#include "rx/hud/memory_stats.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/variable.h"

namespace rx::hud {

RX_CONSOLE_SVAR(
  font_name,
  "hud.memory_stats.font_name",
  "font name of memory stats hud",
  "Inconsolata-Regular");

RX_CONSOLE_IVAR(
  font_size,
  "hud.memory_stats.font_size",
  "font size of memory stats hud",
  16,
  64,
  25);

memory_stats::memory_stats(render::immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void memory_stats::render() {
  const auto allocator = &memory::system_allocator::instance();
  const auto stats = static_cast<const memory::system_allocator*>(allocator)->stats();

  const render::frontend::context& frontend = *m_immediate->frontend();
  const math::vec2f &screen_size = frontend.swapchain()->dimensions().cast<rx_f32>();

  rx_f32 y = 25.0f;
  auto line{[&](const string &_line) {
    m_immediate->frame_queue().record_text(
      *font_name,
      math::vec2f{screen_size.x - 25.0f, y},
      *font_size,
      1.0f,
      render::immediate2D::text_align::k_right,
      _line,
      {1.0f, 1.0f, 1.0f, 1.0f});
    y += *font_size;
  }};

  line(string::format("used memory (requested): %s", string::human_size_format(stats.used_request_bytes)));
  line(string::format("used memory (actual):    %s", string::human_size_format(stats.used_actual_bytes)));
  line(string::format("peak memory (requested): %s", string::human_size_format(stats.peak_request_bytes)));
  line(string::format("peak memory (actual):    %s", string::human_size_format(stats.peak_actual_bytes)));
}

} // namespace rx::hud
