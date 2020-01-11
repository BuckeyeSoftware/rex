#ifndef RX_HUD_RENDER_STATS_H
#define RX_HUD_RENDER_STATS_H

namespace rx::render {
  struct immediate2D;
} // namespace rx::render

namespace rx::hud {

struct render_stats {
  render_stats(render::immediate2D* _immediate);
  void render();
private:
  render::immediate2D* m_immediate;
};

} // namespace rx::hud

#endif // RX_HUD_RENDER_STATS_H
