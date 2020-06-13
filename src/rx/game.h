#ifndef RX_GAME_H
#define RX_GAME_H
#include "rx/core/concepts/interface.h"

#include "rx/input/context.h"

namespace Rx {

struct Game
  : Concepts::Interface
{
  enum class status {
    k_running,
    k_restart,
    k_shutdown
  };

  virtual bool on_init() = 0;
  virtual status on_slice(Input::Context& _input) = 0;
  virtual void on_resize(const Math::Vec2z& _resolution) = 0;
};

} // namespace rx

#endif // RX_GAME_H
