#include <string.h> // strcmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errno, ERANGE

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/log.h" // RX_LOG

namespace rx::console {

static concurrency::spin_lock g_lock;
static variable_reference* g_head; // protected by |g_lock|

RX_LOG("console", console_print);

bool interface::load(const char* file_name) {
  // sort references
  {
    concurrency::scope_lock locked(g_lock);
    g_head = sort(g_head);
  }

  filesystem::file file(file_name, "r");
  if (!file) {
    return false;
  }

  console_print(log::level::k_info, "loading '%s'", file_name);
  for (string line_contents; file.read_line(line_contents); ) {
    string line{line_contents.lstrip(" \t")};
    if (line.is_empty() || strchr("#;[", line[0])) {
      // ignore empty and comment lines
      continue;
    }

    // tokenize line contents
    array<string> tokens{line.split(' ', 2)};
    variable_status status{change(tokens[0], tokens[1])};
    switch (status) {
    case variable_status::k_malformed:
      console_print(log::level::k_error, "'%s' malformed", tokens[0]);
      break;
    case variable_status::k_not_found:
      console_print(log::level::k_error, "'%s' not found", tokens[0]);
      break;
    case variable_status::k_out_of_range:
      console_print(log::level::k_error, "'%s' out of range", tokens[0]);
      break;
    case variable_status::k_success:
      console_print(log::level::k_info, "'%s' changed to '%s'", tokens[0], tokens[1]);
      break;
    case variable_status::k_type_mismatch:
      console_print(log::level::k_error, "'%s' type mismatch", tokens[0]);
      break;
    }
  }

  return true;
}

bool interface::save(const char* file_name) {
  filesystem::file file(file_name, "w");
  if (!file) {
    return false;
  }

  console_print(log::level::k_info, "saving '%s'", file_name);
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
      const auto min_fmt{min == k_int_min ? "-inf" : string::format("%d", min)};
      const auto max_fmt{max == k_int_max ? "+inf" : string::format("%d", max)};
      file.print("## %s (in range [%s, %s], defaults to %d)\n",
        head->description(), min_fmt, max_fmt, handle->initial());
      file.print(handle->get() == handle->initial() ? ";%s %d\n" : "%s %d\n",
        head->name(), handle->get());
    } else if (head->type() == variable_type::k_float) {
      const auto handle{head->cast<rx_f32>()};
      const auto min{handle->min()};
      const auto max{handle->max()};
      const auto min_fmt{min == k_float_min ? "-inf" : string::format("%f", min)};
      const auto max_fmt{max == k_float_max ? "+inf" : string::format("%f", max)};
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
        const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
        const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
        const auto min_z{min.z == k_float_min ? "-inf" : string::format("%f", min.z)};
        const auto min_w{min.w == k_float_min ? "-inf" : string::format("%f", min.w)};
        const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
        const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
        const auto max_z{max.z == k_float_max ? "+inf" : string::format("%f", max.z)};
        const auto max_w{max.w == k_float_max ? "+inf" : string::format("%f", max.w)};
        min_fmt = string::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
        max_fmt = string::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
        const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
        const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
        const auto min_z{min.z == k_int_min ? "-inf" : string::format("%d", min.z)};
        const auto min_w{min.w == k_int_min ? "-inf" : string::format("%d", min.w)};
        const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
        const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
        const auto max_z{max.z == k_int_max ? "+inf" : string::format("%d", max.z)};
        const auto max_w{min.w == k_int_min ? "+inf" : string::format("%d", max.w)};
        min_fmt = string::format("{%s, %s, %s, %s}", min_x, min_y, min_z, min_w);
        max_fmt = string::format("{%s, %s, %s, %s}", max_x, max_y, max_z, max_w);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
        const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
        const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
        const auto min_z{min.z == k_float_min ? "-inf" : string::format("%f", min.z)};
        const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
        const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
        const auto max_z{max.z == k_float_max ? "+inf" : string::format("%f", max.z)};
        min_fmt = string::format("{%s, %s, %s}", min_x, min_y, min_z);
        max_fmt = string::format("{%s, %s, %s}", max_x, max_y, max_z);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
        const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
        const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
        const auto min_z{min.z == k_int_min ? "-inf" : string::format("%d", min.z)};
        const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
        const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
        const auto max_z{max.z == k_int_max ? "+inf" : string::format("%d", max.z)};
        min_fmt = string::format("{%s, %s, %s}", min_x, min_y, min_z);
        max_fmt = string::format("{%s, %s, %s}", max_x, max_y, max_z);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
        const auto min_x{min.x == k_float_min ? "-inf" : string::format("%f", min.x)};
        const auto min_y{min.y == k_float_min ? "-inf" : string::format("%f", min.y)};
        const auto max_x{max.x == k_float_max ? "+inf" : string::format("%f", max.x)};
        const auto max_y{max.y == k_float_max ? "+inf" : string::format("%f", max.y)};
        min_fmt = string::format("{%s, %s}", min_x, min_y);
        max_fmt = string::format("{%s, %s}", max_x, max_y);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
        const auto min_x{min.x == k_int_min ? "-inf" : string::format("%d", min.x)};
        const auto min_y{min.y == k_int_min ? "-inf" : string::format("%d", min.y)};
        const auto max_x{max.x == k_int_max ? "+inf" : string::format("%d", max.x)};
        const auto max_y{max.y == k_int_max ? "+inf" : string::format("%d", max.y)};
        min_fmt = string::format("{%s, %s}", min_x, min_y);
        max_fmt = string::format("{%s, %s}", max_x, max_y);
      } else {
        min_fmt = string::format("%s", min);
        max_fmt = string::format("%s", max);
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
variable_status interface::parse_string<bool>(const string& contents, bool& result) {
  if (contents == "true" || contents == "false") {
    result = contents == "true";
    return variable_status::k_success;
  }
  return variable_status::k_type_mismatch;
}

template<>
variable_status interface::parse_string<string>(const string& contents, string& result) {
  result = contents;
  return variable_status::k_success;
}

template<>
variable_status interface::parse_string<rx_s32>(const string& contents, rx_s32& result) {
  const char* input{contents.data()};
  char* scan{nullptr};

  const auto value{strtoll(input, &scan, 10)};

  if (errno == ERANGE) {
    return variable_status::k_out_of_range;
  }

  if (scan == input || *scan != '\0') {
    return variable_status::k_malformed;
  }

  if (value < k_int_min || value > k_int_max) {
    return variable_status::k_out_of_range;
  }

  result = static_cast<rx_s32>(value);
  return variable_status::k_success;
}

template<>
variable_status interface::parse_string<rx_f32>(const string& contents, rx_f32& result) {
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
variable_status interface::parse_string<math::vec4f>(const string& contents, math::vec4f& result) {
  math::vec4f value;
  if (contents.scan("{%f, %f, %f, %f}", &value.x, &value.y, &value.z, &value.w) != 4) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status interface::parse_string<math::vec4i>(const string& contents, math::vec4i& result) {
  math::vec4i value;
  if (contents.scan("{%d, %d, %d, %d}", &value.x, &value.y, &value.z, &value.w) != 4) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

// vec3
template<>
variable_status interface::parse_string<math::vec3f>(const string& contents, math::vec3f& result) {
  math::vec3f value;
  if (contents.scan("{%f, %f, %f}", &value.x, &value.y, &value.z) != 3) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status interface::parse_string<math::vec3i>(const string& contents, math::vec3i& result) {
  math::vec3i value;
  if (contents.scan("{%d, %d, %d}", &value.x, &value.y, &value.z) != 3) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

// vec2
template<>
variable_status interface::parse_string<math::vec2f>(const string& contents, math::vec2f& result) {
  math::vec2f value;
  if (contents.scan("{%f, %f}", &value.x, &value.y) != 2) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<>
variable_status interface::parse_string<math::vec2i>(const string& contents, math::vec2i& result) {
  math::vec2i value;
  if (contents.scan("{%d, %d}", &value.x, &value.y) != 2) {
    return variable_status::k_malformed;
  }
  result = value;
  return variable_status::k_success;
}

template<typename T>
variable_status interface::set_from_name_and_string(const string& name, const string& value_string) {
  T value;
  variable_status status;
  if ((status = parse_string<T>(value_string, value)) == variable_status::k_success) {
    return set_from_name_and_value<T>(name, value);
  }
  return status;
}

template variable_status interface::set_from_name_and_string<bool>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<string>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<rx_s32>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<rx_f32>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec4f>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec4i>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec3f>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec3i>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec2f>(const string& _name, const string& _string_value);
template variable_status interface::set_from_name_and_string<math::vec2i>(const string& _name, const string& _string_value);

template<typename T>
variable_status interface::set_from_name_and_value(const string& _name, const T& _value) {
  for (variable_reference* head{g_head}; head; head = head->m_next) {
    if (head->name() == _name) {
      if (head->type() != variable_trait<T>::type) {
        return variable_status::k_type_mismatch;
      }
      return set_from_reference_and_value(head, _value);
    }
  }
  return variable_status::k_not_found;
}

template variable_status interface::set_from_name_and_value<bool>(const string& _name, const bool& _value);
template variable_status interface::set_from_name_and_value<string>(const string& _name, const string& _value);
template variable_status interface::set_from_name_and_value<rx_s32>(const string& _name, const rx_s32& _value);
template variable_status interface::set_from_name_and_value<rx_f32>(const string& _name, const rx_f32& _value);
template variable_status interface::set_from_name_and_value<math::vec4f>(const string& _name, const math::vec4f& _value);
template variable_status interface::set_from_name_and_value<math::vec4i>(const string& _name, const math::vec4i& _value);
template variable_status interface::set_from_name_and_value<math::vec3f>(const string& _name, const math::vec3f& _value);
template variable_status interface::set_from_name_and_value<math::vec3i>(const string& _name, const math::vec3i& _value);
template variable_status interface::set_from_name_and_value<math::vec2f>(const string& _name, const math::vec2f& _value);
template variable_status interface::set_from_name_and_value<math::vec2i>(const string& _name, const math::vec2i& _value);

template<typename T>
variable_status interface::set_from_reference_and_value(variable_reference* _reference, const T& _value) {
  return _reference->cast<T>()->set(_value);
}

template variable_status interface::set_from_reference_and_value<bool>(variable_reference* _reference, const bool& _value);
template variable_status interface::set_from_reference_and_value<string>(variable_reference* _reference, const string& _value);
template variable_status interface::set_from_reference_and_value<rx_s32>(variable_reference* _reference, const rx_s32& _value);
template variable_status interface::set_from_reference_and_value<rx_f32>(variable_reference* _reference, const rx_f32& _value);
template variable_status interface::set_from_reference_and_value<math::vec4f>(variable_reference* _reference, const math::vec4f& _value);
template variable_status interface::set_from_reference_and_value<math::vec4i>(variable_reference* _reference, const math::vec4i& _value);
template variable_status interface::set_from_reference_and_value<math::vec3f>(variable_reference* _reference, const math::vec3f& _value);
template variable_status interface::set_from_reference_and_value<math::vec3i>(variable_reference* _reference, const math::vec3i& _value);
template variable_status interface::set_from_reference_and_value<math::vec2f>(variable_reference* _reference, const math::vec2f& _value);
template variable_status interface::set_from_reference_and_value<math::vec2i>(variable_reference* _reference, const math::vec2i& _value);


template<typename T>
variable_status interface::set_from_reference_and_string(variable_reference* _reference, const string& _value_string) {
  T value;
  variable_status status;
  if ((status = parse_string<T>(_value_string, value)) == variable_status::k_success) {
    return set_from_reference_and_value(_reference, value);
  }
  return status;
}

template variable_status interface::set_from_reference_and_string<bool>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<string>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<rx_s32>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<rx_f32>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec4f>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec4i>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec3f>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec3i>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec2f>(variable_reference* _reference, const string& _string_value);
template variable_status interface::set_from_reference_and_string<math::vec2i>(variable_reference* _reference, const string& _string_value);

variable_status interface::change(const string& _name, const string& _string_value) {
  for (variable_reference* head{g_head}; head; head = head->m_next) {
    if (head->name() != _name) {
      continue;
    }

    switch (head->type()) {
    case variable_type::k_boolean:
      return set_from_reference_and_string<bool>(head, _string_value);
    case variable_type::k_int:
      return set_from_reference_and_string<rx_s32>(head, _string_value);
    case variable_type::k_float:
      return set_from_reference_and_string<rx_f32>(head, _string_value);
    case variable_type::k_string:
      return set_from_reference_and_string<string>(head, _string_value);
    case variable_type::k_vec4f:
      return set_from_reference_and_string<math::vec4f>(head, _string_value);
    case variable_type::k_vec4i:
      return set_from_reference_and_string<math::vec4i>(head, _string_value);
    case variable_type::k_vec3f:
      return set_from_reference_and_string<math::vec3f>(head, _string_value);
    case variable_type::k_vec3i:
      return set_from_reference_and_string<math::vec3i>(head, _string_value);
    case variable_type::k_vec2f:
      return set_from_reference_and_string<math::vec2f>(head, _string_value);
    case variable_type::k_vec2i:
      return set_from_reference_and_string<math::vec2i>(head, _string_value);
      break;
    }
  }
  return variable_status::k_not_found;
}

variable_reference* interface::get_from_name(const string& _name) {
  for (variable_reference* head{g_head}; head; head = head->m_next) {
    if (head->name() != _name) {
      continue;
    }

    return head;
  }

  return nullptr;
}

variable_reference* interface::add_variable_reference(variable_reference* reference) {
  console_print(log::level::k_info, "registered '%s'", reference->m_name);
  concurrency::scope_lock locked(g_lock);
  variable_reference* next = g_head;
  g_head = reference;
  return next;
}

variable_reference* interface::split(variable_reference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  variable_reference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

variable_reference* interface::merge(variable_reference* lhs, variable_reference* rhs) {
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

variable_reference* interface::sort(variable_reference* reference) {
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
