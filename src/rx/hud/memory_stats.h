#ifndef RX_HUD_MEMORY_STATS_H
#define RX_HUD_MEMORY_STATS_H

namespace rx::render {
  struct immediate2D;
} // namespace rx::render

namespace rx::hud {

struct memory_stats {
  memory_stats(render::immediate2D* _immediate);
  void render();
private:
  render::immediate2D* m_immediate;
};

} // namespace rx::hud

#endif // RX_HUD_MEMORY_STATS_H
