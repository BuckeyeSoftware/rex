#ifndef RX_GAME_H
#define RX_GAME_H
#include "rx/input/context.h"

#include "rx/core/markers.h"

namespace Rx {

struct Game {
  RX_MARK_INTERFACE(Game);

  enum class Status {
    RUNNING,
    RESTART,
    SHUTDOWN
  };

  virtual bool on_init() = 0;
  virtual Status on_update(Input::Context& _input) = 0;
  virtual bool on_render() = 0;
  virtual void on_resize(const Math::Vec2z& _resolution) = 0;
};

} // namespace rx

#endif // RX_GAME_H
