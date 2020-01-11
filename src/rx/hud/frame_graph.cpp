#include "rx/hud/frame_graph.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/timer.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

namespace rx::hud {

frame_graph::frame_graph(render::immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void frame_graph::render() {
  const render::frontend::interface& frontend{*m_immediate->frontend()};
  const render::frontend::frame_timer& _timer{frontend.timer()};
  const math::vec2f &screen_size{frontend.swapchain()->dimensions().cast<rx_f32>()};
  const math::vec2f box_size{600.0f, 200.0f};
  const rx_f32 box_bottom{25.0f};
  const rx_f32 box_middle{box_bottom + (box_size.h / 2)};
  const rx_f32 box_top{box_bottom + box_size.h};
  const rx_f32 box_left{screen_size.w / 2 - box_size.w / 2};
  const rx_f32 box_center{box_left + box_size.w / 2};
  const rx_f32 box_right{box_left + box_size.w};

  m_immediate->frame_queue().record_rectangle(
    {box_left, box_bottom},
    box_size,
    0,
    {0.0f, 0.0f, 0.0f, 0.5f});

  const auto k_frame_scale{16.667 * 2.0f};
  vector<math::vec2f> points;
  _timer.frame_times().each_fwd([&](const render::frontend::frame_timer::frame_time &_time) {
    const auto delta_x{rx_f32((_timer.ticks() * _timer.resolution() - _time.life) / render::frontend::frame_timer::k_frame_history_seconds)};
    const auto delta_y{rx_f32(algorithm::min(_time.frame / k_frame_scale, 1.0))};
    const math::vec2f point{box_right - delta_x * box_size.w, box_top - delta_y * box_size.h};
    points.push_back(point);
  });

  const rx_size n_points{points.size()};
  for (rx_size i{1}; i < n_points; i++) {
    m_immediate->frame_queue().record_line(
      points[i - 1],
      points[i],
      0.0f,
      1.0f,
      {0.0f, 1.0f, 0.0f, 1.0f});
  }

  m_immediate->frame_queue().record_line({box_left,   box_bottom}, {box_left,   box_top},    0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_line({box_center, box_bottom}, {box_center, box_top},    0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_line({box_right,  box_bottom}, {box_right,  box_top},    0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_line({box_left,   box_bottom}, {box_right,  box_bottom}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_line({box_left,   box_middle}, {box_right,  box_middle}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_line({box_left,   box_top},    {box_right,  box_top},    0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_center,       box_top    + 5.0f}, 18, 1.0f, render::immediate2D::text_align::k_center, "Frame Time", {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_top    - 5.0f}, 18, 1.0f, render::immediate2D::text_align::k_left, "0.0", {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_middle - 5.0f}, 18, 1.0f, render::immediate2D::text_align::k_left, string::format("%.1f", k_frame_scale * .5), {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_bottom - 5.0f}, 18, 1.0f, render::immediate2D::text_align::k_left, string::format("%.1f", k_frame_scale), {1.0f, 1.0f, 1.0f, 1.0f});
}

} // namespace rx::hud
