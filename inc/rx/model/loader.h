#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H

#include <rx/core/concepts/interface.h>
#include <rx/core/array.h>
#include <rx/core/string.h>
#include <rx/core/json.h>

#include <rx/math/vec2.h>
#include <rx/math/vec3.h>

#include <rx/math/mat3x4.h>

namespace rx::model {

struct loader : concepts::interface {
  loader(memory::allocator* _allocator);

  bool load(const string& _file_name);

  // implemented by each model loader
  virtual bool read(const array<rx_byte>& _data) = 0;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  const array<rx_u32> elements() const & { return m_elements; }
  const array<math::vec3f> positions() const & { return m_positions; }

protected:
  void generate_normals();
  void generate_tangents();

  memory::allocator* m_allocator;
  string m_error;
  array<rx_u32> m_elements;
  array<math::vec3f> m_positions;
  array<math::vec2f> m_coordinates;
  array<math::vec3f> m_normals;
  array<math::vec4f> m_tangents; // w = bitangent sign
  array<math::vec4b> m_blend_indices;
  array<math::vec4b> m_blend_weights;

  array<math::mat3x4f> m_generic_base_frame;
  array<math::mat3x4f> m_inverse_base_frame;

  json m_materials;
};

template<typename... Ts>
inline bool loader::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H