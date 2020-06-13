#ifndef RX_HUD_RENDER_STATS_H
#define RX_HUD_RENDER_STATS_H

namespace Rx::Render {
  struct Immediate2D;
} // namespace rx::render

namespace Rx::hud {

struct RenderStats {
  RenderStats(Render::Immediate2D* _immediate);
  void render();
private:
  Render::Immediate2D* m_immediate;
};

} // namespace rx::hud

#endif // RX_HUD_RENDER_STATS_H
