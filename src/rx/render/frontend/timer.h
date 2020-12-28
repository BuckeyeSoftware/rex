#ifndef RX_RENDER_FRONTEND_TIMER_H
#define RX_RENDER_FRONTEND_TIMER_H
#include "rx/core/vector.h"

namespace Rx::Render::Frontend {

struct FrameTimer {
  static constexpr Float64 k_frame_history_seconds{2.0};

  FrameTimer();

  Float32 mspf() const;
  Uint32 fps() const;
  Float32 delta_time() const;
  Float64 resolution() const;
  Uint64 ticks() const;

  void cap_fps(Float32 _max_fps);

  void reset();
  bool update();

  struct FrameTime {
    Float64 life;
    Float64 frame;
  };

  const Vector<FrameTime>& frame_times() const &;

private:
  Uint64 m_frequency;
  Float64 m_resolution;
  Float32 m_max_frame_ticks;
  Uint64 m_last_second_ticks;
  Uint64 m_frame_count;
  Uint64 m_min_ticks;
  Uint64 m_max_ticks;
  Float64 m_average_ticks;
  Float64 m_delta_time;
  Uint64 m_last_frame_ticks;
  Uint64 m_current_ticks;
  Uint64 m_target_ticks;
  Uint64 m_frame_min;
  Uint64 m_frame_max;
  Float32 m_frame_average;
  Uint32 m_frames_per_second;
  Vector<FrameTime> m_frame_times;
};

inline Float32 FrameTimer::mspf() const {
  return m_frame_average;
}

inline Uint32 FrameTimer::fps() const {
  return m_frames_per_second;
}

inline Float32 FrameTimer::delta_time() const {
  return static_cast<Float32>(m_delta_time);
}

inline Float64 FrameTimer::resolution() const {
  return m_resolution;
}

inline Uint64 FrameTimer::ticks() const {
  return m_current_ticks;
}

inline const Vector<FrameTimer::FrameTime>& FrameTimer::frame_times() const & {
  return m_frame_times;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TIMER_H
