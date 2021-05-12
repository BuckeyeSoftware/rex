#include <string.h>

#include "rx/core/string_table.h"
#include "rx/core/memory/search.h"

namespace Rx {

// [StringTable]
Optional<StringTable> StringTable::create_from_linear_buffer(LinearBuffer&& linear_buffer_) {
  StringTable result{linear_buffer_.allocator()};

  // Enumerate each string in |linear_buffer_| and append an entry to
  // |result.m_string_set|. The contents of the LinearBuffer is just a bunch
  // of null-terminated strings.
  const Size size = linear_buffer_.size();
  for (Size i = 0; i < size; i++) {
    // When the StringTable is ultimately moved the SharedString table pointer
    // will be fixed up by |update_table_references|. Just put nullptr in
    // the SharedStrings for now.
    if (!result.m_string_set.insert({i, &result})) {
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
  // Remember to always include the null-terminator on insertions.
  return insert({_string, strlen(_string) + 1});
}

Optional<Size> StringTable::add(const char* _string, Size _length) {
  // Remember to always include the null-terminator on insertions.
  return insert({_string, _length + 1});
}

Optional<Size> StringTable::insert(Span<const char> _span) {
  if (auto search = find(_span.data())) {
    return *search;
  }

  const auto* base =
    reinterpret_cast<const Byte*>(m_string_data.data());
  
  // Search the contents with memmem for |_string| as it may be an overlapping
  // string which we can save on space in the StringTable for.
  if (const auto search = Memory::search(base, m_string_data.size(), _span.data(), _span.size())) {
    const auto cursor = reinterpret_cast<const Byte*>(search);
    if (!m_string_set.insert({Size(cursor - base), this})) {
      return nullopt;
    }
  }

  if (auto shared = SharedString::create(this, _span)) {
    return shared->offset;
  }

  return nullopt;
}

// [StringTable::SharedString]
const char* StringTable::SharedString::as_string() const {
  return reinterpret_cast<const char*>(table->m_string_data.data() + offset);
}

Optional<StringTable::SharedString>
StringTable::SharedString::create(StringTable* table_, Span<const char> _span) {
  auto& string_data = table_->m_string_data;
  auto& string_set = table_->m_string_set;

  const auto offset = string_data.size();

  SharedString shared{offset, table_};

  if (!string_data.resize(offset + _span.size())) {
    return nullopt;
  }

  memcpy(string_data.data() + offset, _span.data(), _span.size());

  if (!string_set.insert(shared)) {
    // This cannot fail, only ever shrinks.
    (void)string_data.resize(offset);
    return nullopt;
  }

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