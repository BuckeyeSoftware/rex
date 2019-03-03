#include <string.h> // strcmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errno, ERANGE

#include <rx/console/console.h>
#include <rx/console/variable.h>

#include <rx/core/concurrency/spin_lock.h>
#include <rx/core/concurrency/scope_lock.h>

#include <rx/core/filesystem/file.h>

#include <rx/core/log.h> // RX_LOG

namespace rx::console {

using rx::concurrency::spin_lock;
using rx::concurrency::scope_lock;

static spin_lock g_lock;
static variable_reference* g_head; // protected by |g_lock|

RX_LOG("console", console_print);

bool console::load(const char* file_name) {
  // sort references
  {
    scope_lock<spin_lock> locked(g_lock);
    g_head = sort(g_head);
  }

  rx::filesystem::file file(file_name, "r");
  if (!file) {
    return false;
  }

  console_print(rx::log::level::k_info, "loading '%s'", file_name);
  for (rx::string line_contents; file.read_line(line_contents); ) {
    rx::string line{line_contents.lstrip(" \t")};
    if (line.is_empty() || strchr("#;[", line[0])) {
      // ignore empty and comment lines
      continue;
    }

    // tokenize line contents
    rx::array<string> tokens{line.split(' ', 2)};
    variable_status status{change(tokens[0], tokens[1])};
    switch (status) {
    case variable_status::k_malformed:
      console_print(rx::log::level::k_error, "'%s' malformed", tokens[0]);
      break;
    case variable_status::k_not_found:
      console_print(rx::log::level::k_error, "'%s' not found", tokens[0]);
      break;
    case variable_status::k_out_of_range:
      console_print(rx::log::level::k_error, "'%s' out of range", tokens[0]);
      break;
    case variable_status::k_success:
      console_print(rx::log::level::k_info, "'%s' changed to '%s'", tokens[0], tokens[1]);
      break;
    case variable_status::k_type_mismatch:
      console_print(rx::log::level::k_error, "'%s' type mismatch", tokens[0]);
      break;
    }
  }

  return true;
}

bool console::save(const char* file_name) {
  rx::filesystem::file file(file_name, "w");
  if (!file) {
    return false;
  }

  console_print(rx::log::level::k_info, "saving '%s'", file_name);
  for (const variable_reference *head{g_head}; head; head = head->m_next) {
    if (head->type() == variable_type::k_boolean) {
      const auto handle{head->cast<bool>()};
      file.print("## %s (defaults to %s)\n",
        head->description(), handle->initial() ? "true" : "false");
      file.print(handle->get() == handle->initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), handle->get() ? "true" : "false");
    } else if (head->type() == variable_type::k_int) {
      const auto handle{head->cast<rx_s32>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      const auto min_fmt{min == k_int_min ? "-inf" : string{"%d", min}};
      const auto max_fmt{max == k_int_max ? "+inf" : string{"%d", max}};
      file.print("## %s (in range [%s, %s], defaults to %d)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %d\n" : "%s %d\n",
        head->name(), handle->get());
    } else if (head->type() == variable_type::k_float) {
      const auto handle{head->cast<rx_f32>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      const auto min_fmt{min == k_float_min ? "-inf" : string{"%f", min}};
      const auto max_fmt{max == k_float_max ? "+inf" : string{"%f", max}};
      file.print("## %s (in range [%s, %s], defaults to %f)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %f\n" : "%s %f\n",
        head->name(), handle->get());
    } else if (head->type() == variable_type::k_string) {
      const auto escape = [](const string& contents) {
        string result;
        result.reserve(contents.size() * 4);
        for (rx_size i{0}; i < contents.size(); i++) {
          switch (contents[i]) {
          case '"':
            [[fallthrough]];
          case '\\':
            result += '\\';
            [[fallthrough]];
          default:
            result += contents[i];
            break;
          }
        }
        return result;
      };
      const auto handle{head->cast<string>()};
      const auto get_escaped{escape(handle->get())};
      const auto initial_escaped{escape(handle->initial())};
      file.print("## %s (defaults to \"%s\")\n",
        head->description(), initial_escaped);
      file.print(get_escaped == initial_escaped ? ";%s \"%s\"\n" : "%s \"%s\"\n",
        head->name(), get_escaped);
    } else if (head->type() == variable_type::k_vec4f) {
      const auto handle{head->cast<vec4f>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_float_min) || max.is_any(k_float_max)) {
        const auto min_x{min.x == k_float_min ? "-inf" : string{"%f", min.x}};
        const auto min_y{min.y == k_float_min ? "-inf" : string{"%f", min.y}};
        const auto min_z{min.z == k_float_min ? "-inf" : string{"%f", min.z}};
        const auto min_w{min.w == k_float_min ? "-inf" : string{"%f", min.w}};
        const auto max_x{max.x == k_float_max ? "+inf" : string{"%f", max.x}};
        const auto max_y{max.y == k_float_max ? "+inf" : string{"%f", max.y}};
        const auto max_z{max.z == k_float_max ? "+inf" : string{"%f", max.z}};
        const auto max_w{max.w == k_float_max ? "+inf" : string{"%f", max.w}};
        min_fmt = { "{%s, %s, %s, %s}", min_x, min_y, min_z, min_w };
        max_fmt = { "{%s, %s, %s, %s}", max_x, max_y, max_z, max_w };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), handle->initial());
    } else if (head->type() == variable_type::k_vec4i) {
      const auto handle{head->cast<vec4i>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_int_min) || max.is_any(k_int_max)) {
        const auto min_x{min.x == k_int_min ? "-inf" : string{"%d", min.x}};
        const auto min_y{min.y == k_int_min ? "-inf" : string{"%d", min.y}};
        const auto min_z{min.z == k_int_min ? "-inf" : string{"%d", min.z}};
        const auto min_w{min.w == k_int_min ? "-inf" : string{"%d", min.w}};
        const auto max_x{max.x == k_int_max ? "+inf" : string{"%d", max.x}};
        const auto max_y{max.y == k_int_max ? "+inf" : string{"%d", max.y}};
        const auto max_z{max.z == k_int_max ? "+inf" : string{"%d", max.z}};
        const auto max_w{min.w == k_int_min ? "+inf" : string{"%d", max.w}};
        min_fmt = { "{%s, %s, %s, %s}", min_x, min_y, min_z, min_w };
        max_fmt = { "{%s, %s, %s, %s}", max_x, max_y, max_z, max_w };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->name(), min_fmt, max_fmt, handle->initial());
    } else if (head->type() == variable_type::k_vec3f) {
      const auto handle{head->cast<vec3f>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_float_min) || max.is_any(k_float_max)) {
        const auto min_x{min.x == k_float_min ? "-inf" : string{"%f", min.x}};
        const auto min_y{min.y == k_float_min ? "-inf" : string{"%f", min.y}};
        const auto min_z{min.z == k_float_min ? "-inf" : string{"%f", min.z}};
        const auto max_x{max.x == k_float_max ? "+inf" : string{"%f", max.x}};
        const auto max_y{max.y == k_float_max ? "+inf" : string{"%f", max.y}};
        const auto max_z{max.z == k_float_max ? "+inf" : string{"%f", max.z}};
        min_fmt = { "{%s, %s, %s}", min_x, min_y, min_z };
        max_fmt = { "{%s, %s, %s}", max_x, max_y, max_z };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), handle->initial());
    } else if (head->type() == variable_type::k_vec3i) {
      const auto handle{head->cast<vec3i>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_int_min) || max.is_any(k_int_max)) {
        const auto min_x{min.x == k_int_min ? "-inf" : string{"%d", min.x}};
        const auto min_y{min.y == k_int_min ? "-inf" : string{"%d", min.y}};
        const auto min_z{min.z == k_int_min ? "-inf" : string{"%d", min.z}};
        const auto max_x{max.x == k_int_max ? "+inf" : string{"%d", max.x}};
        const auto max_y{max.y == k_int_max ? "+inf" : string{"%d", max.y}};
        const auto max_z{max.z == k_int_max ? "+inf" : string{"%d", max.z}};
        min_fmt = { "{%s, %s, %s}", min_x, min_y, min_z };
        max_fmt = { "{%s, %s, %s}", max_x, max_y, max_z };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->name(), min_fmt, max_fmt, handle->initial());
    } else if (head->type() == variable_type::k_vec2f) {
      const auto handle{head->cast<vec2f>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_float_min) || max.is_any(k_float_max)) {
        const auto min_x{min.x == k_float_min ? "-inf" : string{"%f", min.x}};
        const auto min_y{min.y == k_float_min ? "-inf" : string{"%f", min.y}};
        const auto max_x{max.x == k_float_max ? "+inf" : string{"%f", max.x}};
        const auto max_y{max.y == k_float_max ? "+inf" : string{"%f", max.y}};
        min_fmt = { "{%s, %s}", min_x, min_y };
        max_fmt = { "{%s, %s}", max_x, max_y };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), handle->initial());
    } else if (head->type() == variable_type::k_vec2i) {
      const auto handle{head->cast<vec2i>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      string min_fmt;
      string max_fmt;
      if (min.is_any(k_int_min) || max.is_any(k_int_max)) {
        const auto min_x{min.x == k_int_min ? "-inf" : string{"%d", min.x}};
        const auto min_y{min.y == k_int_min ? "-inf" : string{"%d", min.y}};
        const auto max_x{max.x == k_int_max ? "+inf" : string{"%d", max.x}};
        const auto max_y{max.y == k_int_max ? "+inf" : string{"%d", max.y}};
        min_fmt = { "{%s, %s}", min_x, min_y };
        max_fmt = { "{%s, %s}", max_x, max_y };
      } else {
        min_fmt = { "%s", min };
        max_fmt = { "%s", max };
      }
      file.print("## %s (in range [%s, %s], defaults to %s)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), handle->initial());
    }
  }

  return true;
}

