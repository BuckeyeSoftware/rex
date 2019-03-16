#ifndef RX_RENDER_TIMER_H
#define RX_RENDER_TIMER_H

#include <rx/core/types.h>

namespace rx::render {

struct frame_timer {
  frame_timer();

  rx_f32 mspf() const;
  rx_u32 fps() const;
  rx_f32 delta_time() const;
  rx_u32 ticks() const;

  void cap_fps(rx_f32 _max_fps);

  void reset();
  bool update();

private:
  rx_f32 m_max_frame_ticks;
  rx_u32 m_last_second_ticks;
  rx_u64 m_frame_count;
  rx_u32 m_min_ticks;
  rx_u32 m_max_ticks;
  rx_f32 m_average_ticks;
  rx_f32 m_delta_time;
  rx_u32 m_last_frame_ticks;
  rx_u32 m_current_ticks;
  rx_u32 m_target_ticks;
  rx_u32 m_frame_min;
  rx_u32 m_frame_max;
  rx_f32 m_frame_average;
  rx_u32 m_frames_per_second;
};

inline rx_f32 frame_timer::mspf() const {
  return m_frame_average;
}

inline rx_u32 frame_timer::fps() const {
  return m_frames_per_second;
}

inline rx_f32 frame_timer::delta_time() const {
  return m_delta_time;
}

inline rx_u32 frame_timer::ticks() const {
  return m_current_ticks;
}

} // namespace rx::render

#endif // RX_RENDER_TIMER_H