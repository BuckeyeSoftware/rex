#include <rx/render/state.h>

namespace rx::render {

// scissor_state
scissor_state::scissor_state()
  : m_hash{k_dirty_bit}
  , m_enabled{false}
{
  flush();
}

bool scissor_state::operator==(const scissor_state& _other) const {
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

rx_size scissor_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, hash<math::vec2i>{}(m_offset));
    m_hash = hash_combine(m_hash, hash<math::vec2i>{}(m_size));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// blend_state
blend_state::blend_state()
  : m_hash{k_dirty_bit}
  , m_color_src_factor{factor_type::k_one}
  , m_color_dst_factor{factor_type::k_zero}
  , m_alpha_src_factor{factor_type::k_one}
  , m_alpha_dst_factor{factor_type::k_zero}
  , m_write_mask{k_mask_all}
  , m_enabled{false}
{
  flush();
}

bool blend_state::operator==(const blend_state& _other) const {
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

rx_size blend_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, hash<rx_s32>{}(static_cast<rx_s32>(m_color_src_factor)));
    m_hash = hash_combine(m_hash, hash<rx_s32>{}(static_cast<rx_s32>(m_color_dst_factor)));
    m_hash = hash_combine(m_hash, hash<rx_s32>{}(static_cast<rx_s32>(m_alpha_src_factor)));
    m_hash = hash_combine(m_hash, hash<rx_s32>{}(static_cast<rx_s32>(m_alpha_dst_factor)));
    m_hash = hash_combine(m_hash, hash<rx_s32>{}(static_cast<rx_s32>(m_write_mask)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// depth_state
depth_state::depth_state()
  : m_hash{k_dirty_bit}
  , m_flags{0}
{
  flush();
}

bool depth_state::operator==(const depth_state& _other) const {
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

rx_size depth_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<bool>{}(m_flags & k_test);
    m_hash = hash_combine(m_hash, hash<bool>{}(m_flags & k_write));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// cull_state
cull_state::cull_state()
  : m_hash{k_dirty_bit}
  , m_front_face{front_face_type::k_clock_wise}
  , m_cull_face{cull_face_type::k_back}
  , m_enabled{true}
{
  flush();
}

bool cull_state::operator==(const cull_state& _other) const {
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

rx_size cull_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_front_face)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_cull_face)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// stencil_state
stencil_state::stencil_state()
  : m_hash{k_dirty_bit}
  , m_write_mask{0xff}
  , m_function{function_type::k_always}
  , m_reference{0x00}
  , m_mask{0xff}
  , m_front_fail_action{operation_type::k_keep}
  , m_front_depth_fail_action{operation_type::k_keep}
  , m_front_depth_pass_action{operation_type::k_keep}
  , m_back_fail_action{operation_type::k_keep}
  , m_back_depth_fail_action{operation_type::k_keep}
  , m_back_depth_pass_action{operation_type::k_keep}
  , m_enabled{false}
{
  flush();
}

bool stencil_state::operator==(const stencil_state& _other) const {
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

rx_size stencil_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<bool>{}(m_enabled);
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_write_mask)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_function)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_reference)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_mask)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_front_fail_action)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_front_depth_fail_action)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_front_depth_pass_action)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_back_fail_action)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_back_depth_fail_action)));
    m_hash = hash_combine(m_hash, hash<rx_u32>{}(static_cast<rx_u32>(m_back_depth_pass_action)));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

// polygon_state
polygon_state::polygon_state()
  : m_hash{k_dirty_bit}
  , m_mode{mode_type::k_fill}
{
  flush();
}

bool polygon_state::operator==(const polygon_state& _other) const {
  if (m_hash != _other.m_hash) {
    return false;
  }

  if (m_mode != _other.m_mode) {
    return false;
  }

  return true;
}

rx_size polygon_state::flush() {
  if (m_hash & k_dirty_bit) {
    m_hash = hash<rx_u32>{}(static_cast<rx_u32>(m_mode));
    m_hash &= ~k_dirty_bit;
  }
  return m_hash;
}

} // namespace rx::render
