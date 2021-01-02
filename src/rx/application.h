#ifndef RX_APPLICATION_H
#define RX_APPLICATION_H
#include "rx/math/vec2.h"
#include "rx/core/markers.h"

namespace Rx {

struct Engine;

struct Application {
  RX_MARK_NO_COPY(Application);
  RX_MARK_NO_MOVE(Application);

  virtual ~Application() = default;

  constexpr Application(Engine* _engine);

  virtual bool on_init() = 0;
  virtual bool on_update() = 0;
  virtual bool on_render() = 0;
  virtual void on_resize(const Math::Vec2z& _resolution) = 0;

  Engine* engine() const;

private:
  Engine* m_engine;
};

inline constexpr Application::Application(Engine* _engine)
  : m_engine{_engine}
{
}

inline Engine* Application::engine() const {
  return m_engine;
}

} // namespace Rx

#endif // RX_APPLICATION_H
