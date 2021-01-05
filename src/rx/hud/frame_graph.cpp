#include "rx/hud/frame_graph.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/timer.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

namespace Rx::hud {

FrameGraph::FrameGraph(Render::Immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void FrameGraph::render() {
  const Render::Frontend::Context& frontend{*m_immediate->frontend()};
  const Render::Frontend::FrameTimer& _timer{frontend.timer()};
  const Math::Vec2f &screen_size{frontend.swapchain()->dimensions().cast<Float32>()};
  const Math::Vec2f box_size{600.0f, 200.0f};
  const Float32 box_bottom{25.0f};
  const Float32 box_middle{box_bottom + (box_size.h / 2)};
  const Float32 box_top{box_bottom + box_size.h};
  const Float32 box_left{screen_size.w / 2 - box_size.w / 2};
  const Float32 box_center{box_left + box_size.w / 2};
  const Float32 box_right{box_left + box_size.w};

  m_immediate->frame_queue().record_rectangle(
    {box_left, box_bottom},
    box_size,
    0,
    {0.0f, 0.0f, 0.0f, 0.5f});

  static constexpr const auto FRAME_SCALE = 16.667f * 2.0f;
  Vector<Math::Vec2f> points;
  _timer.frame_times().each_fwd([&](const Render::Frontend::FrameTimer::FrameTime &_time) {
    const auto delta_x{Float32((_timer.ticks() * _timer.resolution() - _time.life) / Render::Frontend::FrameTimer::HISTORY_SECONDS)};
    const auto delta_y{Float32(Algorithm::min(_time.frame / FRAME_SCALE, 1.0))};
    const Math::Vec2f point{box_right - delta_x * box_size.w, box_top - delta_y * box_size.h};
    points.push_back(point);
  });

  const Size n_points{points.size()};
  for (Size i{1}; i < n_points; i++) {
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
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_center,       box_top    + 5.0f}, 18, 1.0f, Render::Immediate2D::TextAlign::CENTER, "Frame Time", {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_top    - 5.0f}, 18, 1.0f, Render::Immediate2D::TextAlign::LEFT, "0.0", {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_middle - 5.0f}, 18, 1.0f, Render::Immediate2D::TextAlign::LEFT, String::format("%.1f", FRAME_SCALE * .5), {1.0f, 1.0f, 1.0f, 1.0f});
  m_immediate->frame_queue().record_text("Inconsolata-Regular", {box_right + 5.0f, box_bottom - 5.0f}, 18, 1.0f, Render::Immediate2D::TextAlign::LEFT, String::format("%.1f", FRAME_SCALE), {1.0f, 1.0f, 1.0f, 1.0f});
}

} // namespace rx::hud