// parse string contents into value for console
template<>
variable_status console::parse_string<bool>(const string& contents, bool& result) {
  if (contents == "true" || contents == "false") {
    result = contents == "true";
    return variable_status::k_success;
  }
  return variable_status::k_type_mismatch;
}

template<>
variable_status console::parse_string<string>(const string& contents, string& result) {
  result = contents;
  return variable_status::k_success;
}

template<>
variable_status console::parse_string<rx_s32>(const string& contents, rx_s32& result) {
  const char* input{contents.data()};
  char* scan{nullptr};

  const auto value{strtoll(input, &scan, 10)};

  if (errno == ERANGE) {
    return variable_status::k_out_of_range;
  }

  if (scan == input || *scan != '\0') {
    return variable_status::k_malformed;
  }

  if (value < INT_MIN || value > INT_MAX) {
    return variable_status::k_out_of_range;
  }

  result = value;
  return variable_status::k_success;
}

template<>
variable_status console::parse_string<rx_f32>(const string& contents, rx_f32& result) {
  const char* input{contents.data()};
  char* scan{nullptr};

  const auto value{strtof(input, &scan)};

  if (errno == ERANGE) {
    return variable_status::k_out_of_range;
  }

  if (scan == input || *scan != '\0') {
    return variable_status::k_malformed;
  }

  result = value;
  return variable_status::k_success;
}

