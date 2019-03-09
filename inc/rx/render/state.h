#ifndef RX_RENDER_STATE_H
#define RX_RENDER_STATE_H

#include <rx/math/vec2.h>

namespace rx::render {

struct scissor_state {
  scissor_state();

  void record_enable(bool _enable);
  void record_offset(const math::vec2i& _offset);
  void record_size(const math::vec2i& _size);

  bool enabled() const;
  const math::vec2i& offset() const;
  const math::vec2i& size() const;

  bool operator!=(const scissor_state& _other) const;
  bool operator==(const scissor_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  math::vec2i m_offset;
  math::vec2i m_size;

  bool m_enabled;
};

struct blend_state {
  enum class factor_type : rx_u8 {
    k_zero,
    k_one,
    k_src_color,
    k_one_minus_src_color,
    k_dst_color,
    k_one_minus_dst_color,
    k_src_alpha,
    k_one_minus_src_alpha,
    k_dst_alpha,
    k_one_minus_dst_alpha,
    k_constant_color,
    k_one_minus_constant_color,
    k_constant_alpha,
    k_one_minus_constant_alpha,
    k_src_alpha_saturate
  };

  static constexpr const rx_u8 k_mask_all{(1 << 4)-1};

  blend_state();

  void record_blend_factors(factor_type _src, factor_type _dst);
  void record_color_blend_factors(factor_type _src, factor_type _dst);
  void record_alpha_blend_factors(factor_type _src, factor_type _dst);
  void record_write_mask(rx_u8 _write_mask);

  factor_type color_src_factor() const;
  factor_type color_dst_factor() const;
  factor_type alpha_src_factor() const;
  factor_type alpha_dst_factor() const;
  rx_u8 write_mask() const;

  bool operator!=(const blend_state& _other) const;
  bool operator==(const blend_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  factor_type m_color_src_factor;
  factor_type m_color_dst_factor;
  factor_type m_alpha_src_factor;
  factor_type m_alpha_dst_factor;
  rx_u8 m_write_mask;

  bool m_enabled;
};

struct depth_state {
  depth_state();

  void record_test(bool _test);
  void record_write(bool _write);

  bool test() const;
  bool write() const;

  bool operator!=(const depth_state& _other) const;
  bool operator==(const depth_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  enum {
    k_test = 1 << 0,
    k_write = 1 << 1
  };

  rx_u8 m_flags;
};

struct cull_state {
  enum class front_face_type : rx_u8 {
    k_clock_wise,
    k_counter_clock_wise
  };

  enum class cull_face_type : rx_u8 {
    k_front,
    k_back
  };

  cull_state();

  void record_enable(bool _enable);
  void record_front_face(front_face_type _front_face);
  void record_cull_face(cull_face_type _cull_face);

  bool enabled() const;
  front_face_type front_face() const;
  cull_face_type cull_face() const;

  bool operator!=(const cull_state& _other) const;
  bool operator==(const cull_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  front_face_type m_front_face;
  cull_face_type m_cull_face;
  bool m_enabled;
};

struct stencil_state {
  enum class function_type : rx_u8 {
    k_never,
    k_less,
    k_less_equal,
    k_greater,
    k_greater_equal,
    k_equal,
    k_not_equal,
    k_always
  };

  enum class operation_type : rx_u8 {
    k_keep,
    k_zero,
    k_replace,
    k_increment,
    k_increment_wrap,
    k_decrement,
    k_decrement_wrap,
    k_invert
  };

  stencil_state();

  void record_enable(bool _enable);
  void record_write_mask(rx_u8 _write_mask);
  void record_function(function_type _function);
  void record_reference(rx_u8 _reference);
  void record_mask(rx_u8 _mask);

  void record_fail_action(operation_type _action);
  void record_depth_fail_action(operation_type _action);
  void record_depth_pass_action(operation_type _action);

  void record_front_fail_action(operation_type _action);
  void record_front_depth_fail_action(operation_type _action);
  void record_front_depth_pass_action(operation_type _action);

  void record_back_fail_action(operation_type _action);
  void record_back_depth_fail_action(operation_type _action);
  void record_back_depth_pass_action(operation_type _action);

