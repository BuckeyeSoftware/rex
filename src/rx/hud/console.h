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
  Console(Render::Immediate2D* _immediate);
  void render();
  void update(Rx::Console::Context& console_, Input::Context& input_);
private:
  Render::Immediate2D* m_immediate;
  Input::Text m_text;
  Size m_selection;
  Vector<String> m_suggestions;
  Vector<String> m_lines;
};

} // namespace rx::hud

#endif // RX_HUD_CONSOLE_H
