#include "rx/render/frontend/sampler.h"

#include "rx/core/hash/mix_enum.h"
#include "rx/core/hash/mix_float.h"

#include "rx/core/hash/combine.h"

namespace Rx::Render::Frontend {

Size Sampler::flush() {
  if (!(m_hash & DIRTY_BIT)) {
    return m_hash;
  }

  m_hash =                       Hash::mix_enum(m_min_filter);
  m_hash = Hash::combine(m_hash, Hash::mix_enum(m_mag_filter));

  m_hash = Hash::combine(m_hash, Hash::mix_enum(m_mipmap_mode));

  m_hash = Hash::combine(m_hash, Hash::mix_enum(m_address_mode_u));
  m_hash = Hash::combine(m_hash, Hash::mix_enum(m_address_mode_v));
  m_hash = Hash::combine(m_hash, Hash::mix_enum(m_address_mode_w));

  m_hash = Hash::combine(m_hash, Hash::mix_float(m_mipmap_lod_bias));

  m_hash = Hash::combine(m_hash, Hash::mix_float(m_anisotropy));

  m_hash = Hash::combine(m_hash, Hash::mix_float(m_lod.min));
  m_hash = Hash::combine(m_hash, Hash::mix_float(m_lod.max));

  m_hash &= ~DIRTY_BIT;

  return m_hash;
}

bool Sampler::operator==(const Sampler& _sampler) const {
  if (m_hash != _sampler.m_hash) {
    return false;
  }

  if (m_min_filter != _sampler.m_min_filter) {
    return false;
  }

  if (m_mag_filter != _sampler.m_mag_filter) {
    return false;
  }

  if (m_mipmap_mode != _sampler.m_mipmap_mode) {
    return false;
  }

  if (m_address_mode_u != _sampler.m_address_mode_u) {
    return false;
  }

  if (m_address_mode_v != _sampler.m_address_mode_v) {
    return false;
  }

  if (m_address_mode_w != _sampler.m_address_mode_w) {
    return false;
  }

  if (m_mipmap_lod_bias != _sampler.m_mipmap_lod_bias) {
    return false;
  }

  if (m_anisotropy != _sampler.m_anisotropy) {
    return false;
  }

  if (m_lod != _sampler.m_lod) {
    return false;
  }

  return true;
}

} // namespace Rx::Render::Frontend