  bool enabled() const;
  rx_u8 write_mask() const;
  function_type function() const;
  rx_u8 reference() const;
  rx_u8 mask() const;
  operation_type front_fail_action() const;
  operation_type front_depth_fail_action() const;
  operation_type front_depth_pass_action() const;
  operation_type back_fail_action() const;
  operation_type back_depth_fail_action() const;
  operation_type back_depth_pass_action() const;

  bool operator!=(const stencil_state& _other) const;
  bool operator==(const stencil_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  rx_u8 m_write_mask;
  function_type m_function;
  rx_u8 m_reference;
  rx_u8 m_mask;
  operation_type m_front_fail_action;
  operation_type m_front_depth_fail_action;
  operation_type m_front_depth_pass_action;
  operation_type m_back_fail_action;
  operation_type m_back_depth_fail_action;
  operation_type m_back_depth_pass_action;
  bool m_enabled;
};

struct polygon_state {
  polygon_state();

  enum class mode_type : rx_u8 {
    k_point,
    k_line,
    k_fill
  };

  void record_mode(mode_type _mode);

  mode_type mode() const;

  bool operator!=(const polygon_state& _other) const;
  bool operator==(const polygon_state& _other) const;

  rx_size flush();

private:
  static constexpr rx_size k_dirty_bit{1_z << (sizeof(rx_size)*CHAR_BIT - 1)};
  rx_size m_hash;

  mode_type m_mode;
};

struct state {
  scissor_state scissor;
  blend_state blend;
  depth_state depth;
  cull_state cull;
  stencil_state stencil;
  polygon_state polygon;

