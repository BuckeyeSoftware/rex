#include "rx/render/frontend/state.h"

namespace Rx::Render::Frontend {

// scissor_state
ScissorState::ScissorState()
  : m_hash{k_dirty_bit}
  , m_enabled{false}
{
  flush();
}

bool ScissorState::operator==(const ScissorState& _other) const {
  RX_ASSERT(!(m_hash & k_dirty_bit), "not flushed");
  RX_ASSERT(!(_other.m_hash & k_dirty_bit), "not flushed");

  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_enabled != _other.m_enabled) {
    return false;
  }

  if (m_offset != _other.m_offset) {
    return false;
  }

  if (m_size != _other.m_size) {
    return false;
  }

  return true;
}

Size ScissorState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, Hash<Math::Vec2i>{}(m_offset));
    m_hash = hash_combine(m_hash, Hash<Math::Vec2i>{}(m_size));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// blend_state
BlendState::BlendState()
  : m_hash{k_dirty_bit}
  , m_color_src_factor{FactorType::k_one}
  , m_color_dst_factor{FactorType::k_zero}
  , m_alpha_src_factor{FactorType::k_one}
  , m_alpha_dst_factor{FactorType::k_zero}
  , m_write_mask{k_mask_all}
  , m_enabled{false}
{
  flush();
}

bool BlendState::operator==(const BlendState& _other) const {
  RX_ASSERT(!(m_hash & k_dirty_bit), "not flushed");
  RX_ASSERT(!(_other.m_hash & k_dirty_bit), "not flushed");

  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_enabled != _other.m_enabled) {
    return false;
  }

  if (m_color_src_factor != _other.m_color_src_factor) {
    return false;
  }

  if (m_color_dst_factor != _other.m_color_dst_factor) {
    return false;
  }

  if (m_alpha_src_factor != _other.m_alpha_src_factor) {
    return false;
  }

  if (m_write_mask != _other.m_write_mask) {
    return false;
  }

  return true;
}

Size BlendState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, Hash<Sint32>{}(static_cast<Sint32>(m_color_src_factor)));
    m_hash = hash_combine(m_hash, Hash<Sint32>{}(static_cast<Sint32>(m_color_dst_factor)));
    m_hash = hash_combine(m_hash, Hash<Sint32>{}(static_cast<Sint32>(m_alpha_src_factor)));
    m_hash = hash_combine(m_hash, Hash<Sint32>{}(static_cast<Sint32>(m_alpha_dst_factor)));
    m_hash = hash_combine(m_hash, Hash<Sint32>{}(static_cast<Sint32>(m_write_mask)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// depth_state
DepthState::DepthState()
  : m_hash{k_dirty_bit}
  , m_flags{0}
{
  flush();
}

bool DepthState::operator==(const DepthState& _other) const {
  RX_ASSERT(!(m_hash & k_dirty_bit), "not flushed");
  RX_ASSERT(!(_other.m_hash & k_dirty_bit), "not flushed");

  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_flags != _other.m_flags) {
    return false;
  }

  return true;
}

Size DepthState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<bool>{}(m_flags & k_test);
    m_hash = hash_combine(m_hash, Hash<bool>{}(m_flags & k_write));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// cull_state
CullState::CullState()
  : m_hash{k_dirty_bit}
  , m_front_face{FrontFaceType::k_clock_wise}
  , m_cull_face{CullFaceType::k_back}
  , m_enabled{true}
{
  flush();
}

bool CullState::operator==(const CullState& _other) const {
  RX_ASSERT(!(m_hash & k_dirty_bit), "not flushed");
  RX_ASSERT(!(_other.m_hash & k_dirty_bit), "not flushed");

  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_enabled != _other.m_enabled) {
    return false;
  }

  if (m_front_face != _other.m_front_face) {
    return false;
  }

  if (m_cull_face != _other.m_cull_face) {
    return false;
  }

  return true;
}

