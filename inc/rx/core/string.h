#ifndef RX_CORE_STRING_H
#define RX_CORE_STRING_H

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/format.h> // format
#include <rx/core/array.h> // array

#include <rx/core/memory/system_allocator.h> // memory::{system_allocator, allocator, block}

#include <rx/core/traits/remove_cvref.h>

namespace rx {

// 32-bit: 32 bytes
// 64-bit: 48 bytes
struct string {
  static constexpr const rx_size k_npos{-1_z};
  static constexpr const rx_size k_small_string{16};

  string();
  string(const string& _contents);
  string(const char* _contents);
  string(const char* _contents, rx_size _size);
  template<typename... Ts>
  string(const char* _format, Ts&&... _arguments);

  string(memory::allocator* _allocator);
  string(memory::allocator* _allocator, const string& _contents);
  string(memory::allocator* _allocator, const char* _contents);
  string(memory::allocator* _allocator, const char* _contents, rx_size _size);
  template<typename... Ts>
  string(memory::allocator* _allocator, const char* _format, Ts&&... _arguments);

  string(string&& _contents);

  ~string();

  string& operator=(const string& contents);
  string& operator=(string&& contents);

  void reserve(rx_size size);
  void resize(rx_size size);

  rx_size size() const;
  bool is_empty() const;
  void clear();

  string& append(const char* first, const char* last);
  string& append(const char* contents, rx_size size);
  string& append(const char* contents);
  string& append(const string& contents);
  string& append(char ch);

  // returns copy of string with leading characters in set removed
  string lstrip(const char* set) const;

  // returns copy of string with trailing characters in set removed
  string rstrip(const char* set) const;

  // split string by |token| up to |count| times, use |count| of zero for no limit
  array<string> split(int ch, rx_size count = 0) const;

  // take substring from |offset| of |length|, use |length| of zero for whole string
  string substring(rx_size offset, rx_size length = 0) const;

  // scan string
  rx_size scan(const char* scan_format, ...) const;

  char pop_back();

  char& operator[](rx_size index);
  const char& operator[](rx_size index) const;

  char& last();
  const char& last() const;

  char* data();
  const char* data() const;

  static string human_size_format(rx_size _size);

private:
  static string formatter(memory::allocator* alloc, const char* fmt, ...);

  void swap(string& other);

  memory::allocator* m_allocator;
  char* m_data;
  char* m_last;
  char* m_capacity;
  char m_buffer[k_small_string] = {0};
};

// utf-16, Windows compatible "wide-string"
struct wide_string {
  // constructors that use system allocator
  wide_string();
  wide_string(const wide_string& _other);
  wide_string(const rx_u16* _contents);
  wide_string(const rx_u16* _contents, rx_size _size);
  wide_string(const char* _contents, rx_size _size);
  wide_string(const char* _contents);
  wide_string(const string& _contents);

  // custom allocator versions
  wide_string(memory::allocator* _allocator);
  wide_string(memory::allocator* _allocator, const wide_string& _other);
  wide_string(memory::allocator* _allocator, const rx_u16* _contents);
  wide_string(memory::allocator* _allocator, const rx_u16* _contents, rx_size _size);
  wide_string(memory::allocator* _allocator, const char* _contents, rx_size _size);
  wide_string(memory::allocator* _allocator, const char* _contents);
  wide_string(memory::allocator* _allocator, const string& _contents);

  wide_string(wide_string&& _other);

  ~wide_string();

  // disable all assignment operators because you're not supposed to use wide_string
  // for any other purpose than to convert string (which is utf8) to utf16 for
  // interfaces expecting that, such as the ones on Windows
  wide_string& operator=(const wide_string&) = delete;
  wide_string& operator=(const rx_u16*) = delete;
  wide_string& operator=(const char*) = delete;
  wide_string& operator=(const string&) = delete;

  rx_size size() const;
  bool is_empty() const;

  rx_u16& operator[](rx_size _index);
  const rx_u16& operator[](rx_size _index) const;

