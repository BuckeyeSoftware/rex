#include "rx/hud/memory_stats.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/variable.h"

namespace Rx::hud {

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

MemoryStats::MemoryStats(Render::Immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void MemoryStats::render() {
  const auto allocator = &Memory::SystemAllocator::instance();
  const auto stats = static_cast<const Memory::SystemAllocator*>(allocator)->stats();

  const Render::Frontend::Context& frontend = *m_immediate->frontend();
  const Math::Vec2f &screen_size = frontend.swapchain()->dimensions().cast<Float32>();

  Float32 y = 25.0f;
  auto line{[&](const String &_line) {
    m_immediate->frame_queue().record_text(
            *font_name,
            Math::Vec2f{screen_size.x - 25.0f, y},
            *font_size,
            1.0f,
            Render::Immediate2D::TextAlign::RIGHT,
            _line,
            {1.0f, 1.0f, 1.0f, 1.0f});
    y += *font_size;
  }};

  line(String::format("used memory (requested): %s", String::human_size_format(stats.used_request_bytes)));
  line(String::format("used memory (actual):    %s", String::human_size_format(stats.used_actual_bytes)));
  line(String::format("peak memory (requested): %s", String::human_size_format(stats.peak_request_bytes)));
  line(String::format("peak memory (actual):    %s", String::human_size_format(stats.peak_actual_bytes)));
}

} // namespace rx::hud