Size CullState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_front_face)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_cull_face)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// stencil_state
StencilState::StencilState()
  : m_hash{k_dirty_bit}
  , m_write_mask{0xff}
  , m_function{FunctionType::k_always}
  , m_reference{0x00}
  , m_mask{0xff}
  , m_front_fail_action{OperationType::k_keep}
  , m_front_depth_fail_action{OperationType::k_keep}
  , m_front_depth_pass_action{OperationType::k_keep}
  , m_back_fail_action{OperationType::k_keep}
  , m_back_depth_fail_action{OperationType::k_keep}
  , m_back_depth_pass_action{OperationType::k_keep}
  , m_enabled{false}
{
  flush();
}

bool StencilState::operator==(const StencilState& _other) const {
  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_enabled != _other.m_enabled) {
    return false;
  }

  if (m_function != _other.m_function) {
    return false;
  }

  if (m_reference != _other.m_reference) {
    return false;
  }

  if (m_mask != _other.m_mask) {
    return false;
  }

  if (m_front_fail_action != _other.m_front_fail_action) {
    return false;
  }

  if (m_front_depth_fail_action != _other.m_front_depth_fail_action) {
    return false;
  }

  if (m_front_depth_pass_action != _other.m_front_depth_pass_action) {
    return false;
  }

  if (m_back_fail_action != _other.m_back_fail_action) {
    return false;
  }

  if (m_back_depth_fail_action != _other.m_back_depth_fail_action) {
    return false;
  }

  if (m_back_depth_pass_action != _other.m_back_depth_pass_action) {
    return false;
  }

  return true;
}

Size StencilState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_write_mask)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_function)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_reference)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_mask)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_front_fail_action)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_front_depth_fail_action)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_front_depth_pass_action)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_back_fail_action)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_back_depth_fail_action)));
    m_hash = hash_combine(m_hash, Hash<Uint32>{}(static_cast<Uint32>(m_back_depth_pass_action)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// polygon_state
PolygonState::PolygonState()
  : m_hash{k_dirty_bit}
  , m_mode{ModeType::k_fill}
{
  flush();
}

bool PolygonState::operator==(const PolygonState& _other) const {
  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_mode != _other.m_mode) {
    return false;
  }

  return true;
}

Size PolygonState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = Hash<Uint32>{}(static_cast<Uint32>(m_mode));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// viewport_state
ViewportState::ViewportState()
  : m_hash{k_dirty_bit}
{
  flush();
}

Size ViewportState::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash_combine(m_hash, Hash<Math::Vec2i>{}(m_offset));
    m_hash = hash_combine(m_hash, Hash<Math::Vec2z>{}(m_dimensions));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// state
void State::flush() {
  m_hash = scissor.flush();
  m_hash = hash_combine(m_hash, blend.flush());
  m_hash = hash_combine(m_hash, depth.flush());
  m_hash = hash_combine(m_hash, cull.flush());
  m_hash = hash_combine(m_hash, stencil.flush());
  m_hash = hash_combine(m_hash, polygon.flush());
  m_hash = hash_combine(m_hash, viewport.flush());
}

bool State::operator==(const State& _state) const {
  // The specific order of these comparisons are finely tuned for the quickest
  // early-out based on two criterons.
  //
  // 1) Smaller and easier to compare sub states are compared first as they take
  //    less time to compare than larger and harder sub states.
  //
  // 2) More frequently changing state is compared first. Something like
  //    PolygonState is statistically less likely to change in a renderer than
  //    say DepthState as an example.
  //
  // The hash - which represents a really crude bloom filter, is always compared
  // first as it's a simple integer comparison.
  if (_state.m_hash != m_hash) {
    return false;
  }

  if (_state.cull != cull) {
    return false;
  }

  if (_state.depth != depth) {
    return false;
  }

  if (_state.blend != blend) {
    return false;
  }

  if (_state.polygon != polygon) {
    return false;
  }

  if (_state.stencil != stencil) {
    return false;
  }

  if (_state.viewport != viewport) {
    return false;
  }

  if (_state.scissor != scissor) {
    return false;
  }

  return true;
}

} // namespace Rx::Render::Frontend
