#ifndef RX_GAME_H
#define RX_GAME_H
#include "rx/math/vec2.h"

#include "rx/core/markers.h"

namespace Rx {

namespace Console { struct Context; }
namespace Input { struct Context; }

struct Game {
  RX_MARK_INTERFACE(Game);

  virtual bool on_init() = 0;
  virtual bool on_update(Console::Context& console_, Input::Context& input_) = 0;
  virtual bool on_render(Console::Context& console_) = 0;
  virtual void on_resize(const Math::Vec2z& _resolution) = 0;
};

} // namespace rx

#endif // RX_GAME_H
