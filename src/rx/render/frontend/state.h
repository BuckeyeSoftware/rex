#ifndef RX_RENDER_FRONTEND_STATE_H
#define RX_RENDER_FRONTEND_STATE_H
#include "rx/math/vec2.h"

namespace Rx::Render::Frontend {

struct ScissorState {
  ScissorState();

  void record_enable(bool _enable);
  void record_offset(const Math::Vec2i& _offset);
  void record_size(const Math::Vec2i& _size);

  bool enabled() const;
  const Math::Vec2i& offset() const;
  const Math::Vec2i& size() const;

  bool operator!=(const ScissorState& _other) const;
  bool operator==(const ScissorState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  Math::Vec2i m_offset;
  Math::Vec2i m_size;

  bool m_enabled;
};

struct BlendState {
  enum class FactorType : Uint8 {
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

  static constexpr const Uint8 k_mask_all{(1 << 4) - 1};

  BlendState();

  void record_enable(bool _enable);
  void record_blend_factors(FactorType _src, FactorType _dst);
  void record_color_blend_factors(FactorType _src, FactorType _dst);
  void record_alpha_blend_factors(FactorType _src, FactorType _dst);
  void record_write_mask(Uint8 _write_mask);

  bool enabled() const;
  FactorType color_src_factor() const;
  FactorType color_dst_factor() const;
  FactorType alpha_src_factor() const;
  FactorType alpha_dst_factor() const;
  Uint8 write_mask() const;

  bool operator!=(const BlendState& _other) const;
  bool operator==(const BlendState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  FactorType m_color_src_factor;
  FactorType m_color_dst_factor;
  FactorType m_alpha_src_factor;
  FactorType m_alpha_dst_factor;
  Uint8 m_write_mask;

  bool m_enabled;
};

struct DepthState {
  DepthState();

  void record_test(bool _test);
  void record_write(bool _write);

  bool test() const;
  bool write() const;

  bool operator!=(const DepthState& _other) const;
  bool operator==(const DepthState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  enum {
    k_test = 1 << 0,
    k_write = 1 << 1
  };

  Uint8 m_flags;
};

struct CullState {
  enum class FrontFaceType : Uint8 {
    k_clock_wise,
    k_counter_clock_wise
  };

  enum class CullFaceType : Uint8 {
    k_front,
    k_back
  };

  CullState();

  void record_enable(bool _enable);
  void record_front_face(FrontFaceType _front_face);
  void record_cull_face(CullFaceType _cull_face);

  bool enabled() const;
  FrontFaceType front_face() const;
  CullFaceType cull_face() const;

  bool operator!=(const CullState& _other) const;
  bool operator==(const CullState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  FrontFaceType m_front_face;
  CullFaceType m_cull_face;
  bool m_enabled;
};

struct StencilState {
  enum class FunctionType : Uint8 {
    k_never,
    k_less,
    k_less_equal,
    k_greater,
    k_greater_equal,
    k_equal,
    k_not_equal,
    k_always
  };

  enum class OperationType : Uint8 {
    k_keep,
    k_zero,
    k_replace,
    k_increment,
    k_increment_wrap,
    k_decrement,
    k_decrement_wrap,
    k_invert
  };

  StencilState();

  void record_enable(bool _enable);
  void record_write_mask(Uint8 _write_mask);
  void record_function(FunctionType _function);
  void record_reference(Uint8 _reference);
  void record_mask(Uint8 _mask);

  void record_fail_action(OperationType _action);
  void record_depth_fail_action(OperationType _action);
  void record_depth_pass_action(OperationType _action);

  void record_front_fail_action(OperationType _action);
  void record_front_depth_fail_action(OperationType _action);
  void record_front_depth_pass_action(OperationType _action);

  void record_back_fail_action(OperationType _action);
  void record_back_depth_fail_action(OperationType _action);
  void record_back_depth_pass_action(OperationType _action);

  bool enabled() const;
  Uint8 write_mask() const;
  FunctionType function() const;
  Uint8 reference() const;
  Uint8 mask() const;
  OperationType front_fail_action() const;
  OperationType front_depth_fail_action() const;
  OperationType front_depth_pass_action() const;
  OperationType back_fail_action() const;
  OperationType back_depth_fail_action() const;
  OperationType back_depth_pass_action() const;

