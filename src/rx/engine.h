#ifndef RX_ENGINE_H
#define RX_ENGINE_H
#include "rx/core/log.h"
#include "rx/core/ptr.h"

#include "rx/console/context.h"
#include "rx/console/variable.h"

#include "rx/input/context.h"

#include "rx/display.h"
#include "rx/application.h"

namespace Rx {

namespace Render::Backend { struct Context; }
namespace Render::Frontend { struct Context; }

struct Engine {
  RX_MARK_NO_COPY(Engine);
  RX_MARK_NO_MOVE(Engine);

  Engine();
  ~Engine();

  bool init();

  enum class Status {
    RUNNING,
    RESTART,
    SHUTDOWN
  };

  // Run's one iteration of the engine loop.
  Status run();

  Console::Context& console();
  Input::Context& input();
  Render::Frontend::Context* renderer();

protected:
  friend struct Application;

  Console::Context m_console;
  Input::Context m_input;
  Render::Backend::Context* m_render_backend;
  Render::Frontend::Context* m_render_frontend;
  Vector<Log::WriteEvent::Handle> m_logging_event_handles;
  Vector<Display> m_displays;
  Status m_status;

  // The following event handlers deal with common console events.
  Event<void(Console::Variable<Sint32>&)>::Handle m_on_display_fullscreen_change;
  Event<void(Console::Variable<Sint32>&)>::Handle m_on_display_swap_interval_change;
  Event<void(Console::Variable<Math::Vec2i>&)>::Handle m_on_display_resolution_change;

  Ptr<Application> m_application;
};

inline Console::Context& Engine::console() {
  return m_console;
}

inline Input::Context& Engine::input() {
  return m_input;
}

inline Render::Frontend::Context* Engine::renderer() {
  return m_render_frontend;
}

} // namespace Rx

#endif // RX_ENGINE_H
