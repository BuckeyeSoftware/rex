#ifndef RX_GAME_H
#define RX_GAME_H
#include "rx/core/concepts/interface.h"

#include "rx/input/input.h"

namespace rx {

struct game
  : concepts::interface
{
  virtual bool on_init() = 0;
  virtual bool on_slice(const input::input& _input) = 0;
  virtual void on_resize(const math::vec2z& _resolution) = 0;
};

} // namespace rx

#endif // RX_GAME_H
