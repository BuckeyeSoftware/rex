#ifndef RX_HUD_FRAME_GRAPH_H
#define RX_HUD_FRAME_GRAPH_H

namespace rx::render {
  struct immediate2D;
} // namespace rx::render

namespace rx::hud {

struct frame_graph {
  frame_graph(render::immediate2D* _immediate);
  void render();
private:
  render::immediate2D* m_immediate;
};

} // namespace rx::hud

#endif // RX_HUD_FRAME_GRAPH_H
