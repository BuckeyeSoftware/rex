#include "rx/console/variable.h"
#include "rx/console/interface.h"

#include "rx/core/hints/unreachable.h"

namespace rx::console {

const char* variable_type_as_string(variable_type _type) {
  switch (_type) {
  case variable_type::k_boolean:
    return "bool";
  case variable_type::k_string:
    return "string";
  case variable_type::k_int:
    return "int";
  case variable_type::k_float:
    return "float";
  case variable_type::k_vec4f:
    return "vec4f";
  case variable_type::k_vec4i:
    return "vec4i";
  case variable_type::k_vec3f:
    return "vec3f";
  case variable_type::k_vec3i:
    return "vec3i";
  case variable_type::k_vec2f:
    return "vec2f";
  case variable_type::k_vec2i:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
}

static string escape(const string& _contents) {
  string result{_contents.allocator()};
  result.reserve(_contents.size() * 4);
  for (rx_size i{0}; i < _contents.size(); i++) {
    switch (_contents[i]) {
    case '"':
      [[fallthrough]];
    case '\\':
      result += '\\';
      [[fallthrough]];
    default:
      result += _contents[i];
      break;
    }
  }
  return result;
};

variable_reference::variable_reference(const char* name,
  const char* description, void* handle, variable_type type)
  : m_name{name}
  , m_description{description}
  , m_handle{handle}
  , m_type{type}
{
  m_next = interface::add_variable(this);
}

void variable_reference::reset() {
  switch (m_type) {
  case variable_type::k_boolean:
    cast<bool>()->reset();
    break;
  case variable_type::k_string:
    cast<string>()->reset();
    break;
  case variable_type::k_int:
    cast<rx_s32>()->reset();
    break;
  case variable_type::k_float:
    cast<rx_f32>()->reset();
    break;
  case variable_type::k_vec4f:
    cast<math::vec4f>()->reset();
    break;
  case variable_type::k_vec4i:
    cast<math::vec4i>()->reset();
    break;
  case variable_type::k_vec3f:
    cast<math::vec3f>()->reset();
    break;
  case variable_type::k_vec3i:
    cast<math::vec3i>()->reset();
    break;
  case variable_type::k_vec2f:
    cast<math::vec2f>()->reset();
    break;
  case variable_type::k_vec2i:
    cast<math::vec2i>()->reset();
    break;
  }
}

string variable_reference::print_current() const {
  switch (m_type) {
  case variable_type::k_boolean:
    return cast<bool>()->get() ? "true" : "false";
  case variable_type::k_string:
    return string::format("\"%s\"", escape(cast<string>()->get()));
  case variable_type::k_int:
    return string::format("%d", cast<rx_s32>()->get());
  case variable_type::k_float:
    return string::format("%f", cast<rx_f32>()->get());
  case variable_type::k_vec4f:
    return string::format("%s", cast<math::vec4f>()->get());
  case variable_type::k_vec4i:
    return string::format("%s", cast<math::vec4i>()->get());
  case variable_type::k_vec3f:
    return string::format("%s", cast<math::vec3f>()->get());
  case variable_type::k_vec3i:
    return string::format("%s", cast<math::vec3i>()->get());
  case variable_type::k_vec2f:
    return string::format("%s", cast<math::vec2f>()->get());
  case variable_type::k_vec2i:
    return string::format("%s", cast<math::vec2i>()->get());
  }

  RX_HINT_UNREACHABLE();
}

string variable_reference::print_range() const {
  if (m_type == variable_type::k_int) {
    const auto handle{cast<rx_s32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_int_min ? "-inf" : string::format("%d", min)};
    const auto max_fmt{max == k_int_max ? "+inf" : string::format("%d", max)};
    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_float) {
    const auto handle{cast<rx_f32>()};
    const auto min{handle->min()};
    const auto max{handle->max()};
    const auto min_fmt{min == k_float_min ? "-inf" : string::format("%f", min)};
    const auto max_fmt{max == k_float_max ? "+inf" : string::format("%f", max)};
    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec4f) {
    const auto handle{cast<vec4f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : string::format("%f", min.z)};
      const auto min_w{min.w == k_float_min ? "-inf" : string::format("%f", min.w)};
      min_fmt = string::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : string::format("%f", max.z)};
      const auto max_w{max.w == k_float_max ? "+inf" : string::format("%f", max.w)};
      max_fmt = string::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec4i) {
    const auto handle{cast<vec4i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : string::format("%d", min.z)};
      const auto min_w{min.w == k_int_min ? "-inf" : string::format("%d", min.w)};
      min_fmt = string::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : string::format("%d", max.z)};
      const auto max_w{max.w == k_int_max ? "+inf" : string::format("%d", max.w)};
      max_fmt = string::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec3f) {
    const auto handle{cast<vec3f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
      const auto min_z{min.z == k_float_min ? "-inf" : string::format("%f", min.z)};
      min_fmt = string::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
      const auto max_z{max.z == k_float_max ? "+inf" : string::format("%f", max.z)};
      max_fmt = string::format("{%s, %s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec3i) {
    const auto handle{cast<vec3i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
      const auto min_z{min.z == k_int_min ? "-inf" : string::format("%d", min.z)};
      min_fmt = string::format("{%s, %s, %s}", min_x, min_y, min_z);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
      const auto max_z{max.z == k_int_max ? "+inf" : string::format("%d", max.z)};
      max_fmt = string::format("{%s, %s, %s}", max_x, max_y, max_z);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec2f) {
    const auto handle{cast<vec2f>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_float_min)) {
      const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
      const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
      min_fmt = string::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_float_max)) {
      const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
      const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
      max_fmt = string::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  } else if (m_type == variable_type::k_vec2i) {
    const auto handle{cast<vec2i>()};
    const auto min{handle->min()};
    const auto max{handle->max()};

    string min_fmt;
    if (min.is_any(k_int_min)) {
      const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
      const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
      min_fmt = string::format("{%s, %s}", min_x, min_y);
    } else {
      min_fmt = string::format("%s", min);
    }

    string max_fmt;
    if (max.is_any(k_int_max)) {
      const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
      const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
      max_fmt = string::format("{%s, %s}", max_x, max_y);
    } else {
      max_fmt = string::format("%s", max);
    }

    return string::format("[%s, %s]", min_fmt, max_fmt);
  }

  RX_HINT_UNREACHABLE();
}

string variable_reference::print_initial() const {
  switch (m_type) {
  case variable_type::k_boolean:
    return cast<bool>()->initial() ? "true" : "false";
  case variable_type::k_string:
    return string::format("\"%s\"", escape(cast<string>()->initial()));
  case variable_type::k_int:
    return string::format("%d", cast<rx_s32>()->initial());
  case variable_type::k_float:
    return string::format("%f", cast<rx_f32>()->initial());
  case variable_type::k_vec4f:
    return string::format("%s", cast<math::vec4f>()->initial());
  case variable_type::k_vec4i:
    return string::format("%s", cast<math::vec4i>()->initial());
  case variable_type::k_vec3f:
    return string::format("%s", cast<math::vec3f>()->initial());
  case variable_type::k_vec3i:
    return string::format("%s", cast<math::vec3i>()->initial());
  case variable_type::k_vec2f:
    return string::format("%s", cast<math::vec2f>()->initial());
  case variable_type::k_vec2i:
    return string::format("%s", cast<math::vec2i>()->initial());
  }

  RX_HINT_UNREACHABLE();
}

bool variable_reference::is_initial() const {
  switch (m_type) {
  case variable_type::k_boolean:
    return cast<bool>()->get() == cast<bool>()->initial();
  case variable_type::k_string:
    return cast<string>()->get() == cast<string>()->initial();
  case variable_type::k_int:
    return cast<rx_s32>()->get() == cast<rx_s32>()->initial();
  case variable_type::k_float:
    return cast<rx_f32>()->get() == cast<rx_f32>()->initial();
  case variable_type::k_vec4f:
    return cast<math::vec4f>()->get() == cast<math::vec4f>()->initial();
  case variable_type::k_vec4i:
    return cast<math::vec4i>()->get() == cast<math::vec4i>()->initial();
  case variable_type::k_vec3f:
    return cast<math::vec3f>()->get() == cast<math::vec3f>()->initial();
  case variable_type::k_vec3i:
    return cast<math::vec3i>()->get() == cast<math::vec3i>()->initial();
  case variable_type::k_vec2f:
    return cast<math::vec2f>()->get() == cast<math::vec2f>()->initial();
  case variable_type::k_vec2i:
    return cast<math::vec2i>()->get() == cast<math::vec2i>()->initial();
  }

  RX_HINT_UNREACHABLE();
}

template struct variable<rx_s32>;
template struct variable<rx_f32>;
template struct variable<vec2i>;
template struct variable<vec2f>;
template struct variable<vec3i>;
template struct variable<vec3f>;
template struct variable<vec4i>;
template struct variable<vec4f>;

} // namespace rx::console
