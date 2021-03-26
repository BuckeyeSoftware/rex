#include "rx/console/variable.h"
#include "rx/console/context.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Console {

const char* VariableType_as_string(VariableType _type) {
  switch (_type) {
  case VariableType::BOOLEAN:
    return "bool";
  case VariableType::STRING:
    return "string";
  case VariableType::INT:
    return "int";
  case VariableType::FLOAT:
    return "float";
  case VariableType::VEC4F:
    return "vec4f";
  case VariableType::VEC4I:
    return "vec4i";
  case VariableType::VEC3F:
    return "vec3f";
  case VariableType::VEC3I:
    return "vec3i";
  case VariableType::VEC2F:
    return "vec2f";
  case VariableType::VEC2I:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
}

static Optional<String> escape(const String& _contents) {
  String result{_contents.allocator()};
  if (!result.reserve(_contents.size() * 4)) {
    return nullopt;
  }

  for (Size i{0}; i < _contents.size(); i++) {
    switch (_contents[i]) {
    case '"':
      [[fallthrough]];
    case '\\':
      // These appends shouldn't fail due to the reserve above. Check anyways
      // to silence nodiscard warnings with String::append.
      if (!result.append('\\')) {
        return nullopt;
      }
      [[fallthrough]];
    default:
      if (!result.append(_contents[i])) {
        return nullopt;
      }
      break;
    }
  }

  return result;
};

VariableReference::VariableReference(const char* name,
  const char* description, void* handle, VariableType type)
  : m_name{name}
  , m_description{description}
  , m_handle{handle}
  , m_type{type}
{
  m_next = Context::add_variable(this);
}

void VariableReference::reset() {
  switch (m_type) {
  case VariableType::BOOLEAN:
    cast<Bool>()->reset();
    break;
  case VariableType::STRING:
    cast<String>()->reset();
    break;
  case VariableType::INT:
    cast<Sint32>()->reset();
    break;
  case VariableType::FLOAT:
    cast<Float32>()->reset();
    break;
  case VariableType::VEC4F:
    cast<Math::Vec4f>()->reset();
    break;
  case VariableType::VEC4I:
    cast<Math::Vec4i>()->reset();
    break;
  case VariableType::VEC3F:
    cast<Math::Vec3f>()->reset();
    break;
  case VariableType::VEC3I:
    cast<Math::Vec3i>()->reset();
    break;
  case VariableType::VEC2F:
    cast<Math::Vec2f>()->reset();
    break;
  case VariableType::VEC2I:
    cast<Math::Vec2i>()->reset();
    break;
  }
}

String VariableReference::print_current() const {
  switch (m_type) {
  case VariableType::BOOLEAN:
    return cast<Bool>()->get() ? "true" : "false";
  case VariableType::STRING:
    return String::format("\"%s\"", *escape(cast<String>()->get()));
  case VariableType::INT:
    return String::format("%d", cast<Sint32>()->get());
  case VariableType::FLOAT:
    return String::format("%f", cast<Float32>()->get());
  case VariableType::VEC4F:
    return String::format("%s", cast<Math::Vec4f>()->get());
  case VariableType::VEC4I:
    return String::format("%s", cast<Math::Vec4i>()->get());
  case VariableType::VEC3F:
    return String::format("%s", cast<Math::Vec3f>()->get());
  case VariableType::VEC3I:
    return String::format("%s", cast<Math::Vec3i>()->get());
  case VariableType::VEC2F:
    return String::format("%s", cast<Math::Vec2f>()->get());
  case VariableType::VEC2I:
    return String::format("%s", cast<Math::Vec2i>()->get());
  }

  RX_HINT_UNREACHABLE();
}

String VariableReference::print_range() const {
  if (m_type == VariableType::INT) {
    const auto handle{cast<Sint32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_int_min ? "-inf" : String::format("%d", min)};
    const auto max_fmt{max == k_int_max ? "+inf" : String::format("%d", max)};
    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::FLOAT) {
    const auto handle{cast<Float32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_float_min ? "-inf" : String::format("%f", min)};
    const auto max_fmt{max == k_float_max ? "+inf" : String::format("%f", max)};
    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC4F) {
    const auto handle{cast<Vec4f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : String::format("%f", min.z)};
      const auto min_w{min.w == k_float_min ? "-inf" : String::format("%f", min.w)};
      min_fmt = String::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : String::format("%f", max.z)};
      const auto max_w{max.w == k_float_max ? "+inf" : String::format("%f", max.w)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC4I) {
    const auto handle{cast<Vec4i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : String::format("%d", min.z)};
      const auto min_w{min.w == k_int_min ? "-inf" : String::format("%d", min.w)};
      min_fmt = String::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : String::format("%d", max.z)};
      const auto max_w{max.w == k_int_max ? "+inf" : String::format("%d", max.w)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC3F) {
    const auto handle{cast<Vec3f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : String::format("%f", min.z)};
      min_fmt = String::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : String::format("%f", max.z)};
      max_fmt = String::format("{%s, %s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC3I) {
    const auto handle{cast<Vec3i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : String::format("%d", min.z)};
      min_fmt = String::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : String::format("%d", max.z)};
      max_fmt = String::format("{%s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC2F) {
    const auto handle{cast<Vec2f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : String::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : String::format("%f", min.y)};
      min_fmt = String::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : String::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : String::format("%f", max.y)};
      max_fmt = String::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == VariableType::VEC2I) {
    const auto handle{cast<Vec2i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    String min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : String::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : String::format("%d", min.y)};
      min_fmt = String::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = String::format("%s", min);
    }

    String max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : String::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : String::format("%d", max.y)};
      max_fmt = String::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = String::format("%s", max);
    }

    return String::format("[%s, %s]", min_fmt, max_fmt);
  }

  RX_HINT_UNREACHABLE();
}