  bool operator!=(const StencilState& _other) const;
  bool operator==(const StencilState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  Uint8 m_write_mask;
  FunctionType m_function;
  Uint8 m_reference;
  Uint8 m_mask;
  OperationType m_front_fail_action;
  OperationType m_front_depth_fail_action;
  OperationType m_front_depth_pass_action;
  OperationType m_back_fail_action;
  OperationType m_back_depth_fail_action;
  OperationType m_back_depth_pass_action;
  bool m_enabled;
};

struct PolygonState {
  PolygonState();

  enum class ModeType : Uint8 {
    k_point,
    k_line,
    k_fill
  };

  void record_mode(ModeType _mode);

  ModeType mode() const;

  bool operator!=(const PolygonState& _other) const;
  bool operator==(const PolygonState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  ModeType m_mode;
};

struct ViewportState {
  ViewportState();

  void record_offset(const Math::Vec2i& _offset);
  void record_dimensions(const Math::Vec2z& _dimensions);

  const Math::Vec2i& offset() const &;
  const Math::Vec2z& dimensions() const &;

  bool operator!=(const ViewportState& _other) const;
  bool operator==(const ViewportState& _other) const;

  Size flush();

private:
  static constexpr Size k_dirty_bit{1_z << (sizeof(Size) * 8 - 1)};
  Size m_hash;

  Math::Vec2i m_offset;
  Math::Vec2z m_dimensions;
};


struct State {
  ScissorState scissor;
  BlendState blend;
  DepthState depth;
  CullState cull;
  StencilState stencil;
  PolygonState polygon;
  ViewportState viewport;

  void flush();

