#ifndef RX_RENDER_CANVAS_H
#define RX_RENDER_CANVAS_H
#include "rx/core/vector.h"
#include "rx/math/mat3x4.h"
#include "rx/render/frontend/state.h"

extern "C" typedef struct NVGcontext NVGcontext;

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Buffer;
  struct Program;
  struct Target;
  struct Texture2D;
  struct Technique;
} // namespace Frontend

struct Canvas {
  RX_MARK_NO_COPY(Canvas);

  enum : Uint8 {
    ANTIALIAS       = 1 << 0,
    STENCIL_STROKES = 1 << 1
  };

  static Optional<Canvas> create(Frontend::Context* _context,
    const Math::Vec2z& _dimensions, Uint8 _flags);

  Frontend::Texture2D* texture() const;

  Canvas() : m_context{nullptr} {}
  Canvas(Canvas&& canvas_);
  ~Canvas();

  Canvas& operator=(Canvas&& canvas_);

  operator NVGcontext*() const;

private:
  constexpr Canvas(NVGcontext* _context);

  void release();

  NVGcontext* m_context;
};

inline constexpr Canvas::Canvas(NVGcontext* _context)
  : m_context{_context}
{
}

inline Canvas::Canvas(Canvas&& canvas_)
  : m_context{Utility::exchange(canvas_.m_context, nullptr)}
{
}

inline Canvas::~Canvas() {
  release();
}

inline Canvas& Canvas::operator=(Canvas&& canvas_) {
  if (&canvas_ != this) {
    release();
    m_context = Utility::exchange(canvas_.m_context, nullptr);
  }
  return *this;
}

inline Canvas::operator NVGcontext*() const {
  return m_context;
}

} // namespace Rx::Render

#endif // RX_RENDER_CANVAS_H