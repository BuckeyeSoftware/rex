#include <string.h> // memmove

#include "rx/input/context.h"

#include "rx/core/hints/unreachable.h"

#include "rx/core/utility/swap.h"

namespace Rx::Input {

Context::Context(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_layers{m_allocator}
  , m_root{this}
  , m_clipboard{m_allocator}
  , m_updated{false}
{
  // The root context should have the mouse captured.
  m_root.capture_mouse(true);
}

void Context::handle_event(const Event& _event) {
  switch (_event.type) {
  case Event::Type::k_mouse_button:
    // Can only change layer when mouse isn't captured and button was pressed.
    if (!active_layer().is_mouse_captured() && _event.as_mouse_button.down) {
      const auto& position = _event.as_mouse_button.position.cast<Float32>();
      // Ignore if the press occured in the active layer.
      if (!active_layer().region().contains(position)) {
        // Check for intersection in all other layers.
        for (Size n_layers = m_layers.size(), i = 0; i < n_layers; i++) {
          // When we intersect, make that layer active.
          if (m_layers[i]->region().contains(position)) {
            raise_layer(m_layers[i]);
            break;
          }
        }
      }
    }
    [[fallthrough]];
  default:
    // Send the event to the active layer.
    active_layer().handle_event(_event);
    break;
  case Event::Type::k_controller_notification:
    // Controller notification events should be sent to every layer.
    m_layers.each_fwd([&](Layer* _layer) { _layer->handle_event(_event); });
    break;
  case Event::Type::k_clipboard:
    // The clipboard is context-global.
    m_clipboard = Utility::move(_event.as_clipboard.contents);
    break;
  }
}

int Context::on_update(Float32 _delta_time) {
  m_layers.each_fwd([&](Layer* _layer) { _layer->update(_delta_time); });
  update_mouse_capture();
  return Utility::exchange(m_updated, 0);
}

void Context::on_resize(const Math::Vec2z& _dimensions) {
  const auto new_scale = _dimensions.cast<Float32>();
  if (m_root.region().dimensions.area() <= 0.0f) {
    m_root.resize(new_scale);
    return;
  }
  const auto old_scale = m_root.region().dimensions;
  m_layers.each_fwd([&](Layer* layer_) {
    const auto& region = layer_->region();
    layer_->resize(region.dimensions / old_scale * new_scale);
    layer_->move(region.offset/ old_scale * new_scale);
  });
}

bool Context::raise_layer(Layer* _layer) {
  if (const auto index = m_layers.find(_layer)) {
    // Erase the layer and push it to the back.
    auto layer = m_layers[*index];
    m_layers.erase(*index, *index + 1);
    return m_layers.push_back(layer);
  }
  return false;
}

bool Context::append_layer(Layer* _layer) {
  // Prevent appending the same layer more than once.
  if (const auto index = m_layers.find(_layer)) {
    return false;
  }
  m_layers.push_back(_layer);
  return true;
}

bool Context::remove_layer(Layer* _layer) {
  if (const auto index = m_layers.find(_layer)) {
    m_layers.erase(*index, *index + 1);
    return true;
  }
  return false;
}

void Context::update_mouse_capture() {
  bool capture = active_layer().is_mouse_captured();
  if (capture != m_mouse_captured) {
    m_updated |= MOUSE_CAPTURE;
    m_mouse_captured = capture;
  }
}

void Context::update_clipboard(String&& contents_) {
  m_updated |= CLIPBOARD;
  m_clipboard = Utility::move(contents_);
}

void Context::render_regions(Render::Immediate2D* _immediate) const {
  m_layers.each_fwd([_immediate](const Layer* _layer) {
    _layer->render_region(_immediate);
  });
}

} // namespace rx::input