  rx_u16* data();
  const rx_u16* data() const;

private:
  memory::allocator* m_allocator;
  rx_u16* m_data;
  rx_size m_size;
};

// hash function for string
template<typename T>
struct hash;

template<>
struct hash<string> {
  rx_size operator()(const string& contents) const {
    // djb2
    rx_size value{5381};
    for (const char *ch{contents.data()}; *ch; ch++) {
      value = ((value << 5) + value) + *ch;
    }
    return value;
  }
};

// format function for string
template<>
struct format<string> {
  const char* operator()(const string& value) const {
    return value.data();
  }
};

template<typename... Ts>
inline string::string(memory::allocator* _allocator, const char* _format, Ts&&... _arguments)
  : string{utility::move(formatter(_allocator, _format, format<traits::remove_cvref<Ts>>{}(utility::forward<Ts>(_arguments))...))}
{
}

inline string::string()
  : string{&memory::g_system_allocator}
{
}

inline string::string(const string& _contents)
  : string{_contents.m_allocator, _contents}
{
}

inline string::string(const char* _contents)
  : string{&memory::g_system_allocator, _contents}
{
}

inline string::string(const char* _contents, rx_size _size)
  : string{&memory::g_system_allocator, _contents, _size}
{
}

template<typename... Ts>
inline string::string(const char* _format, Ts&&... _arguments)
  : string{&memory::g_system_allocator, _format, utility::forward<Ts>(_arguments)...}
{
}

inline rx_size string::size() const {
  return m_last - m_data;
}

inline bool string::is_empty() const {
  return m_last - m_data == 0;
}

inline void string::clear() {
  resize(0);
}

inline string& string::append(const char* contents, rx_size size) {
  return append(contents, contents + size);
}

inline string& string::append(const string& contents) {
  return append(contents.data(), contents.size());
}

inline string& string::append(char ch) {
  return append(&ch, 1);
}

inline char& string::operator[](rx_size index) {
  // NOTE(dweiler): <= is not a bug, indexing the null-terminator is allowed
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

inline const char& string::operator[](rx_size index) const {
  // NOTE(dweiler): <= is not a bug, indexing the null-terminator is allowed
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

inline char& string::last() {
  return m_data[size() - 1];
}

inline const char& string::last() const {
  return m_data[size() - 1];
}

inline char* string::data() {
  return m_data;
}

inline const char* string::data() const {
  return m_data;
}

inline string operator+(const string& lhs, const char* rhs) {
  return string(lhs).append(rhs);
}

inline string operator+(const string& lhs, const string& rhs) {
  return string(lhs).append(rhs);
}

inline string& operator+=(string &lhs, const char* rhs) {
  lhs.append(rhs);
  return lhs;
}

inline string& operator+=(string &lhs, char ch) {
  lhs.append(ch);
  return lhs;
}

// not inlined since it would explode code size
bool operator==(const string& lhs, const string& rhs);
bool operator!=(const string& lhs, const string& rhs);
bool operator<(const string& lhs, const string& rhs);
bool operator>(const string& lhs, const string& rhs);

// wide_string
inline wide_string::wide_string()
  : wide_string{&memory::g_system_allocator}
{
}

inline wide_string::wide_string(const rx_u16* _contents)
  : wide_string{&memory::g_system_allocator, _contents}
{
}

inline wide_string::wide_string(const rx_u16* _contents, rx_size _size)
  : wide_string{&memory::g_system_allocator, _contents, _size}
{
}

inline wide_string::wide_string(const char* _contents, rx_size _size)
  : wide_string{&memory::g_system_allocator, _contents, _size}
{
}

inline wide_string::wide_string(const char* _contents)
  : wide_string{&memory::g_system_allocator, _contents}
{
}

inline wide_string::wide_string(const string& _contents)
  : wide_string{&memory::g_system_allocator, _contents}
{
}

inline wide_string::wide_string(const wide_string& _other)
  : wide_string{_other.m_allocator, _other}
{
}

inline rx_size wide_string::size() const {
  return m_size;
}

inline bool wide_string::is_empty() const {
  return m_size == 0;
}

inline rx_u16& wide_string::operator[](rx_size _index) {
  RX_ASSERT(_index < m_size, "out of bounds");
  return m_data[_index];
}

inline const rx_u16& wide_string::operator[](rx_size _index) const {
  RX_ASSERT(_index < m_size, "out of bounds");
  return m_data[_index];
}

inline rx_u16* wide_string::data() {
  return m_data;
}

inline const rx_u16* wide_string::data() const {
  return m_data;
}

} // namespace rx

#endif // RX_CORE_STRING_H
