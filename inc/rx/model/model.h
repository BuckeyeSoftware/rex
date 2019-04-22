#ifndef RX_MODEL_MODEL_H
#define RX_MODEL_MODEL_H

#include <rx/model/loader.h>

namespace rx::model {

struct model {
  constexpr model(memory::allocator* _allocator);
  ~model();

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
  const array<rx_u32>& elements() const &;
  array<rx_u32>&& elements() &&;
  rx_size joints() const;

private:
  friend struct animation;

  memory::allocator* m_allocator;

  bool m_is_animated;

  union {
    utility::nat as_nat;
    array<vertex> as_vertices;
    array<animated_vertex> as_animated_vertices;
  };
  array<rx_u32> m_elements;
  array<loader::animation> m_animations;
  array<math::mat3x4f> m_frames;
  rx_size m_joints;
};

inline constexpr model::model(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_is_animated{false}
  , as_nat{}
  , m_elements{m_allocator}
  , m_joints{0}
{
}

inline bool model::is_animated() const {
  return m_is_animated;
}

inline const array<model::vertex>& model::vertices() const & {
  RX_ASSERT(!m_is_animated, "not a static model");
  return as_vertices;
}

inline array<model::vertex>&& model::vertices() && {
  return utility::move(as_vertices);
}

inline const array<model::animated_vertex> model::animated_vertices() const & {
  RX_ASSERT(m_is_animated, "not a animated model");
  return as_animated_vertices;
}

inline array<model::animated_vertex>&& model::animated_vertices() && {
  RX_ASSERT(m_is_animated, "not a animated model");
  return utility::move(as_animated_vertices);
}

inline const array<rx_u32>& model::elements() const & {
  return m_elements;
}

inline array<rx_u32>&& model::elements() && {
  return utility::move(m_elements);
}

inline rx_size model::joints() const {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_MODEL_H