#include "rx/render/frontend/timer.h"

#include "rx/core/math/abs.h"

#include "rx/core/time/qpc.h"
#include "rx/core/time/delay.h"

namespace rx::render::frontend {

frame_timer::frame_timer()
  : m_frequency{time::qpc_frequency()}
  , m_resolution{1.0 / m_frequency}
  , m_max_frame_ticks{0.0f}
  , m_last_second_ticks{0}
  , m_frame_count{0}
  , m_min_ticks{0}
  , m_max_ticks{0}
  , m_average_ticks{0.0f}
  , m_delta_time{0.0f}
  , m_last_frame_ticks{time::qpc_ticks()}
  , m_current_ticks{0}
  , m_target_ticks{0}
  , m_frame_min{0}
  , m_frame_max{0}
  , m_frame_average{0.0f}
  , m_frames_per_second{0}
{
}

void frame_timer::cap_fps(rx_f32 _max_fps) {
  static constexpr const rx_f32 k_dampen{0.00001f};
  m_max_frame_ticks = _max_fps <= 0.0f ? -1.0f : (m_frequency / _max_fps) - k_dampen;
}

void frame_timer::reset() {
  m_last_second_ticks = time::qpc_ticks();
  m_frequency = time::qpc_frequency();
  m_resolution = 1.0 / m_frequency;
  m_frame_count = 0;
  m_min_ticks = m_frequency;
  m_max_ticks = 0;
  m_average_ticks = 0.0;
}

bool frame_timer::update() {
  m_frame_count++;
  m_target_ticks = m_max_frame_ticks != -1.0f
    ? m_last_second_ticks + rx_u64(m_frame_count * m_max_frame_ticks) : 0;
  m_current_ticks = time::qpc_ticks();
  m_average_ticks += m_current_ticks - m_last_frame_ticks;

  const rx_f64 life_time{m_current_ticks * m_resolution};
  const rx_f64 frame_time{(life_time - (m_last_frame_ticks * m_resolution)) * 1000.0};
  m_frame_times.push_back({life_time, frame_time});

  // Erase old frame times. We only want a window that is |k_frame_history_seconds|
  // in size.
  for (rx_size i{0}; i < m_frame_times.size(); i++) {
    if (m_frame_times[i].life >= life_time - k_frame_history_seconds) {
      if (i != 0) {
        m_frame_times.erase(0, i);
      }
      break;
    }
  }

  if (m_current_ticks - m_last_frame_ticks <= m_min_ticks) {
    m_min_ticks = m_current_ticks - m_last_frame_ticks;
  }

  if (m_current_ticks - m_last_frame_ticks >= m_max_ticks) {
    m_max_ticks = m_current_ticks - m_last_frame_ticks;
  }

  if (m_target_ticks && m_current_ticks < m_target_ticks) {
    const auto ticks_before_delay{time::qpc_ticks()};
    time::delay(static_cast<rx_u64>((m_target_ticks - m_current_ticks) * 1000) / m_frequency);
    m_current_ticks = time::qpc_ticks();
    m_average_ticks += m_current_ticks - ticks_before_delay;
  }

  const rx_f64 delta{m_resolution * (m_current_ticks - m_last_frame_ticks)};
  m_delta_time = delta;
  m_last_frame_ticks = m_current_ticks;

  if (m_current_ticks - m_last_second_ticks >= m_frequency) {
    m_frames_per_second = static_cast<rx_u32>(m_frame_count);
    m_frame_average = static_cast<rx_f32>((m_resolution * m_average_ticks / m_frame_count) * 1000.0);
    m_frame_min = m_min_ticks;
    m_frame_max = m_max_ticks;

    reset();

    return true;
  }

  return false;
}

} // namespace rx::render::frontend