// vec4
template<>
variable_status console::parse_string<math::vec4f>(const string& contents, math::vec4f& result) {
  math::vec4f value;
  if (contents.scan("{%f, %f, %f, %f}", &value.x, &value.y, &value.z, &value.w) != 4) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status console::parse_string<math::vec4i>(const string& contents, math::vec4i& result) {
  math::vec4i value;
  if (contents.scan("{%d, %d, %d, %d}", &value.x, &value.y, &value.z, &value.w) != 4) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

// vec3
template<>
variable_status console::parse_string<math::vec3f>(const string& contents, math::vec3f& result) {
  math::vec3f value;
  if (contents.scan("{%f, %f, %f}", &value.x, &value.y, &value.z) != 3) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status console::parse_string<math::vec3i>(const string& contents, math::vec3i& result) {
  math::vec3i value;
  if (contents.scan("{%d, %d, %d}", &value.x, &value.y, &value.z) != 3) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

// vec2
template<>
variable_status console::parse_string<math::vec2f>(const string& contents, math::vec2f& result) {
  math::vec2f value;
  if (contents.scan("{%f, %f}", &value.x, &value.y) != 2) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status console::parse_string<math::vec2i>(const string& contents, math::vec2i& result) {
  math::vec2i value;
  if (contents.scan("{%d, %d}", &value.x, &value.y) != 2) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<typename T>
variable_status console::set_from_string(const string& name, const string& value_string) {
  T value;
  variable_status status;
  if ((status = parse_string<T>(value_string, value)) == variable_status::k_success) {
    return set_from_value<T>(name, value);
  }
  return status;
}

template<typename T>
variable_status console::set_from_value(const string& name, const T& value) {
  for (auto head{g_head}; head; head = head->m_next) {
    if (head->name() == name) {
      if (head->type() != variable_trait<T>::type) {
        return variable_status::k_type_mismatch;
      }
      return head->cast<T>()->set(value);
    }
  }
  return variable_status::k_not_found;
}

template variable_status console::set_from_value<bool>(const string& name, const bool& value);
template variable_status console::set_from_value<string>(const string& name, const string& value);
template variable_status console::set_from_value<rx_s32>(const string& name, const rx_s32& value);
template variable_status console::set_from_value<rx_f32>(const string& name, const rx_f32& value);
template variable_status console::set_from_value<math::vec4f>(const string& name, const math::vec4f& value);
template variable_status console::set_from_value<math::vec4i>(const string& name, const math::vec4i& value);
template variable_status console::set_from_value<math::vec3f>(const string& name, const math::vec3f& value);
template variable_status console::set_from_value<math::vec3i>(const string& name, const math::vec3i& value);
template variable_status console::set_from_value<math::vec2f>(const string& name, const math::vec2f& value);
template variable_status console::set_from_value<math::vec2i>(const string& name, const math::vec2i& value);

variable_status console::change(const string& name, const string& value) {
  for (variable_reference* head{g_head}; head; head = head->m_next) {
    if (head->name() != name) {
      continue;
    }

    switch (head->type()) {
    case variable_type::k_boolean:
      return set_from_string<bool>(name, value);
    case variable_type::k_int:
      return set_from_string<rx_s32>(name, value);
    case variable_type::k_float:
      return set_from_string<rx_f32>(name, value);
    case variable_type::k_string:
      return set_from_string<string>(name, value);
    case variable_type::k_vec4f:
      return set_from_string<math::vec4f>(name, value);
    case variable_type::k_vec4i:
      return set_from_string<math::vec4i>(name, value);
    case variable_type::k_vec3f:
      return set_from_string<math::vec3f>(name, value);
    case variable_type::k_vec3i:
      return set_from_string<math::vec3i>(name, value);
    case variable_type::k_vec2f:
      return set_from_string<math::vec2f>(name, value);
    case variable_type::k_vec2i:
      return set_from_string<math::vec2i>(name, value);
      break;
    }
  }
  return variable_status::k_not_found;
}

variable_reference* console::add_variable_reference(variable_reference* reference) {
  console_print(rx::log::level::k_info, "registered '%s'", reference->m_name);
  scope_lock<spin_lock> locked(g_lock);
  variable_reference* next = g_head;
  g_head = reference;
  return next;
}

variable_reference* console::split(variable_reference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  variable_reference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

variable_reference* console::merge(variable_reference* lhs, variable_reference* rhs) {
  if (!lhs) {
    return rhs;
  }

  if (!rhs) {
    return lhs;
  }

  if (strcmp(lhs->m_name, rhs->m_name) > 0) {
    rhs->m_next = merge(lhs, rhs->m_next);
    return rhs;
  }

  lhs->m_next = merge(lhs->m_next, rhs);
  return lhs;
}

variable_reference* console::sort(variable_reference* reference) {
  if (!reference) {
    return nullptr;
  }

  if (!reference->m_next) {
    return reference;
  }

  variable_reference* splitted = split(reference);

  return merge(sort(reference), sort(splitted));
}

} // namespace rx::console
