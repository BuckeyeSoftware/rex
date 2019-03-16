#include <SDL.h> // SDL_GetTicks

#include <rx/render/timer.h>

namespace rx::render {

frame_timer::frame_timer()
  : m_max_frame_ticks{0.0f}
  , m_last_second_ticks{0}
  , m_frame_count{0}
  , m_min_ticks{0}
  , m_max_ticks{0}
  , m_average_ticks{0.0f}
  , m_delta_time{0.0f}
  , m_last_frame_ticks{0}
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
  m_max_frame_ticks = _max_fps <= 0.0f ? -1.0f : (1000.0f / _max_fps) - k_dampen;
}

void frame_timer::reset() {
  m_frame_count = 0;
  m_min_ticks = 1000;
  m_max_ticks = 0;
  m_average_ticks = 0.0f;
  m_last_second_ticks = SDL_GetTicks();
}

bool frame_timer::update() {
  m_frame_count++;
  m_target_ticks = m_max_frame_ticks != -1.0f
    ? m_last_second_ticks + rx_u32(m_frame_count * m_max_frame_ticks) : 0;
  m_current_ticks = SDL_GetTicks();
  m_average_ticks += m_current_ticks - m_last_frame_ticks;

  if (m_current_ticks - m_last_frame_ticks <= m_min_ticks) {
    m_min_ticks = m_current_ticks - m_last_frame_ticks;
  }

  if (m_current_ticks - m_last_frame_ticks >= m_max_ticks) {
    m_max_ticks = m_current_ticks - m_last_frame_ticks;
  }

  if (m_target_ticks && m_current_ticks < m_target_ticks) {
    const rx_u32 ticks_before_delay{SDL_GetTicks()};
    SDL_Delay(m_target_ticks - m_current_ticks);
    m_current_ticks = SDL_GetTicks();
    m_average_ticks += m_current_ticks - ticks_before_delay;
  }

  m_delta_time = 0.001f * (m_current_ticks - m_last_frame_ticks);
  m_last_frame_ticks = m_current_ticks;

  if (m_current_ticks - m_last_second_ticks >= 1000) {
    m_frames_per_second = m_frame_count;
    m_frame_average = m_average_ticks / m_frame_count;
    m_frame_min = m_min_ticks;
    m_frame_max = m_max_ticks;

    reset();

    return true;
  }

  return false;
}

} // namespace rx::render