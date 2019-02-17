#ifndef RX_CORE_STRING_H
#define RX_CORE_STRING_H

#include <rx/core/types.h> // rx_size
#include <rx/core/assert.h> // RX_ASSERT

#include <rx/core/memory/block.h> // block
#include <rx/core/memory/allocator.h> // allocator

namespace rx {

// 48 bytes
struct string {
  static constexpr const rx_size k_npos{-1_z};
  static constexpr const rx_size k_small_string{16};

  string(memory::allocator* alloc);
  string(memory::allocator* alloc, const string& contents);
  string(memory::allocator* alloc, const char* contents);
  string(memory::allocator* alloc, const char* contents, rx_size size);
  string(memory::allocator* alloc, const char* begin, const char* end);
  string(const string& contents);
  string(string&& contents);
  ~string();

  string& operator=(const string& contents);
  string& operator=(string&& contents);

  void reserve(rx_size size);
  void resize(rx_size size);

  rx_size size() const;
  bool empty() const;
  void clear();

  string& append(const char* first, const char* last);
  string& append(const char* contents, rx_size size);
  string& append(const char* contents);
  string& append(const string& contents);
  string& append(char ch);

  char pop_back();

  char& operator[](rx_size index);
  const char& operator[](rx_size index) const;

  char* data();
  const char* data() const;

private:
  void swap(string& other);

  memory::allocator* m_allocator;
  memory::block m_data;
  rx_byte* m_last;
  rx_byte m_buffer[k_small_string] = {0};
};

inline rx_size string::size() const {
  return m_last - m_data.data();
}

inline bool string::empty() const {
  return m_last - m_data.data();
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
  return m_data.cast<char*>()[index];
}

inline const char& string::operator[](rx_size index) const {
  // NOTE(dweiler): <= is not a bug, indexing the null-terminator is allowed
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data.cast<char*>()[index];
}

inline char* string::data() {
  return m_data.cast<char*>();
}

inline const char* string::data() const {
  return m_data.cast<const char*>();
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

} // namespace rx

#endif // RX_CORE_STRING_H
