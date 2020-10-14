#ifndef RX_HUD_CONSOLE_H
#define RX_HUD_CONSOLE_H
#include "rx/input/context.h"

namespace Rx::Render {
  struct Immediate2D;
} // namespace Rx::Render

namespace Rx::Console {
  struct Context;
} // namespace Rx::Console

namespace Rx::hud {

struct Console {
  Console(Render::Immediate2D* _immediate, Input::Context& _input);
  void render();
  void update(Rx::Console::Context& console_);
  void raise();
private:
  Render::Immediate2D* m_immediate;
  Input::Context& m_input_context;
  Input::Layer m_input_layer;
  Input::Text m_text;
  Size m_selection;
  Vector<String> m_suggestions;
  Vector<String> m_lines;
};

inline void Console::raise() {
  m_input_layer.raise();
}

} // namespace rx::hud

#endif // RX_HUD_CONSOLE_H
