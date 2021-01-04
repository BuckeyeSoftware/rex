#ifndef RX_RENDER_COLOR_GRADER_H
#define RX_RENDER_COLOR_GRADER_H
#include "rx/core/bitset.h"
#include "rx/core/string.h"

#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Texture3D;
} // namespace Frontend

struct ColorGrader {
  RX_MARK_NO_COPY(ColorGrader);

  // Each LUT is w=SIZE, h=SIZE, d=SIZE.
  static inline constexpr const Size SIZE = 16;

  // The maximum depth size of the 3D texture.
  // The full 3D texture is w=SIZE, h=SIZE, d=MAX_DEPTH.
  static inline constexpr const Size MAX_DEPTH = 4096;

  // The number of LUTs that can be stored along Z, since d=SIZE.
  static inline constexpr const Size MAX_LUTS = MAX_DEPTH / SIZE;

  // Single lookup table. When allocated it always contains a neutral table.
  struct LUT {
    RX_MARK_NO_COPY(LUT);

    constexpr LUT();
    LUT(LUT&& lut_);
    ~LUT();

    LUT& operator=(LUT&& lut_);

    // Write |SIZE * SIZE * SIZE * sizeof(Math::Vec4h)| bytes to the LUT,
    // replacing the contents.
    //
    // These LUTs can be authored externally.
    void write(const Byte* _data);

    // Read a color grading LUT from a file directly. These can be authored
    // in w=SIZE*SIZE, h=SIZE format like those accepted by UE4.
    //
    // TODO(dweiler):
    //  Support .cube files for color grading grading, like those used by
    //  professional studio cameras and Adobe Premiere.
    bool read(const String& _file_name);

    // The scale and offset of the LUT in the atlas in texel units.
    Math::Vec2f properties() const;

    // The atlas this LUT belongs to.
    Frontend::Texture3D* atlas() const;

  private:
    friend struct ColorGrader;

    constexpr LUT(ColorGrader* _grader, Uint32 _handle);

    void release();

    ColorGrader* m_grader;
    Uint32 m_handle;
  };

  ColorGrader(Frontend::Context* _frontend);
  ~ColorGrader();

  void create();

  // Must be called to update dirty LUTs.
  void update();

  // Allocate a LUT. Keep the LUT object around for as long as it's used.
  Optional<LUT> allocate();

  Frontend::Texture3D* texture() const;

private:
  friend struct LUT;

  void write(Uint32 _handle, const Byte* _data);

  Frontend::Context* m_frontend;
  Frontend::Texture3D* m_texture;

  // List of allocated and dirty LUTs.
  Bitset m_allocated;
  Bitset m_dirty;
};

// [ColorGrader::LUT]
inline constexpr ColorGrader::LUT::LUT()
  : LUT{nullptr, 0}
{
}

inline ColorGrader::LUT::LUT(LUT&& lut_)
  : m_grader{Utility::exchange(lut_.m_grader, nullptr)}
  , m_handle{Utility::exchange(lut_.m_handle, 0)}
{
}

inline ColorGrader::LUT::~LUT() {
  release();
}

inline ColorGrader::LUT& ColorGrader::LUT::operator=(LUT&& lut_) {
  release();
  m_grader = Utility::exchange(lut_.m_grader, nullptr);
  m_handle = Utility::exchange(lut_.m_handle, 0);
  return *this;
}

inline constexpr ColorGrader::LUT::LUT(ColorGrader* _grader, Uint32 _handle)
  : m_grader{_grader}
  , m_handle{_handle}
{
}

inline void ColorGrader::LUT::release() {
  if (m_grader) {
    m_grader->m_allocated.clear(m_handle);
  }
}

inline void ColorGrader::LUT::write(const Byte* _data) {
  m_grader->write(m_handle, _data);
}

inline Math::Vec2f ColorGrader::LUT::properties() const {
  RX_ASSERT(m_grader, "invalid");

  // Calculate the scale and offset in the atlas for this LUT. This clamps the
  // UVs in such a way to stay on pixel centers to avoid sampling adjacent
  // slices when at the boundary of the LUT.
  const auto uvs_per_slice = 1.0f / ColorGrader::MAX_DEPTH;
  const auto uvs_per_lut = (uvs_per_slice * ColorGrader::SIZE);
  const auto scale = (ColorGrader::SIZE - 1.0f) / ColorGrader::MAX_DEPTH;
  const auto offset = uvs_per_slice * 0.5f + uvs_per_lut * m_handle;
  return {scale, offset};
}

inline Frontend::Texture3D* ColorGrader::LUT::atlas() const {
  RX_ASSERT(m_grader, "invalid");
  return m_grader->texture();
}

// [ColorGrader]
inline Frontend::Texture3D* ColorGrader::texture() const {
  return m_texture;
}

} // namespace Rx::Render

#endif // RX_RENDER_COLOR_GRADER_H
