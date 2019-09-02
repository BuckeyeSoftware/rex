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

  const vector<vertex>& vertices() const &;
  vector<vertex>&& vertices() &&;
  const vector<animated_vertex> animated_vertices() const &;
  vector<animated_vertex>&& animated_vertices() &&;
  const vector<mesh>& meshes() const &;
  const vector<rx_u32>& elements() const &;
  vector<rx_u32>&& elements() &&;
  const vector<loader::joint>& joints() const &;

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
    vector<vertex> as_vertices;
    vector<animated_vertex> as_animated_vertices;
  };

  vector<rx_u32> m_elements;
  vector<mesh> m_meshes;
  vector<loader::animation> m_animations;
  vector<math::mat3x4f> m_frames;
  vector<loader::joint> m_joints;
};

inline constexpr interface::interface(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_flags{0}
  , as_nat{}
  , m_elements{m_allocator}
  , m_joints{m_allocator}
{
}

inline bool interface::is_animated() const {
  return m_flags & k_animated;
}

inline const vector<interface::vertex>& interface::vertices() const & {
  RX_ASSERT(!is_animated(), "not a static model");
  return as_vertices;
}

inline vector<interface::vertex>&& interface::vertices() && {
  RX_ASSERT(!is_animated(), "not a static model");
  return utility::move(as_vertices);
}

inline const vector<interface::animated_vertex> interface::animated_vertices() const & {
  RX_ASSERT(is_animated(), "not a animated model");
  return as_animated_vertices;
}

inline vector<interface::animated_vertex>&& interface::animated_vertices() && {
  RX_ASSERT(is_animated(), "not a animated model");
  return utility::move(as_animated_vertices);
}

inline const vector<mesh>& interface::meshes() const & {
  return m_meshes;
}

inline const vector<rx_u32>& interface::elements() const & {
  return m_elements;
}

inline vector<rx_u32>&& interface::elements() && {
  return utility::move(m_elements);
}

inline const vector<loader::joint>& interface::joints() const & {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_INTERFACE_H