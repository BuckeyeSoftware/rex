#ifndef RX_MODEL_MODEL_H
#define RX_MODEL_MODEL_H

#include <rx/math/vec2.h>
#include <rx/math/vec3.h>
#include <rx/math/vec4.h>

#include <rx/core/utility/nat.h>
#include <rx/core/array.h>
#include <rx/core/string.h>

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
    math::vec4b blend_weight;
    math::vec4b blend_index;
  };

  bool is_animated() const;

  const array<vertex>& vertices() const &;
  const array<animated_vertex> animated_vertices() const &;
  const array<rx_u32>& elements() const &;

private:
  memory::allocator* m_allocator;

  bool m_is_animated;

  union {
    utility::nat as_nat;
    array<vertex> as_vertices;
    array<animated_vertex> as_animated_vertices;
  };
  array<rx_u32> m_elements;
};

inline constexpr model::model(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_is_animated{false}
  , as_nat{}
  , m_elements{m_allocator}
{
}

inline bool model::is_animated() const {
  return m_is_animated;
}

inline const array<model::vertex>& model::vertices() const & {
  return as_vertices;
}

inline const array<model::animated_vertex> model::animated_vertices() const & {
  return as_animated_vertices;
}

inline const array<rx_u32>& model::elements() const & {
  return m_elements;
}

} // namespace rx::model

#endif // RX_MODEL_MODEL_H