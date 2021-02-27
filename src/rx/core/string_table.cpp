#include <string.h>

#include "rx/core/string_table.h"

namespace Rx {

// [StringTable]
Optional<StringTable> StringTable::create_from_linear_buffer(LinearBuffer&& linear_buffer_) {
  StringTable result;

  // Enumerate each string in |linear_buffer_| and append an entry to
  // |result.m_string_set|. The contents of the LinearBuffer is just a bunch
  // of null-terminated strings.
  const Size size = linear_buffer_.size();
  for (Size i = 0; i < size; i++) {
    // When the StringTable is ultimately moved the SharedString table pointer
    // will be fixed up by |update_table_references|. Just put nullptr in
    // the SharedStrings for now.
    if (!result.m_string_set.insert({i, nullptr})) {
      return nullopt;
    }
    // Skip this string in the table. The `i++` in the for loop will skip
    // the null-byte.
    i += strlen(reinterpret_cast<const char *>(linear_buffer_.data() + i));
  }

  result.m_string_data = Utility::move(linear_buffer_);

  return result;
}

Optional<Size> StringTable::add(const char* _string) {
  return add(_string, strlen(_string));
}

Optional<Size> StringTable::add(const char* _string, Size _length) {
  if (auto search = find(_string)) {
    return *search;
  }

  const auto* base =
    reinterpret_cast<const Byte*>(m_string_data.data());

  // Search the contents with memmem for |_string| as it may be an overlapping
  // string which we can save on space in the StringTable for.
  if (const auto search = memmem(base, m_string_data.size(), _string, _length)) {
    const auto cursor = reinterpret_cast<const Byte*>(search);
    if (!m_string_set.insert({Size(cursor - base), this})) {
      return nullopt;
    }
  }

  // Remember to always include the null-terminator on insertions.
  return insert(_string, _length + 1);
}

// [StringTable::SharedString]
const char* StringTable::SharedString::as_string() const {
  return reinterpret_cast<const char*>(table->m_string_data.data() + offset);
}

Optional<StringTable::SharedString>
StringTable::SharedString::create(StringTable* table_, const char* _string, Size _length) {
  auto& string_data = table_->m_string_data;
  auto& string_set = table_->m_string_set;

  const auto offset = string_data.size();

  SharedString shared{offset, table_};

  if (!string_set.insert(shared)) {
    return nullopt;
  }

  if (!string_data.resize(offset + _length)) {
    string_set.erase(shared);
    return nullopt;
  }

  memcpy(string_data.data() + offset, _string, _length);

  return shared;
}

bool StringTable::SharedString::operator==(const char* _string) const {
  return strcmp(as_string(), _string) == 0;
}

bool StringTable::SharedString::operator==(const SharedString& _string) const {
  return strcmp(as_string(), _string.as_string()) == 0;
}

Size StringTable::SharedString::hash() const {
  return Hash::string(as_string());
}

} // namespace Rx