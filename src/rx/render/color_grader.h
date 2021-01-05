#ifndef RX_RENDER_COLOR_GRADER_H
#define RX_RENDER_COLOR_GRADER_H
#include "rx/core/bitset.h"
#include "rx/core/string.h"

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Texture3D;
} // namespace Frontend

struct ColorGrader {
  RX_MARK_NO_COPY(ColorGrader);

  // The maximum Z depth of an atlas.
  static inline constexpr const Size MAX_DEPTH = 4096;

  // An entry in the atlas.
  struct Entry {
    RX_MARK_NO_COPY(Entry);

    constexpr Entry();
    constexpr Entry(ColorGrader* _atlas, Uint16 _handle);
    Entry(Entry&&);
    ~Entry();
    Entry& operator=(Entry&& entry_);

    // Write |_samples| into the entry.
    void write(const Math::Vec4f* _samples); // Linear single-precision float.
    void write(const Math::Vec4h* _samples); // Linear half-precision float.
    void write(const Math::Vec4b* _samples); // 8-bit sRGB.

    // The offset and scale of this entry in the atlas in texel units.
    Math::Vec2f properties() const;

    // The atlas this entry is part of.
    const ColorGrader* atlas() const;

  private:
    void release();

    ColorGrader* m_atlas;
    Uint16 m_handle;
  };

  ColorGrader();
  ColorGrader(ColorGrader&& atlas_);
  ~ColorGrader();
  ColorGrader& operator=(ColorGrader&& atlas_);

  static Optional<ColorGrader> create(Frontend::Context* _context, Size _size);

  Optional<Entry> allocate();
  Optional<Entry> load(const String& _file_name);

  // Update the atlas. This needs to be called when entries are written to.
  void update();

  Frontend::Texture3D* texture() const;

private:
  friend struct Entry;

  void release();

  Optional<Entry> allocate_uninitialized();

  Frontend::Context* m_frontend;
  Frontend::Texture3D* m_texture;

  Bitset m_allocated;
  Bitset m_dirty;

  // The neutral table samples.
  Vector<Math::Vec4h> m_neutral;

  // Scratch space for performing LUT conversions to avoid allocations.
  Concurrency::SpinLock m_scratch_lock;
  Vector<Math::Vec4h> m_scratch RX_HINT_GUARDED_BY(m_scratch_lock);
};

// [ColorGrader]
inline ColorGrader::~ColorGrader() {
  release();
}

inline ColorGrader& ColorGrader::operator=(ColorGrader&& atlas_) {
  release();
  m_frontend = Utility::exchange(atlas_.m_frontend, nullptr);
  m_texture = Utility::exchange(atlas_.m_texture, nullptr);
  m_allocated = Utility::move(atlas_.m_allocated);
  m_dirty = Utility::move(atlas_.m_dirty);
  m_neutral = Utility::move(atlas_.m_neutral);
  m_scratch = Utility::move(atlas_.m_scratch);
  return *this;
}

inline ColorGrader::ColorGrader(ColorGrader&& atlas_)
  : m_frontend{Utility::exchange(atlas_.m_frontend, nullptr)}
  , m_texture{Utility::exchange(atlas_.m_texture, nullptr)}
  , m_allocated{Utility::move(atlas_.m_allocated)}
  , m_dirty{Utility::move(atlas_.m_dirty)}
  , m_neutral{Utility::move(atlas_.m_neutral)}
  , m_scratch{Utility::move(atlas_.m_scratch)}
{
}

inline Frontend::Texture3D* ColorGrader::texture() const {
  return m_texture;
}

// [ColorGrader::Entry]
inline constexpr ColorGrader::Entry::Entry()
  : Entry{nullptr, 0}
{
}

inline constexpr ColorGrader::Entry::Entry(ColorGrader* _atlas, Uint16 _handle)
  : m_atlas{_atlas}
  , m_handle{_handle}
{
}

inline ColorGrader::Entry::Entry(Entry&& entry_)
  : m_atlas{Utility::exchange(entry_.m_atlas, nullptr)}
  , m_handle{Utility::exchange(entry_.m_handle, 0)}
{
}

inline ColorGrader::Entry::~Entry() {
  release();
}

inline ColorGrader::Entry& ColorGrader::Entry::operator=(Entry&& entry_) {
  release();
  m_atlas = Utility::exchange(entry_.m_atlas, nullptr);
  m_handle = Utility::exchange(entry_.m_handle, 0);
  return *this;
}

inline void ColorGrader::Entry::release() {
  if (m_atlas) {
    m_atlas->m_allocated.clear(m_handle);
  }
}

inline const ColorGrader* ColorGrader::Entry::atlas() const {
  return m_atlas;
}

} // namespace Rx::Render

#endif // RX_RENDER_COLOR_GRADER_H