  void flush() {
    m_hash = scissor.flush();
    m_hash = hash_combine(m_hash, blend.flush());
    m_hash = hash_combine(m_hash, depth.flush());
    m_hash = hash_combine(m_hash, cull.flush());
    m_hash = hash_combine(m_hash, stencil.flush());
    m_hash = hash_combine(m_hash, polygon.flush());
  }

private:
  rx_size m_hash;
};

// scissor_state
inline void scissor_state::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void scissor_state::record_offset(const math::vec2i& _offset) {
  m_offset = _offset;
  m_hash |= k_dirty_bit;
}

inline void scissor_state::record_size(const math::vec2i& _size) {
  m_size = _size;
  m_hash = k_dirty_bit;
}

inline bool scissor_state::enabled() const {
  return m_enabled;
}

inline const math::vec2i& scissor_state::offset() const {
  return m_offset;
}

inline const math::vec2i& scissor_state::size() const {
  return m_size;
}

inline bool scissor_state::operator!=(const scissor_state& _other) const {
  return !operator==(_other);
}

// blend_state
inline void blend_state::record_blend_factors(factor_type _src, factor_type _dst) {
  record_color_blend_factors(_src, _dst);
  record_alpha_blend_factors(_src, _dst);
}

inline void blend_state::record_color_blend_factors(factor_type _src, factor_type _dst) {
  m_color_src_factor = _src;
  m_color_dst_factor = _dst;
  m_hash |= k_dirty_bit;
}

inline void blend_state::record_alpha_blend_factors(factor_type _src, factor_type _dst) {
  m_alpha_src_factor = _src;
  m_alpha_dst_factor = _dst;
  m_hash |= k_dirty_bit;
}

inline void blend_state::record_write_mask(rx_u8 _write_mask) {
  m_write_mask = _write_mask;
  m_hash |= k_dirty_bit;
}

inline blend_state::factor_type blend_state::color_src_factor() const {
  return m_color_src_factor;
}

inline blend_state::factor_type blend_state::color_dst_factor() const {
  return m_color_dst_factor;
}

inline blend_state::factor_type blend_state::alpha_src_factor() const {
  return m_alpha_src_factor;
}

inline blend_state::factor_type blend_state::alpha_dst_factor() const {
  return m_alpha_dst_factor;
}

inline rx_u8 blend_state::write_mask() const {
  return m_write_mask;
}

inline bool blend_state::operator!=(const blend_state& _other) const {
  return !operator==(_other);
}

// depth_state
inline void depth_state::record_test(bool _test) {
  if (_test) {
    m_flags |= k_test;
  } else {
    m_flags &= ~k_test;
  }
  m_hash |= k_dirty_bit;
}

inline void depth_state::record_write(bool _write) {
  if (_write) {
    m_flags |= k_write;
  } else {
    m_flags &= ~k_write;
  }
  m_hash |= k_dirty_bit;
}

inline bool depth_state::test() const {
  return m_flags & k_test;
}

inline bool depth_state::write() const {
  return m_flags & k_write;
}

inline bool depth_state::operator!=(const depth_state& _other) const {
  return !operator==(_other);
}

// cull_state
inline void cull_state::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void cull_state::record_front_face(front_face_type _front_face) {
  m_front_face = _front_face;
  m_hash |= k_dirty_bit;
}

inline void cull_state::record_cull_face(cull_face_type _cull_face) {
  m_cull_face = _cull_face;
  m_hash |= k_dirty_bit;
}

inline bool cull_state::enabled() const {
  return m_enabled;
}

inline cull_state::front_face_type cull_state::front_face() const {
  return m_front_face;
}

inline cull_state::cull_face_type cull_state::cull_face() const {
  return m_cull_face;
}

inline bool cull_state::operator!=(const cull_state& _other) const {
  return !operator==(_other);
}

// stencil_state
inline void stencil_state::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_write_mask(rx_u8 _write_mask) {
  m_write_mask = _write_mask;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_function(function_type _function) {
  m_function = _function;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_reference(rx_u8 _reference) {
  m_reference = _reference;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_mask(rx_u8 _mask) {
  m_mask = _mask;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_fail_action(operation_type _action) {
  record_front_fail_action(_action);
  record_back_fail_action(_action);
}

inline void stencil_state::record_depth_fail_action(operation_type _action) {
  record_front_depth_fail_action(_action);
  record_back_depth_fail_action(_action);
}

inline void stencil_state::record_depth_pass_action(operation_type _action) {
  record_front_depth_pass_action(_action);
  record_back_depth_pass_action(_action);
}

inline void stencil_state::record_front_fail_action(operation_type _action) {
  m_front_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_front_depth_fail_action(operation_type _action) {
  m_front_depth_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_front_depth_pass_action(operation_type _action) {
  m_front_depth_pass_action = _action;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_back_fail_action(operation_type _action) {
  m_back_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_back_depth_fail_action(operation_type _action) {
  m_back_depth_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void stencil_state::record_back_depth_pass_action(operation_type _action) {
  m_back_depth_pass_action = _action;
  m_hash |= k_dirty_bit;
}

inline bool stencil_state::enabled() const {
  return m_enabled;
}

inline rx_u8 stencil_state::write_mask() const {
  return m_write_mask;
}

inline stencil_state::function_type stencil_state::function() const {
  return m_function;
}

inline rx_u8 stencil_state::reference() const {
  return m_reference;
}

inline rx_u8 stencil_state::mask() const {
  return m_mask;
}

inline stencil_state::operation_type stencil_state::front_fail_action() const {
  return m_front_fail_action;
}

inline stencil_state::operation_type stencil_state::front_depth_fail_action() const {
  return m_front_depth_fail_action;
}

inline stencil_state::operation_type stencil_state::front_depth_pass_action() const {
  return m_front_depth_pass_action;
}

inline stencil_state::operation_type stencil_state::back_fail_action() const {
  return m_back_fail_action;
}

inline stencil_state::operation_type stencil_state::back_depth_fail_action() const {
  return m_back_depth_fail_action;
}

inline stencil_state::operation_type stencil_state::back_depth_pass_action() const {
  return m_back_depth_pass_action;
}

inline bool stencil_state::operator!=(const stencil_state& _other) const {
  return !operator==(_other);
}

// polygon_state
inline void polygon_state::record_mode(mode_type _mode) {
  m_mode = _mode;
  m_hash |= k_dirty_bit;
}

inline polygon_state::mode_type polygon_state::mode() const {
  return m_mode;
}

inline bool polygon_state::operator!=(const polygon_state& _other) const {
  return !operator==(_other);
}

} // namespace rx::render

#endif