  bool operator==(const State& _state) const;
  bool operator!=(const State& _state) const;

private:
  Size m_hash;
};

// scissor_state
inline void ScissorState::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void ScissorState::record_offset(const Math::Vec2i& _offset) {
  m_offset = _offset;
  m_hash |= k_dirty_bit;
}

inline void ScissorState::record_size(const Math::Vec2i& _size) {
  m_size = _size;
  m_hash = k_dirty_bit;
}

inline bool ScissorState::enabled() const {
  return m_enabled;
}

inline const Math::Vec2i& ScissorState::offset() const {
  return m_offset;
}

inline const Math::Vec2i& ScissorState::size() const {
  return m_size;
}

inline bool ScissorState::operator!=(const ScissorState& _other) const {
  return !operator==(_other);
}

// blend_state
inline void BlendState::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void BlendState::record_blend_factors(FactorType _src, FactorType _dst) {
  record_color_blend_factors(_src, _dst);
  record_alpha_blend_factors(_src, _dst);
}

inline void BlendState::record_color_blend_factors(FactorType _src, FactorType _dst) {
  m_color_src_factor = _src;
  m_color_dst_factor = _dst;
  m_hash |= k_dirty_bit;
}

inline void BlendState::record_alpha_blend_factors(FactorType _src, FactorType _dst) {
  m_alpha_src_factor = _src;
  m_alpha_dst_factor = _dst;
  m_hash |= k_dirty_bit;
}

inline void BlendState::record_write_mask(Uint8 _write_mask) {
  m_write_mask = _write_mask;
  m_hash |= k_dirty_bit;
}

inline bool BlendState::enabled() const {
  return m_enabled;
}

inline BlendState::FactorType BlendState::color_src_factor() const {
  return m_color_src_factor;
}

inline BlendState::FactorType BlendState::color_dst_factor() const {
  return m_color_dst_factor;
}

inline BlendState::FactorType BlendState::alpha_src_factor() const {
  return m_alpha_src_factor;
}

inline BlendState::FactorType BlendState::alpha_dst_factor() const {
  return m_alpha_dst_factor;
}

inline Uint8 BlendState::write_mask() const {
  return m_write_mask;
}

inline bool BlendState::operator!=(const BlendState& _other) const {
  return !operator==(_other);
}

// depth_state
inline void DepthState::record_test(bool _test) {
  if (_test) {
    m_flags |= k_test;
  } else {
    m_flags &= ~k_test;
  }
  m_hash |= k_dirty_bit;
}

inline void DepthState::record_write(bool _write) {
  if (_write) {
    m_flags |= k_write;
  } else {
    m_flags &= ~k_write;
  }
  m_hash |= k_dirty_bit;
}

inline bool DepthState::test() const {
  return m_flags & k_test;
}

inline bool DepthState::write() const {
  return m_flags & k_write;
}

inline bool DepthState::operator!=(const DepthState& _other) const {
  return !operator==(_other);
}

// cull_state
inline void CullState::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void CullState::record_front_face(FrontFaceType _front_face) {
  m_front_face = _front_face;
  m_hash |= k_dirty_bit;
}

inline void CullState::record_cull_face(CullFaceType _cull_face) {
  m_cull_face = _cull_face;
  m_hash |= k_dirty_bit;
}

inline bool CullState::enabled() const {
  return m_enabled;
}

inline CullState::FrontFaceType CullState::front_face() const {
  return m_front_face;
}

inline CullState::CullFaceType CullState::cull_face() const {
  return m_cull_face;
}

inline bool CullState::operator!=(const CullState& _other) const {
  return !operator==(_other);
}

// stencil_state
inline void StencilState::record_enable(bool _enable) {
  m_enabled = _enable;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_write_mask(Uint8 _write_mask) {
  m_write_mask = _write_mask;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_function(FunctionType _function) {
  m_function = _function;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_reference(Uint8 _reference) {
  m_reference = _reference;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_mask(Uint8 _mask) {
  m_mask = _mask;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_fail_action(OperationType _action) {
  record_front_fail_action(_action);
  record_back_fail_action(_action);
}

inline void StencilState::record_depth_fail_action(OperationType _action) {
  record_front_depth_fail_action(_action);
  record_back_depth_fail_action(_action);
}

inline void StencilState::record_depth_pass_action(OperationType _action) {
  record_front_depth_pass_action(_action);
  record_back_depth_pass_action(_action);
}

inline void StencilState::record_front_fail_action(OperationType _action) {
  m_front_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_front_depth_fail_action(OperationType _action) {
  m_front_depth_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_front_depth_pass_action(OperationType _action) {
  m_front_depth_pass_action = _action;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_back_fail_action(OperationType _action) {
  m_back_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_back_depth_fail_action(OperationType _action) {
  m_back_depth_fail_action = _action;
  m_hash |= k_dirty_bit;
}

inline void StencilState::record_back_depth_pass_action(OperationType _action) {
  m_back_depth_pass_action = _action;
  m_hash |= k_dirty_bit;
}

inline bool StencilState::enabled() const {
  return m_enabled;
}

inline Uint8 StencilState::write_mask() const {
  return m_write_mask;
}

inline StencilState::FunctionType StencilState::function() const {
  return m_function;
}

inline Uint8 StencilState::reference() const {
  return m_reference;
}

inline Uint8 StencilState::mask() const {
  return m_mask;
}

inline StencilState::OperationType StencilState::front_fail_action() const {
  return m_front_fail_action;
}

inline StencilState::OperationType StencilState::front_depth_fail_action() const {
  return m_front_depth_fail_action;
}

inline StencilState::OperationType StencilState::front_depth_pass_action() const {
  return m_front_depth_pass_action;
}

inline StencilState::OperationType StencilState::back_fail_action() const {
  return m_back_fail_action;
}

inline StencilState::OperationType StencilState::back_depth_fail_action() const {
  return m_back_depth_fail_action;
}

inline StencilState::OperationType StencilState::back_depth_pass_action() const {
  return m_back_depth_pass_action;
}

inline bool StencilState::operator!=(const StencilState& _other) const {
  return !operator==(_other);
}

// polygon_state
inline void PolygonState::record_mode(ModeType _mode) {
  m_mode = _mode;
  m_hash |= k_dirty_bit;
}

inline PolygonState::ModeType PolygonState::mode() const {
  return m_mode;
}

inline bool PolygonState::operator!=(const PolygonState& _other) const {
  return !operator==(_other);
}

// viewport_state
inline void ViewportState::record_offset(const Math::Vec2i& _offset) {
  m_offset = _offset;
  m_hash |= k_dirty_bit;
}

inline void ViewportState::record_dimensions(const Math::Vec2z& _dimensions) {
  m_dimensions = _dimensions;
  m_hash |= k_dirty_bit;
}

inline const Math::Vec2i& ViewportState::offset() const & {
  return m_offset;
}

inline const Math::Vec2z& ViewportState::dimensions() const & {
  return m_dimensions;
}

inline bool ViewportState::operator!=(const ViewportState& _other) const {
  return !operator==(_other);
}

inline bool ViewportState::operator==(const ViewportState& _other) const {
  return m_dimensions == _other.m_dimensions && m_offset == _other.m_offset;
}

// state
inline bool State::operator!=(const State& _state) const {
  return !operator==(_state);
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_STATE_H