String VariableReference::print_initial() const {
  switch (m_type) {
  case VariableType::BOOLEAN:
    return cast<Bool>()->initial() ? "true" : "false";
  case VariableType::STRING:
    return String::format("\"%s\"", *escape(cast<String>()->initial()));
  case VariableType::INT:
    return String::format("%d", cast<Sint32>()->initial());
  case VariableType::FLOAT:
    return String::format("%f", cast<Float32>()->initial());
  case VariableType::VEC4F:
    return String::format("%s", cast<Math::Vec4f>()->initial());
  case VariableType::VEC4I:
    return String::format("%s", cast<Math::Vec4i>()->initial());
  case VariableType::VEC3F:
    return String::format("%s", cast<Math::Vec3f>()->initial());
  case VariableType::VEC3I:
    return String::format("%s", cast<Math::Vec3i>()->initial());
  case VariableType::VEC2F:
    return String::format("%s", cast<Math::Vec2f>()->initial());
  case VariableType::VEC2I:
    return String::format("%s", cast<Math::Vec2i>()->initial());
  }

  RX_HINT_UNREACHABLE();
}

bool VariableReference::is_initial() const {
  switch (m_type) {
  case VariableType::BOOLEAN:
    return cast<Bool>()->get() == cast<Bool>()->initial();
  case VariableType::STRING:
    return cast<String>()->get() == cast<String>()->initial();
  case VariableType::INT:
    return cast<Sint32>()->get() == cast<Sint32>()->initial();
  case VariableType::FLOAT:
    return cast<Float32>()->get() == cast<Float32>()->initial();
  case VariableType::VEC4F:
    return cast<Math::Vec4f>()->get() == cast<Math::Vec4f>()->initial();
  case VariableType::VEC4I:
    return cast<Math::Vec4i>()->get() == cast<Math::Vec4i>()->initial();
  case VariableType::VEC3F:
    return cast<Math::Vec3f>()->get() == cast<Math::Vec3f>()->initial();
  case VariableType::VEC3I:
    return cast<Math::Vec3i>()->get() == cast<Math::Vec3i>()->initial();
  case VariableType::VEC2F:
    return cast<Math::Vec2f>()->get() == cast<Math::Vec2f>()->initial();
  case VariableType::VEC2I:
    return cast<Math::Vec2i>()->get() == cast<Math::Vec2i>()->initial();
  }

  RX_HINT_UNREACHABLE();
}

template struct Variable<Sint32>;
template struct Variable<Float32>;
template struct Variable<Vec2i>;
template struct Variable<Vec2f>;
template struct Variable<Vec3i>;
template struct Variable<Vec3f>;
template struct Variable<Vec4i>;
template struct Variable<Vec4f>;

} // namespace Rx::Console
