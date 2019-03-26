#include <string.h> // memcpy

#include <rx/render/program.h>
#include <rx/render/frontend.h>
#include <rx/core/utility/bit.h>

namespace rx::render {
  // checks if |_type| is a sampler type
  static bool is_sampler(uniform::category _type) {
    switch (_type) {
    case uniform::category::k_sampler1D:
      [[fallthrough]];
    case uniform::category::k_sampler2D:
      [[fallthrough]];
    case uniform::category::k_sampler3D:
      [[fallthrough]];
    case uniform::category::k_samplerCM:
      return true;
    default:
      return false;
    }
  }

  uniform::uniform() 
    : m_program{nullptr}
  {
  }

  uniform::uniform(program* _program, rx_size _index, const string& _name, category _type)
    : m_program{_program}
    , m_mask{rx_u64{1} << _index}
    , m_type{_type}
    , m_name{_name}
  {
    as_opaque = m_program->m_allocator->allocate(size());
  }

  uniform::uniform(uniform&& _uniform)
    : m_program{_uniform.m_program}
    , m_mask{_uniform.m_mask}
    , m_type{_uniform.m_type}
    , m_name{utility::move(_uniform.m_name)}
  {
    as_opaque = _uniform.as_opaque;
    _uniform.as_opaque = nullptr;
  }

  uniform::~uniform() {
    m_program->m_allocator->deallocate(as_opaque);
  }

  void uniform::flush(rx_byte* _flush) {
    RX_ASSERT(m_program->m_dirty_uniforms & m_mask, "flush on non-dirty uniform");
    memcpy(_flush, as_opaque, size());
    m_program->m_dirty_uniforms &= ~m_mask;
  }

  rx_size uniform::size_for_type(category _type) {
    switch (_type) {
    case uniform::category::k_sampler1D:
      [[fallthrough]];
    case uniform::category::k_sampler2D:
      [[fallthrough]];
    case uniform::category::k_sampler3D:
      [[fallthrough]];
    case uniform::category::k_samplerCM:
      return sizeof(int);
    case uniform::category::k_bool:
      return sizeof(bool);
    case uniform::category::k_int:
      return sizeof(rx_s32);
    case uniform::category::k_float:
      return sizeof(rx_f32);
    case uniform::category::k_vec2i:
      return sizeof(math::vec2i);
    case uniform::category::k_vec3i:
      return sizeof(math::vec3i);
    case uniform::category::k_vec4i:
      return sizeof(math::vec4i);
    case uniform::category::k_vec2f:
      return sizeof(math::vec2f);
    case uniform::category::k_vec3f:
      return sizeof(math::vec3f);
    case uniform::category::k_vec4f:
      return sizeof(math::vec4f);
    case uniform::category::k_mat3x3f:
      return sizeof(math::mat3x3f);
    case uniform::category::k_mat4x4f:
      return sizeof(math::mat4x4f);
    }
    RX_ASSERT(false, "unreachable");
  }

  void uniform::record_sampler(int _sampler) {
    RX_ASSERT(is_sampler(m_type), "not a sampler");
    if (*as_int != _sampler) {
      *as_int = _sampler;
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_int(int _value) {
    RX_ASSERT(m_type == category::k_int, "not an int");
    if (*as_int != _value) {
      *as_int = _value;
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec2i(const math::vec2i& _value) {
    RX_ASSERT(m_type == category::k_vec2i, "not a vec2i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec3i(const math::vec3i& _value) {
    RX_ASSERT(m_type == category::k_vec3i, "not a vec3i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec4i(const math::vec4i& _value) {
    RX_ASSERT(m_type == category::k_vec4i, "not a vec4i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_bool(bool _value) {
    RX_ASSERT(m_type == category::k_bool, "not a bool");
    if (*as_boolean != _value) {
      *as_boolean = _value;
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_float(rx_f32 _value) {
    RX_ASSERT(m_type == category::k_float, "not a float");
    if (*as_float != _value) {
      *as_float = _value;
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec2f(const math::vec2f& _value) {
    RX_ASSERT(m_type == category::k_vec2f, "not a vec2f");
    if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec3f(const math::vec3f& _value) {
    RX_ASSERT(m_type == category::k_vec3f, "not a vec3f");
    if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_vec4f(const math::vec4f& _value) {
    RX_ASSERT(m_type == category::k_vec4f, "not a vec4f");
    if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_mat3x3f(const math::mat3x3f& _value) {
    RX_ASSERT(m_type == category::k_mat3x3f, "not a mat3x3f");
    if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_mat4x4f(const math::mat4x4f& _value) {
    RX_ASSERT(m_type == category::k_mat4x4f, "not a mat4x4f");
    if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_program->m_dirty_uniforms |= m_mask;
    }
  }

  void uniform::record_raw(const rx_byte* _data, rx_size _size) {
    RX_ASSERT(_size == size_for_type(m_type), "invalid size");
    memcpy(as_opaque, _data, _size);
    m_program->m_dirty_uniforms |= m_mask;
  }

  program::program(frontend* _frontend)
    : resource{_frontend, resource::category::k_program}
    , m_allocator{_frontend->allocator()}
    , m_uniforms{m_allocator}
    , m_dirty_uniforms{0}
  {
  }

  void program::add_shader(shader&& _shader) {
    m_shaders.push_back(utility::move(_shader));
  }

  void program::validate() const {
    RX_ASSERT(!m_shaders.is_empty(), "no shaders specified");
  }

  uniform& program::add_uniform(const string& _name, uniform::category _type) {
    m_uniforms.emplace_back(this, m_uniforms.size(), _name, _type);
    return m_uniforms.last();
  }

  rx_u64 program::dirty_uniforms_bitset() const {
    return m_dirty_uniforms;
  }

  rx_size program::dirty_uniforms_size() const {
    rx_size size{0};
    for (rx_size i{bit_next(m_dirty_uniforms, 0)}; i < 64; i = bit_next(m_dirty_uniforms, i + 1)) {
      size += m_uniforms[i].size();
    }
    return size;
  }

  void program::flush_dirty_uniforms(rx_byte* _data) {
    for (rx_size i{bit_next(m_dirty_uniforms, 0)}; i < 64; i = bit_next(m_dirty_uniforms, i + 1)) {
      auto& this_uniform{m_uniforms[i]};
      this_uniform.flush(_data);
      _data += this_uniform.size();
    }

    RX_ASSERT(m_dirty_uniforms == 0, "failed to flush all uniforms");
  }
} // namespace rx::render