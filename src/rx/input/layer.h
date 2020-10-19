#ifndef RX_INPUT_LAYER_H
#define RX_INPUT_LAYER_H
#include "rx/core/vector.h"

#include "rx/input/mouse.h"
#include "rx/input/keyboard.h"
#include "rx/input/event.h"
#include "rx/input/controller.h"
#include "rx/input/text.h"

#include "rx/math/rectangle.h"

namespace Rx::Render {
  struct Immediate2D;
} // namespace Rx::Render

namespace Rx::Input {

struct Context;

// # Input Layer
//
// The purpose of an input layer is to isolate input to a given active region
// to prevent input bleed and make input management much easier.
//
// Layers behave a lot like windows do in a graphical user interface. They're
// described by a rectangular region and can be raised by clicking on them,
// or programatically.
//
// Layers retain input state when switched between.
struct Layer {
  using Region = Math::Rectangle<Float32>;

  Layer(Context* _context);
  ~Layer();

  void capture_mouse(bool _capture);
  void capture_text(Text* text_);
  void resize(const Math::Vec2f& _dimensions);
  void move(const Math::Vec2f& _offset);
  void raise();

  bool is_active() const;
  bool is_text_captured() const;
  bool is_mouse_captured() const;
  const Region& region() const &;
  const Mouse& mouse() const &;
  const Keyboard& keyboard() const &;
  const Vector<Controller>& controllers() const &;
  Text* text() const;

  void render_region(Render::Immediate2D* _immediate) const;

private:
  friend struct Context;

  void handle_event(const Event& _event);
  void update(Float32 _delta_time);

  Context* m_context;
  Text* m_text;
  Region m_region;
  Mouse m_mouse;
  Keyboard m_keyboard;
  Vector<Controller> m_controllers;
  bool m_mouse_captured;
};

inline void Layer::resize(const Math::Vec2f& _dimensions) {
  m_region.dimensions = _dimensions;
}

inline void Layer::move(const Math::Vec2f& _offset) {
  m_region.offset = _offset;
}

inline bool Layer::is_text_captured() const {
  return m_text != nullptr;
}

inline bool Layer::is_mouse_captured() const {
  return m_mouse_captured;
}

inline const Layer::Region& Layer::region() const & {
  return m_region;
}

inline const Mouse& Layer::mouse() const & {
  return m_mouse;
}

inline const Keyboard& Layer::keyboard() const & {
  return m_keyboard;
}

inline const Vector<Controller>& Layer::controllers() const & {
  return m_controllers;
}

inline Text* Layer::text() const {
  return m_text;
}

} // namespace Rx::Input

#endif // RX_INPUT_LAYER_H
