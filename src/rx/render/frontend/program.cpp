#include <string.h> // memcpy

#include "rx/render/frontend/program.h"
#include "rx/render/frontend/context.h"

#include "rx/core/utility/bit.h"
#include "rx/core/algorithm/min.h"

namespace Rx::Render::Frontend {

static constexpr const Size MAX_BONES = 80;

// checks if |_type| is a sampler Type
static bool is_sampler(Uniform::Type _type) {
  switch (_type) {
  case Uniform::Type::SAMPLER1D:
    [[fallthrough]];
  case Uniform::Type::SAMPLER2D:
    [[fallthrough]];
  case Uniform::Type::SAMPLER3D:
    [[fallthrough]];
  case Uniform::Type::SAMPLERCM:
    return true;
  default:
    return false;
  }
}

Uniform::Uniform()
  : m_program{nullptr}
{
}

Uniform::Uniform(Program* _program, Uint64 _bit, const String& _name, Type _type)
  : m_program{_program}
  , m_bit{_bit}
  , m_type{_type}
  , m_name{_name}
{
  as_opaque = m_program->m_frontend->allocator().allocate(size());
  RX_ASSERT(as_opaque, "out of memory");

  memset(as_opaque, 0, size());
}

Uniform::Uniform(Uniform&& uniform_)
  : m_program{uniform_.m_program}
  , m_bit{uniform_.m_bit}
  , m_type{uniform_.m_type}
  , m_name{Utility::move(uniform_.m_name)}
{
  as_opaque = Utility::exchange(uniform_.as_opaque, nullptr);
}

Uniform::~Uniform() {
  m_program->m_frontend->allocator().deallocate(as_opaque);
}

void Uniform::flush(Byte* _flush) {
  RX_ASSERT(m_program->m_dirty_uniforms & m_bit, "flush on non-dirty uniform");
  memcpy(_flush, as_opaque, size());
  m_program->m_dirty_uniforms &= ~m_bit;
}

Size Uniform::size_for_type(Type _type) {
  switch (_type) {
  case Uniform::Type::SAMPLER1D:
    [[fallthrough]];
  case Uniform::Type::SAMPLER2D:
    [[fallthrough]];
  case Uniform::Type::SAMPLER3D:
    [[fallthrough]];
  case Uniform::Type::SAMPLERCM:
    return sizeof(int);
  case Uniform::Type::BOOL:
    return sizeof(bool);
  case Uniform::Type::INT:
    return sizeof(Sint32);
  case Uniform::Type::FLOAT:
    return sizeof(Float32);
  case Uniform::Type::VEC2I:
    return sizeof(Math::Vec2i);
  case Uniform::Type::VEC3I:
    return sizeof(Math::Vec3i);
  case Uniform::Type::VEC4I:
    return sizeof(Math::Vec4i);
  case Uniform::Type::VEC2F:
    return sizeof(Math::Vec2f);
  case Uniform::Type::VEC3F:
    return sizeof(Math::Vec3f);
  case Uniform::Type::VEC4F:
    return sizeof(Math::Vec4f);
  case Uniform::Type::MAT3X3F:
    return sizeof(Math::Mat3x3f);
  case Uniform::Type::MAT4X4F:
    return sizeof(Math::Mat4x4f);
  case Uniform::Type::BONES:
    return sizeof(Math::Mat3x4f) * MAX_BONES;
  }

  RX_HINT_UNREACHABLE();
}

void Uniform::record_sampler(int _sampler) {
  RX_ASSERT(is_sampler(m_type), "not a sampler");
  if (*as_int != _sampler) {
    *as_int = _sampler;
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_int(int _value) {
  RX_ASSERT(m_type == Type::INT, "not an int");
  if (*as_int != _value) {
    *as_int = _value;
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec2i(const Math::Vec2i& _value) {
  RX_ASSERT(m_type == Type::VEC2I, "not a vec2i");
  if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
    memcpy(as_int, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec3i(const Math::Vec3i& _value) {
  RX_ASSERT(m_type == Type::VEC3I, "not a vec3i");
  if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
    memcpy(as_int, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec4i(const Math::Vec4i& _value) {
  RX_ASSERT(m_type == Type::VEC4I, "not a vec4i");
  if (memcmp(as_int, _value.data(), sizeof _value) != 0) {
    memcpy(as_int, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_bool(bool _value) {
  RX_ASSERT(m_type == Type::BOOL, "not a bool");
  if (*as_boolean != _value) {
    *as_boolean = _value;
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_float(Float32 _value) {
  RX_ASSERT(m_type == Type::FLOAT, "not a float");
  if (*as_float != _value) {
    *as_float = _value;
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec2f(const Math::Vec2f& _value) {
  RX_ASSERT(m_type == Type::VEC2F, "not a vec2f");
  if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
    memcpy(as_float, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec3f(const Math::Vec3f& _value) {
  RX_ASSERT(m_type == Type::VEC3F, "not a vec3f");
  if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
    memcpy(as_float, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_vec4f(const Math::Vec4f& _value) {
  RX_ASSERT(m_type == Type::VEC4F, "not a vec4f");
  if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
    memcpy(as_float, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_mat3x3f(const Math::Mat3x3f& _value) {
  RX_ASSERT(m_type == Type::MAT3X3F, "not a mat3x3f");
  if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
    memcpy(as_float, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_mat4x4f(const Math::Mat4x4f& _value) {
  RX_ASSERT(m_type == Type::MAT4X4F, "not a mat4x4f");
  if (memcmp(as_float, _value.data(), sizeof _value) != 0) {
    memcpy(as_float, _value.data(), sizeof _value);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_bones(const Vector<Math::Mat3x4f>& _frames, Size _joints) {
  RX_ASSERT(m_type == Type::BONES, "not bones");
  const Size size{sizeof(Math::Mat3x4f) * Algorithm::min(_joints, MAX_BONES)};
  if (memcmp(as_float, _frames.data(), size) != 0) {
    memcpy(as_float, _frames.data(), size);
    m_program->mark_uniform_dirty(m_bit);
  }
}

void Uniform::record_raw(const Byte* _data, Size _size) {
  RX_ASSERT(_size == size_for_type(m_type), "invalid size");
  memcpy(as_opaque, _data, _size);
  m_program->mark_uniform_dirty(m_bit);
}

Program::Program(Context* _frontend)
  : Resource{_frontend, Resource::Type::PROGRAM}
  , m_uniforms{m_frontend->allocator()}
  , m_dirty_uniforms{0}
  , m_padding_uniforms{0}
{
}

bool Program::add_shader(Shader&& shader_) {
  // Run the formatter on the |_shader_.source|.
  if (auto format = format_shader(shader_.source)) {
    shader_.source = Utility::move(*format);
    return m_shaders.push_back(Utility::move(shader_));
  }
  return false;
}

void Program::validate() const {
  RX_ASSERT(!m_shaders.is_empty(), "no shaders specified");
}

Uniform& Program::add_uniform(const String& _name, Uniform::Type _type, bool _is_padding) {
  const Uint64 bit{1_u64 << m_uniforms.size()};
  m_uniforms.emplace_back(this, bit, _name, _type);
  if (_is_padding) {
    m_padding_uniforms |= bit;
  }
  update_resource_usage();
  return m_uniforms.last();
}

Uint64 Program::dirty_uniforms_bitset() const {
  return m_dirty_uniforms;
}

Size Program::dirty_uniforms_size() const {
  Size size{0};
  for (Size i{bit_next(m_dirty_uniforms, 0)}; i < 64; i = bit_next(m_dirty_uniforms, i + 1)) {
    size += m_uniforms[i].size();
  }
  return size;
}

void Program::flush_dirty_uniforms(Byte* _data) {
  for (Size i{bit_next(m_dirty_uniforms, 0)}; i < 64; i = bit_next(m_dirty_uniforms, i + 1)) {
    auto& this_uniform{m_uniforms[i]};
    this_uniform.flush(_data);
    _data += this_uniform.size();
  }

  RX_ASSERT(m_dirty_uniforms == 0, "failed to flush all uniforms");
}

void Program::update_resource_usage() {
  Size usage{0};
  m_uniforms.each_fwd([&usage](const Uniform& _uniform) {
    usage += _uniform.size();
  });
  Resource::update_resource_usage(usage);
}

void Program::mark_uniform_dirty(Uint64 _uniform_bit) {
  if (!(m_padding_uniforms & _uniform_bit)) {
    m_dirty_uniforms |= _uniform_bit;
  }
}

// Reformats the given Rex shading |_source| into clean shader text.
Optional<String> Program::format_shader(const String& _source) {
  auto pass = [&](const char* _ch, char* result_) -> Size {
    Size length = 0;

    // Put a character into |result_| and track the size.
    auto put = [&](int _ch) {
      if (result_) {
        *result_++ = _ch;
      }
      length++;
    };

    int i = 0;
    while (*_ch) switch (*_ch++) {
    case '\r':
      continue;
    case '\n':
      put('\n');
      // Skip whitespace.
      while (*_ch && (*_ch == ' ' || *_ch == '\t' || *_ch == '\r')) {
        _ch++;
      }
      // Emit the indentation level except for preprocessor directives and scope termination.
      if (*_ch && (*_ch != '}' && *_ch != '#')) {
        for (int j = 0; j < i; j++) {
          put(' ');
        }
      }
      break;
    case '/':
      put('/');
      switch (*_ch++) {
      // Line comment.
      case '/':
        put('/');
        // Consume until end-of-line.
        while (*_ch && *_ch != '\n') {
          // Don't emit carrige-return.
          if (*_ch != '\r') {
            put(*_ch);
          }
          _ch++;
        }
        break;
      // Block comment.
      case '*':
        put('*');
        // Consume until end-of-block.
        while (*_ch && strncmp(_ch, "*/", 2) != 0) {
          // Don't emit carrige-return.
          if (*_ch != '\r') {
            put(*_ch);
          }
          _ch++;
        }
        put('*');
        put('/');
        break;
      default:
        _ch--;
        break;
      }
      break;
    case '(':
      [[fallthrough]];
    case '{':
      i++;
      put(_ch[-1]);
      break;
    case ')':
      [[fallthrough]];
    case '}':
      i--;
      [[fallthrough]];
    default:
      put(_ch[-1]);
      break;
    }

    // Don't forget the null-terminator.
    put('\0');

    return length;
  };

  auto data = _source.data();
  auto& allocator = _source.allocator();

  const auto length = pass(data, nullptr);

  LinearBuffer result{allocator};
  if (!result.resize(length)) {
    return nullopt;
  }

  pass(data, reinterpret_cast<char*>(result.data()));

  auto disown = result.disown();
  if (!disown) {
    return nullopt;
  }

  return String{*disown};
}

} // namespace Rx::Render::Frontend
