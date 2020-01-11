#ifndef RX_HUD_CONSOLE_H
#define RX_HUD_CONSOLE_H
#include "rx/input/text.h"

namespace rx::render {
  struct immediate2D;
} // namespace rx::render


namespace rx::hud {

struct console {
  console(render::immediate2D* _immediate);
  void render();
  void update(input::input& input_);
private:
  render::immediate2D* m_immediate;
  input::text m_text;
  rx_size m_selection;
  vector<string> m_suggestions;
};

} // namespace rx::hud

#endif // RX_HUD_CONSOLE_H
