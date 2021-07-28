#ifndef RX_RENDER_COLOR_GRADER_H
#define RX_RENDER_COLOR_GRADER_H
#include "rx/core/bitset.h"
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/map.h"

#include "rx/core/memory/null_allocator.h"
#include "rx/core/concurrency/word_lock.h"

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Texture3D;
} // namespace Frontend

struct ColorGrader;

struct ColorGrader {
  RX_MARK_NO_COPY(ColorGrader);
  RX_MARK_NO_MOVE(ColorGrader);

  ColorGrader(Frontend::Context* _frontend);

  // The maximum Z depth of an atlas.
  static inline constexpr const Size MAX_DEPTH = 4096;

  struct Atlas;

  // An entry in the atlas.
  struct Entry {
    RX_MARK_NO_COPY(Entry);

    constexpr Entry();
    constexpr Entry(Atlas* _atlas, Uint16 _handle);
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
    const Atlas* atlas() const;

  private:
    void release();

    Atlas* m_atlas;
    Uint16 m_handle;
  };

  struct Atlas {
    RX_MARK_NO_COPY(Atlas);

    Atlas();
    ~Atlas();
    Atlas(Atlas&& atlas_);
    Atlas& operator=(Atlas&& atlas_);

    static Optional<Atlas> create(ColorGrader* _color_grader, Size _size);

    Optional<Entry> allocate();

    // Update the atlas. This needs to be called when entries are written to.
    void update();

    Frontend::Texture3D* texture() const;

  private:
    Atlas(ColorGrader* _color_grader, Size _size, Bitset&& allocated_,
      Bitset&& dirty_, Vector<Math::Vec4h>&& neutral_,
      Vector<Math::Vec4h>&& scratch_);

    void release();

    friend struct Entry;
    friend struct ColorGrader;

    Optional<Entry> allocate_uninitialized();

    ColorGrader* m_color_grader;

    Frontend::Texture3D* m_texture;
    Size m_size;

    Bitset m_allocated;
    Bitset m_dirty;

    // The neutral table samples.
    Vector<Math::Vec4h> m_neutral;

    // Scratch space for performing LUT conversions to avoid allocations.
    Concurrency::WordLock m_scratch_lock;
    Vector<Math::Vec4h> m_scratch RX_HINT_GUARDED_BY(m_scratch_lock);
  };

  static Optional<ColorGrader> create(Frontend::Context* _context);

  // Load LUT from |_file_name|.
  Optional<Entry> load(const StringView& _file_name);

  // Allocate neutral LUT of size |_size|.
  Optional<Entry> allocate(Size _size);

  void update();

private:
  Atlas* find_or_create_atlas(Size _size);

  Frontend::Context* m_frontend;

  Concurrency::WordLock m_atlases_lock;
  Map<Size, Atlas> m_atlases RX_HINT_GUARDED_BY(m_atlases_lock);
};

// [ColorGrader::Entry]
inline constexpr ColorGrader::Entry::Entry()
  : Entry{nullptr, 0}
{
}

inline constexpr ColorGrader::Entry::Entry(Atlas* _atlas, Uint16 _handle)
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
  if (this != &entry_) {
    release();
    m_atlas = Utility::exchange(entry_.m_atlas, nullptr);
    m_handle = Utility::exchange(entry_.m_handle, 0);
  }
  return *this;
}

inline void ColorGrader::Entry::release() {
  if (m_atlas) {
    m_atlas->m_allocated.clear(m_handle);
  }
}

inline const ColorGrader::Atlas* ColorGrader::Entry::atlas() const {
  return m_atlas;
}

// [ColorGrader::Atlas]
inline ColorGrader::Atlas::Atlas()
  : Atlas {
    nullptr,
    0,
    { },
    { },
    { Memory::NullAllocator::instance() },
    { Memory::NullAllocator::instance() }
  }
{
}

inline  ColorGrader::Atlas::Atlas(ColorGrader* _color_grader, Size _size,
  Bitset&& allocated_, Bitset&& dirty_, Vector<Math::Vec4h>&& neutral_,
  Vector<Math::Vec4h>&& scratch_)
  : m_color_grader{_color_grader}
  , m_texture{nullptr}
  , m_size{_size}
  , m_allocated{Utility::move(allocated_)}
  , m_dirty{Utility::move(dirty_)}
  , m_neutral{Utility::move(neutral_)}
  , m_scratch{Utility::move(scratch_)}
{
}

inline ColorGrader::Atlas::Atlas(Atlas&& atlas_)
  : m_color_grader{Utility::exchange(atlas_.m_color_grader, nullptr)}
  , m_texture{Utility::exchange(atlas_.m_texture, nullptr)}
  , m_size{Utility::exchange(atlas_.m_size, 0)}
  , m_allocated{Utility::move(atlas_.m_allocated)}
  , m_dirty{Utility::move(atlas_.m_dirty)}
  , m_neutral{Utility::move(atlas_.m_neutral)}
  , m_scratch{Utility::move(atlas_.m_scratch)}
{
}

inline ColorGrader::Atlas::~Atlas() {
  release();
}

inline ColorGrader::Atlas& ColorGrader::Atlas::operator=(Atlas&& atlas_) {
  if (this != &atlas_) {
    release();
    m_color_grader = Utility::exchange(atlas_.m_color_grader, nullptr);
    m_texture = Utility::exchange(atlas_.m_texture, nullptr);
    m_size = Utility::exchange(atlas_.m_size, 0);
    m_allocated = Utility::move(atlas_.m_allocated);
    m_dirty = Utility::move(atlas_.m_dirty);
    m_neutral = Utility::move(atlas_.m_neutral);
    m_scratch = Utility::move(atlas_.m_scratch);
  }
  return *this;
}

inline Frontend::Texture3D* ColorGrader::Atlas::texture() const {
  return m_texture;
}

} // namespace Rx::Render

#endif // RX_RENDER_COLOR_GRADER_H
