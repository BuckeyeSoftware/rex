#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H
#include "rx/input/layer.h"

namespace Rx::Input {

struct Context {
  enum : Uint8 {
    CLIPBOARD     = 1 << 0,
    MOUSE_CAPTURE = 1 << 1
  };

  Context();
  Context(Memory::Allocator& _allocator);

  void handle_event(const Event& _event);

  int on_update(Float32 _delta_time);
  void on_resize(const Math::Vec2z& _dimensions);

  Layer& root_layer() &;
  const Layer& root_layer() const &;
  Layer& active_layer() &;
  const Layer& active_layer() const &;

  const Vector<Layer*>& layers() const &;

  const String& clipboard() const &;
  Memory::Allocator& allocator() const;

private:
  friend struct Layer;

  bool raise_layer_by_index(Size _index);
  bool raise_layer_by_pointer(Layer* _layer);
  bool remove_layer_by_index(Size _index);
  bool remove_layer_by_pointer(Layer* _layer);

  void update_mouse_capture();
  void update_clipboard(String&& contents_);

  Memory::Allocator& m_allocator;
  Vector<Layer*> m_layers;
  Layer m_root;
  String m_clipboard;
  Uint8 m_updated;
  bool m_mouse_captured;
};

inline Context::Context()
  : Context{Memory::SystemAllocator::instance()}
{
}

inline Layer& Context::root_layer() & {
  return m_root;
}

inline const Layer& Context::root_layer() const & {
  return m_root;
}

inline Layer& Context::active_layer() & {
  return *m_layers[0];
}

inline const Layer& Context::active_layer() const & {
  return *m_layers[0];
}

inline const Vector<Layer*>& Context::layers() const & {
  return m_layers;
}

inline const String& Context::clipboard() const & {
  return m_clipboard;
}

inline Memory::Allocator& Context::allocator() const {
  return m_allocator;
}

} // namespace Rx::Input

#endif // RX_INPUT_CONTEXT_H
