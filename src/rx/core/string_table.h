#ifndef RX_CORE_STRING_TABLE_H
#define RX_CORE_STRING_TABLE_H
#include "rx/core/linear_buffer.h"
#include "rx/core/set.h"

#include "rx/core/hash/string.h"

namespace Rx {

struct String;

struct StringTable {
  RX_MARK_NO_COPY(StringTable);

  constexpr StringTable();
  constexpr StringTable(Memory::Allocator& _allocator);
  StringTable(StringTable&& string_table_);

  static Optional<StringTable> create_from_linear_buffer(LinearBuffer&& linear_buffer_);

  StringTable& operator=(StringTable&& string_table_);

  Optional<Size> add(const char* _string);
  Optional<Size> add(const char* _string, Size _length);

  Optional<Size> find(const char* _string);

  const LinearBuffer& data() const &;

  const char* operator[](Size _index) const;

  void clear();

private:
  void update_table_references();

  Optional<Size> insert(Span<const char> _span);

  struct SharedString {
    Size offset;
    StringTable* table;
    const char* as_string() const;
    static Optional<SharedString> create(StringTable* table_, Span<const char> _span);
    bool operator==(const char* _string) const;
    bool operator==(const SharedString& _string) const;
    Size hash() const;
  };

  LinearBuffer m_string_data;
  Set<SharedString> m_string_set;
};

// [StringTable]
inline constexpr StringTable::StringTable()
  : StringTable{Memory::SystemAllocator::instance()}
{
}

inline constexpr StringTable::StringTable(Memory::Allocator& _allocator)
  : m_string_data{_allocator}
  , m_string_set{_allocator}
{
}

inline StringTable::StringTable(StringTable&& string_table_)
  : m_string_data{Utility::move(string_table_.m_string_data)}
  , m_string_set{Utility::move(string_table_.m_string_set)}
{
  update_table_references();
}

inline StringTable& StringTable::operator=(StringTable&& string_table_) {
  m_string_data = Utility::move(string_table_.m_string_data);
  m_string_set = Utility::move(string_table_.m_string_set);
  update_table_references();
  return *this;
}

inline Optional<Size> StringTable::find(const char* _string) {
  if (auto search = m_string_set.find(_string)) {
    return search->offset;
  }
  return nullopt;
}

inline const LinearBuffer& StringTable::data() const & {
  return m_string_data;
}

inline const char* StringTable::operator[](Size _index) const {
  RX_ASSERT(_index < m_string_data.size(), "out of bounds");
  return reinterpret_cast<const char*>(m_string_data.data() + _index);
}

inline void StringTable::clear() {
  m_string_data.clear();
  m_string_set.clear();
}

inline void StringTable::update_table_references() {
  m_string_set.each([this](SharedString& shared_string_) {
    shared_string_.table = this;
  });
}

} // namespace Rx

#endif // RX_CORE_STRING_TABLE_H