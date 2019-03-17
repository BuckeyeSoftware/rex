#include <string.h> // memcpy

#include <rx/render/program.h>
#include <rx/render/frontend.h>
#include <rx/core/utility/bit.h>

namespace rx {
namespace render {
  uniform::uniform() 
    : m_allocator{nullptr}
  {
  }

  uniform::uniform(memory::allocator* _allocator, const string& _name, category _type)
    : m_allocator{_allocator}
    , m_type{_type}
    , m_name{_name}
  {
    as_opaque = m_allocator->allocate(size_for_type(m_type));
  }

  uniform::~uniform() {
    m_allocator->deallocate(as_opaque);
  }

  void uniform::flush(rx_byte* _flush) {
    RX_ASSERT(m_dirty, "flush on non-dirty uniform");
    memcpy(_flush, as_opaque, size());
    m_dirty = false;
  }

  rx_size uniform::size_for_type(category _type) {
    switch (_type) {
    case uniform::category::k_sampler:
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
    RX_ASSERT(m_type == category::k_sampler, "not a sampler");
    if (*as_int != _sampler) {
      *as_int = _sampler;
      m_dirty = true;
    }
  }

  void uniform::record_int(int _value) {
    RX_ASSERT(m_type == category::k_int, "not an int");
    if (*as_int != _value) {
      *as_int = _value;
      m_dirty = true;
    }
  }

  void uniform::record_vec2i(const math::vec2i& _value) {
    RX_ASSERT(m_type == category::k_vec2i, "not a vec2i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_vec3i(const math::vec3i& _value) {
    RX_ASSERT(m_type == category::k_vec3i, "not a vec3i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_vec4i(const math::vec4i& _value) {
    RX_ASSERT(m_type == category::k_vec4i, "not a vec4i");
    if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
      memcpy(as_int, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_bool(bool _value) {
    RX_ASSERT(m_type == category::k_bool, "not a bool");
    if (*as_boolean != _value) {
      *as_boolean = _value;
      m_dirty = true;
    }
  }

  void uniform::record_float(rx_f32 _value) {
    RX_ASSERT(m_type == category::k_float, "not a float");
    if (*as_float != _value) {
      *as_float = _value;
      m_dirty = true;
    }
  }

  void uniform::record_vec2f(const math::vec2f& _value) {
    RX_ASSERT(m_type == category::k_vec2f, "not a vec2f");
    if (memcmp(as_float, _value.data(), sizeof _value)) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_vec3f(const math::vec3f& _value) {
    RX_ASSERT(m_type == category::k_vec3f, "not a vec3f");
    if (memcmp(as_float, _value.data(), sizeof _value)) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_vec4f(const math::vec4f& _value) {
    RX_ASSERT(m_type == category::k_vec4f, "not a vec4f");
    if (memcmp(as_float, _value.data(), sizeof _value)) {
      memcpy(as_float, _value.data(), sizeof _value);
      m_dirty = true;
    }
  }

  void uniform::record_raw(const rx_byte* _data, rx_size _size) {
    RX_ASSERT(_size == size_for_type(m_type), "invalid size");
    memcpy(as_opaque, _data, _size);
    m_dirty = true;
  }

  program::description::description(const string& _name, int _shaders, const array<string>& _data,
    const array<string>& _layout, const array<string>& _defines)
    : name{_name}
    , shaders{_shaders}
    , data{_data}
    , layout{_layout}
    , defines{_defines}
  {
  }

  bool operator==(const program::description& _lhs, const program::description& _rhs) {
    if (_lhs.name != _rhs.name) {
      return false;
    }

    if (_lhs.data.size() != _rhs.data.size()) {
      return false;
    }

    if (_lhs.layout.size() != _rhs.layout.size()) {
      return false;
    }

    if (_lhs.defines.size() != _rhs.defines.size()) {
      return false;
    }

    for (rx_size i{0}; i < _lhs.data.size(); i++) {
      if (_lhs.data[i] != _rhs.data[i]) {
        return false;
      }
    }

    for (rx_size i{0}; i < _lhs.layout.size(); i++) {
      if (_lhs.layout[i] != _rhs.layout[i]) {
        return false;
      }
    }

    for (rx_size i{0}; i < _lhs.defines.size(); i++) {
      if (_lhs.defines[i] != _rhs.defines[i]) {
        return false;
      }
    }

    return true;
  }

  program::program(frontend* _frontend)
    : resource{_frontend, resource::category::k_program}
    , m_uniforms{m_frontend->allocator()}
    , m_dirty_bits{0}
    , m_has_description{false}
  {
  }

  void program::record_description(const description& _description) {
    m_description = _description;
    m_has_description = true;
  }

  bool program::validate() const {
    return m_has_description;
  }

  uniform& program::add_uniform(const string& _name, uniform::category _type) {
    m_uniforms.emplace_back(m_frontend->allocator(), _name, _type);
    return m_uniforms.last();
  }

  uniform& program::operator[](rx_size _index) {
    return m_uniforms[_index];
  }

  array<rx_byte> program::flush() {
    if (m_dirty_bits == 0) {
      return {};
    }

    // calculate storage needed to flush program values
    rx_size size{sizeof m_dirty_bits};
    for (rx_size i{bit_next(m_dirty_bits, 0)}; i < sizeof m_dirty_bits * CHAR_BIT; i = bit_next(m_dirty_bits, i + 1)) {
      size += m_uniforms[i].size();
    }

    array<rx_byte> data(&memory::g_system_allocator, size);

    // store dirty bitset as a header to the block of memory to know which uniforms
    // changed, if bit N is set then it means uniform N needs to be changed
    rx_size offset{0};
    memcpy(data.data(), &m_dirty_bits, sizeof m_dirty_bits);
    offset += sizeof m_dirty_bits;

    // store dirty uniform data into block of memory
    for (rx_size i{bit_next(m_dirty_bits, 0)}; i < sizeof m_dirty_bits * CHAR_BIT; i = bit_next(m_dirty_bits, i + 1)) {
      auto& this_uniform{m_uniforms[i]};
      this_uniform.flush(data.data() + offset);
      offset += this_uniform.size();
    }

    return data;
  }
} // namespace render

rx_size hash<render::program::description>::operator()(const render::program::description& _description) {
  using hasher = hash<string>;
  rx_size result{hasher{}(_description.name)};

  _description.data.each_fwd([&result](const string& _data) {
    result = hash_combine(result, hasher{}(_data));
  });

  _description.layout.each_fwd([&result](const string& _layout) {
    result = hash_combine(result, hasher{}(_layout));
  });

  _description.defines.each_fwd([&result](const string& _define) {
    result = hash_combine(result, hasher{}(_define));
  });

  return result;
}

} // namespace rx