#ifndef RX_RENDER_FRONTEND_SAMPLER_H
#define RX_RENDER_FRONTEND_SAMPLER_H
#include "rx/core/types.h"
#include "rx/core/algorithm/saturate.h"

namespace Rx::Render::Frontend {

struct Sampler {
  /// Filter combinations:
  ///
  /// | MAG     | MIN      | MIPMAP  | NEAR FILTERING | FAR FILTERING |
  /// +---------+----------+---------+----------------+---------------|
  /// | NEAREST | NEAREST  | NEAREST | Off            | Off           |
  /// | NEAREST | LINEAR   | NEAREST | Off            | On            |
  /// | NEAREST | NEAREST  | LINEAR  | Off            | Off           |
  /// | NEAREST | LINEAR   | LINEAR  | Off            | On            |
  /// | NEAREST | NEAREST  | NONE    | Off            | Off           |
  /// | NEAREST | LINEAR   | NONE    | Off            | On            |
  /// | LINEAR  | NEAREST  | NEAREST | On             | Off           |
  /// | LINEAR  | LINEAR   | NEAREST | On             | On            |
  /// | LINEAR  | NEAREST  | LINEAR  | On             | Off           |
  /// | LINEAR  | LINEAR   | LINEAR  | On             | On            |
  /// | LINEAR  | NEAREST  | NONE    | On             | Off           |
  /// | LINEAR  | LINEAR   | NONE    | On             | On            |
  ///
  /// Some combinations conceptually don't make sense, for instance:
  ///   MAG = NEAREST, MIN = LINEAR, MIPMAP = LINEAR
  /// Would have a trilinear filtered portion between mips that would sharply
  /// break for the non-filtered magnification portion.
  ///
  /// When MIPMAP != NONE The GL equivalent is
  ///  min = GL_${MIN}
  ///  mag = GL_${MAG}_MIPMAP_${MIPMAP} 
  /// When MIPMAP == NONE The GL equivalent is
  ///  min = GL_${MIN}
  ///  mag = GL_${MAG}
  enum class Filter {
    NEAREST, ///< Nearest filtering within.
    LINEAR   ///< Linear filtering between (Bilinear.)
  };

  /// The type of filtering to use on mipmaps.
  enum class MipmapMode {
    NONE,    ///< No mipmaps.
    NEAREST, ///< Nearest filtering within a mip level.
    LINEAR   ///< Linear filtering between mip level (Trilinear.)
  };

  /// The addressing mode to use for a given texture axis.
  enum class AddressMode {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
  };

  struct Lod {
    static constexpr const Float32 NONE = 1000.0f;
    Float32 min = 0.0f;
    Float32 max = NONE;
    bool operator==(const Lod&) const = default;
    bool operator!=(const Lod&) const = default;
  };

  void record_min_filter(Filter _min_filter);
  void record_mag_filter(Filter _max_filter);

  void record_mipmap_mode(MipmapMode _mipmap_mode);

  void record_address_mode_u(AddressMode _address_mode_u);
  void record_address_mode_v(AddressMode _address_mode_v);
  void record_address_mode_w(AddressMode _address_mode_w);

  void record_mipmap_lod_bias(Float32 _mipmap_lod_bias);

  // Values <= 0 disable anisostropic filtering.
  void record_anisotropy(Float32 _anisotropy);

  void record_lod(const Lod& _lod);

  /// Accessors.
  Filter min_filter() const;
  Filter mag_filter() const;

  MipmapMode mipmap_mode() const;

  AddressMode address_mode_u() const;
  AddressMode address_mode_v() const;
  AddressMode address_mode_w() const;

  Float32 mipmap_lod_bias() const;

  Float32 anisotropy() const;

  Lod lod() const;

  Size flush();
  bool operator==(const Sampler& _sampler) const;

private:
  static constexpr const Size DIRTY_BIT = 1_z << (sizeof(Size) * 8 - 1);

  Filter m_min_filter = Filter::NEAREST;
  Filter m_mag_filter = Filter::NEAREST;

  MipmapMode m_mipmap_mode = MipmapMode::NONE;

  AddressMode m_address_mode_u = AddressMode::REPEAT;
  AddressMode m_address_mode_v = AddressMode::REPEAT;
  AddressMode m_address_mode_w = AddressMode::REPEAT;

  Float32 m_mipmap_lod_bias = 0.0f;

  Float32 m_anisotropy = 0.0f;

  Lod m_lod;

  Size m_hash = DIRTY_BIT;
};

inline void Sampler::record_min_filter(Filter _min_filter) {
  m_min_filter = _min_filter;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_mag_filter(Filter _mag_filter) {
  m_mag_filter = _mag_filter;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_mipmap_mode(MipmapMode _mipmap_mode) {
  m_mipmap_mode = _mipmap_mode;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_address_mode_u(AddressMode _address_mode_u) {
  m_address_mode_u = _address_mode_u;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_address_mode_v(AddressMode _address_mode_v) {
  m_address_mode_v = _address_mode_v;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_address_mode_w(AddressMode _address_mode_w) {
  m_address_mode_w = _address_mode_w;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_mipmap_lod_bias(Float32 _mipmap_lod_bias) {
  m_mipmap_lod_bias = _mipmap_lod_bias;
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_anisotropy(Float32 _anisotropy) {
  m_anisotropy = Algorithm::saturate(_anisotropy);
  m_hash |= DIRTY_BIT;
}

inline void Sampler::record_lod(const Lod& _lod) {
  m_lod = _lod;
  m_hash |= DIRTY_BIT;
}

inline Sampler::Filter Sampler::min_filter() const {
  return m_min_filter;
}

inline Sampler::Filter Sampler::mag_filter() const {
  return m_mag_filter;
}

inline Sampler::MipmapMode Sampler::mipmap_mode() const {
  return m_mipmap_mode;
}

inline Sampler::AddressMode Sampler::address_mode_u() const {
  return m_address_mode_u;
}

inline Sampler::AddressMode Sampler::address_mode_v() const {
  return m_address_mode_v;
}

inline Sampler::AddressMode Sampler::address_mode_w() const {
  return m_address_mode_w;
}

inline Float32 Sampler::mipmap_lod_bias() const {
  return m_mipmap_lod_bias;
}

inline Float32 Sampler::anisotropy() const {
  return m_anisotropy;
}

inline Sampler::Lod Sampler::lod() const {
  return m_lod;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_SAMPLER_H