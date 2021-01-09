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

  // Called on initialization of the application.
  //
  // This should return false when the application fails to initialize.
  virtual bool on_init() = 0;

  // Called when application is updated.
  //
  // This is not necessarily called at the same rate as |on_render|.
  //
  // The |_delta_time| is constant.
  //
  // This should return true if the application should continue running.
  virtual bool on_update(Float32 _delta_time) = 0;

  // Called when application is rendered.
  //
  // This is not necessarily called at the same rate as |on_update|.
  //
  // This should return true if something was rendered. When something is not
  // rendered, returning false acts as a power saving optimization.
  virtual bool on_render() = 0;

  // Called when application is resized.
  //
  // The resized resolution is given as |_resolution|.
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
