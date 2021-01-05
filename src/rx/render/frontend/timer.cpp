#include "rx/render/frontend/timer.h"

#include "rx/core/math/abs.h"

#include "rx/core/time/qpc.h"
#include "rx/core/time/delay.h"

namespace Rx::Render::Frontend {

FrameTimer::FrameTimer()
  : m_frequency{Time::qpc_frequency()}
  , m_resolution{1.0 / m_frequency}
  , m_max_frame_ticks{0.0f}
  , m_last_second_ticks{0}
  , m_frame_count{0}
  , m_min_ticks{0}
  , m_max_ticks{0}
  , m_average_ticks{0.0f}
  , m_delta_time{0.0f}
  , m_last_frame_ticks{Time::qpc_ticks()}
  , m_current_ticks{0}
  , m_target_ticks{0}
  , m_frame_min{0}
  , m_frame_max{0}
  , m_frame_average{0.0f}
  , m_frames_per_second{0}
{
}

void FrameTimer::cap_fps(Float32 _max_fps) {
  static constexpr const Float32 DAMPEN = 0.00001f;
  m_max_frame_ticks = _max_fps <= 0.0f ? -1.0f : (m_frequency / _max_fps) - DAMPEN;
}

void FrameTimer::reset() {
  m_last_second_ticks = Time::qpc_ticks();
  m_frequency = Time::qpc_frequency();
  m_resolution = 1.0 / m_frequency;
  m_frame_count = 0;
  m_min_ticks = m_frequency;
  m_max_ticks = 0;
  m_average_ticks = 0.0;
}

bool FrameTimer::update() {
  m_frame_count++;
  m_target_ticks = m_max_frame_ticks != -1.0f
                   ? m_last_second_ticks + Uint64(m_frame_count * m_max_frame_ticks) : 0;
  m_current_ticks = Time::qpc_ticks();
  m_average_ticks += m_current_ticks - m_last_frame_ticks;

  const Float64 life_time{m_current_ticks * m_resolution};
  const Float64 frame_time{(life_time - (m_last_frame_ticks * m_resolution)) * 1000.0};
  m_frame_times.emplace_back(life_time, frame_time);

  // Erase old frame times. We only want a window that is |HISTORY_SECONDS|
  // in size.
  for (Size i{0}; i < m_frame_times.size(); i++) {
    if (m_frame_times[i].life >= life_time - HISTORY_SECONDS) {
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
    const auto ticks_before_delay{Time::qpc_ticks()};
    Time::delay(static_cast<Uint64>((m_target_ticks - m_current_ticks) * 1000) / m_frequency);
    m_current_ticks = Time::qpc_ticks();
    m_average_ticks += m_current_ticks - ticks_before_delay;
  }

  const Float64 delta{m_resolution * (m_current_ticks - m_last_frame_ticks)};
  m_delta_time = delta;
  m_last_frame_ticks = m_current_ticks;

  if (m_current_ticks - m_last_second_ticks >= m_frequency) {
    m_frames_per_second = static_cast<Uint32>(m_frame_count);
    m_frame_average = static_cast<Float32>((m_resolution * m_average_ticks / m_frame_count) * 1000.0);
    m_frame_min = m_min_ticks;
    m_frame_max = m_max_ticks;

    reset();

    return true;
  }

  return false;
}

} // namespace Rx::Render::Frontend
