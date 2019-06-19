#ifndef RX_MODEL_INTERFACE_H
#define RX_MODEL_INTERFACE_H

#include "rx/model/loader.h"

namespace rx::model {

struct interface {
  constexpr interface(memory::allocator* _allocator);
  ~interface();

  bool load(const string& _file_name);

  struct vertex {
    math::vec3f position;
    math::vec3f normal;
    math::vec4f tangent;
    math::vec2f coordinate;
  };

  struct animated_vertex {
    math::vec3f position;
    math::vec3f normal;
    math::vec4f tangent;
    math::vec2f coordinate;
    math::vec4b blend_weights;
    math::vec4b blend_indices;
  };

  bool is_animated() const;

  const array<vertex>& vertices() const &;
  array<vertex>&& vertices() &&;
  const array<animated_vertex> animated_vertices() const &;
  array<animated_vertex>&& animated_vertices() &&;
  const array<mesh>& meshes() const &;
  const array<rx_u32>& elements() const &;
  array<rx_u32>&& elements() &&;
  rx_size joints() const;

private:
  friend struct animation;

  memory::allocator* m_allocator;

  enum /* m_flags */ {
    k_constructed = 1 << 0,
    k_animated = 1 << 1
  };

  int m_flags;

  union {
    utility::nat as_nat;
    array<vertex> as_vertices;
    array<animated_vertex> as_animated_vertices;
  };

  array<rx_u32> m_elements;
  array<mesh> m_meshes;
  array<loader::animation> m_animations;
  array<math::mat3x4f> m_frames;
  rx_size m_joints;
};

inline constexpr interface::interface(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_flags{0}
  , as_nat{}
  , m_elements{m_allocator}
  , m_joints{0}
{
}

inline bool interface::is_animated() const {
  return m_flags & k_animated;
}

inline const array<interface::vertex>& interface::vertices() const & {
  RX_ASSERT(!is_animated(), "not a static model");
  return as_vertices;
}

inline array<interface::vertex>&& interface::vertices() && {
  RX_ASSERT(!is_animated(), "not a static model");
  return utility::move(as_vertices);
}

inline const array<interface::animated_vertex> interface::animated_vertices() const & {
  RX_ASSERT(is_animated(), "not a animated model");
  return as_animated_vertices;
}

inline array<interface::animated_vertex>&& interface::animated_vertices() && {
  RX_ASSERT(is_animated(), "not a animated model");
  return utility::move(as_animated_vertices);
}

inline const array<mesh>& interface::meshes() const & {
  return m_meshes;
}

inline const array<rx_u32>& interface::elements() const & {
  return m_elements;
}

inline array<rx_u32>&& interface::elements() && {
  return utility::move(m_elements);
}

inline rx_size interface::joints() const {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_INTERFACE_H