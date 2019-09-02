#ifndef RX_RENDER_FRONTEND_TIMER_H
#define RX_RENDER_FRONTEND_TIMER_H
#include "rx/core/vector.h"

namespace rx::render::frontend {

struct frame_timer {
  static constexpr rx_f64 k_frame_history_seconds{2.0};

  frame_timer();

  rx_f32 mspf() const;
  rx_u32 fps() const;
  rx_f32 delta_time() const;
  rx_f64 resolution() const;
  rx_u64 ticks() const;

  void cap_fps(rx_f32 _max_fps);

  void reset();
  bool update();

  struct frame_time {
    rx_f64 life;
    rx_f64 frame;
  };

  const vector<frame_time>& frame_times() const &;

private:
  rx_u64 m_frequency;
  rx_f64 m_resolution;
  rx_f32 m_max_frame_ticks;
  rx_u64 m_last_second_ticks;
  rx_u64 m_frame_count;
  rx_u64 m_min_ticks;
  rx_u64 m_max_ticks;
  rx_f64 m_average_ticks;
  rx_f64 m_delta_time;
  rx_u64 m_last_frame_ticks;
  rx_u64 m_current_ticks;
  rx_u64 m_target_ticks;
  rx_u64 m_frame_min;
  rx_u64 m_frame_max;
  rx_f32 m_frame_average;
  rx_u32 m_frames_per_second;
  vector<frame_time> m_frame_times;
};

inline rx_f32 frame_timer::mspf() const {
  return m_frame_average;
}

inline rx_u32 frame_timer::fps() const {
  return m_frames_per_second;
}

inline rx_f32 frame_timer::delta_time() const {
  return static_cast<rx_f32>(m_delta_time);
}

inline rx_f64 frame_timer::resolution() const {
  return m_resolution;
}

inline rx_u64 frame_timer::ticks() const {
  return m_current_ticks;
}

inline const vector<frame_timer::frame_time>& frame_timer::frame_times() const & {
  return m_frame_times;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TIMER_H