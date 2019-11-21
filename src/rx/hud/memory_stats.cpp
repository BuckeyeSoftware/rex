#include "rx/hud/memory_stats.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

namespace rx::hud {

memory_stats::memory_stats(render::immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void memory_stats::render() {
  const auto stats{memory::g_system_allocator->stats()};
  const render::frontend::interface& frontend{*m_immediate->frontend()};
  const math::vec2f &screen_size{frontend.swapchain()->dimensions().cast<rx_f32>()};

  rx_f32 y = 25.0f;
  auto line{[&](const string &_line) {
    m_immediate->frame_queue().record_text(
      "Consolas-Regular",
      math::vec2f{screen_size.x - 25.0f, y},
      16,
      1.0f,
      render::immediate2D::text_align::k_right,
      _line,
      {1.0f, 1.0f, 1.0f, 1.0f});
    y += 16.0f;
  }};

  line(string::format("used memory (requested): %s", string::human_size_format(stats.used_request_bytes)));
  line(string::format("used memory (actual):    %s", string::human_size_format(stats.used_actual_bytes)));
  line(string::format("peak memory (requested): %s", string::human_size_format(stats.peak_request_bytes)));
  line(string::format("peak memory (actual):    %s", string::human_size_format(stats.peak_actual_bytes)));
}

} // namespace rx::hud
