#ifndef RX_CORE_STRING_H
#define RX_CORE_STRING_H
#include "rx/core/optional.h"
#include "rx/core/memory/system_allocator.h"

namespace Rx {

struct WideString;
struct StringView;

// 32-bit: 16 + INSITU_SIZE bytes
// 64-bit: 32 + INSITU_SIZE bytes
struct RX_API String {
  /// The number of characters to store in-situ before allocations are needed.
  static inline constexpr const Size INSITU_SIZE = 16;

  constexpr String();
  constexpr String(Memory::Allocator& _allocator);

  // TODO(dweiler): Remove copy constructor as this can fail.
  String(const String& _contents);

  String(String&& contents_);
  String(Memory::View _view);
  ~String();

  /// @{
  /// Create a string
  RX_API static Optional<String> create(Memory::Allocator& _allocator, const char* _contents);
  RX_API static Optional<String> create(Memory::Allocator& _allocator, const char* _contents, Size _size);
  /// @}

  /// Copy a string.
  /// \param _other The string to copy.
  RX_API static Optional<String> copy(const String& _other);

  // TODO(dweiler): Have this return Optional<String> since it can fail.
  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  RX_API static String format(Memory::Allocator& _allocator, const char* _format,
    Ts&&... _arguments);

  /// Move assignment operator.
  String& operator=(String&& contents_);

  /// Reserve storage in the string.
  [[nodiscard]] bool reserve(Size _size);

  /// Resize the string.
  [[nodiscard]] bool resize(Size _size);

  /// @{
  /// Find the first instance of a substring.
  Optional<Size> find_first_of(int _ch) const;
  Optional<Size> find_first_of(const StringView& _contents) const;
  /// @}

  /// @{
  /// Find the last instance of a substring.
  Optional<Size> find_last_of(int _ch) const;
  Optional<Size> find_last_of(const StringView& _contents) const;
  /// @}

  /// Size of the string.
  Size size() const;

  /// Capacity of the string.
  Size capacity() const;

  /// Check if the string is stored in-situ or is backed by an allocation.
  bool in_situ() const;

  /// Check if the string is empty.
  bool is_empty() const;

  /// Clear a string.
  void clear();

  /// @{
  /// Append a string.
  [[nodiscard]] bool append(const char* _first, const char* _last);
  [[nodiscard]] bool append(const char* _contents, Size _size);
  [[nodiscard]] bool append(const StringView& _contents);
  [[nodiscard]] bool append(char _ch);
  /// @}

  /// Format a string and append it.
  /// \param _format The format string.
  /// \param _arguments The format arguments.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool formatted_append(const char* _format, Ts&&... _arguments);

  /// @{
  /// Insert a string at a position.
  [[nodiscard]] bool insert_at(Size _position, const char* _contents, Size _size);
  [[nodiscard]] bool insert_at(Size _position, const char* _contents);
  [[nodiscard]] bool insert_at(Size _position, const String& _contents);
  /// @}

  /// Construct a substring.
  Optional<String> substring(Size _offset, Size _length = 0) const;

  /// Scan the string into values.
  Size scan(const char* _scan_format, ...) const;

  /// Pop back the last codepoint.
  char pop_back();

  /// Erase a range of codepoints.
  void erase(Size _begin, Size _end);

  /// @{
  /// Access a codepoint by index.
  char& operator[](Size _index);
  const char& operator[](Size _index) const;
  /// @}

  /// @{
  /// Access the raw string data, null-terminated.
  char* data();
  const char* data() const;
  /// @}

  static RX_API Optional<String> human_size_format(Memory::Allocator& _allocator, Size _size);

  /// Check if string begins with a given prefix.
  [[nodiscard]] bool begins_with(const StringView& _prefix) const;

  /// Check if string ends with a given suffix.
  [[nodiscard]] bool ends_with(const StringView& _suffix) const;

  /// Check if string contains a substring.
  [[nodiscard]] bool contains(const StringView& _needle) const;

  /// Compute a hash code.
  Size hash() const;

  /// Convert the string to UTF-16.
  WideString to_utf16() const;

  /// The allocator used for this string.
  constexpr Memory::Allocator& allocator() const;

  /// Disown the memory of the string.
  Optional<Memory::View> disown();

  /// @{
  /// Non-owning access to the raw codepoints.
  Span<char> span();
  Span<const char> span() const;
  /// @}

  /// Read a line of text from a string.
  /// \param data_ The data to read a line from.
  ///
  /// Modifies the contents pointed by \p data_ to null-terminate at each
  /// newline character and advances the pointer by the line length, returning
  /// a pointer to the line.
  ///
  /// The correct way to use this is in a loop reading lines like:
  /// \code{cpp}
  /// char* data = ...;
  /// char* line = data;
  /// while ((line = String::read_line(line))) {
  ///   ...
  /// }
  /// \endcode
  RX_API static char *read_line(char*& data_);

private:
  static RX_API Optional<String> formatter(Memory::Allocator& _allocator, const char* _format, ...);

  void swap(String& other);

  Memory::Allocator* m_allocator;
  char* m_data;
  char* m_last;
  char* m_capacity;

  char m_buffer[INSITU_SIZE];
};

// utf-16, Windows compatible "wide-string"
struct WideString {
  RX_MARK_NO_MOVE_ASSIGN(WideString);

  // Custom allocator versions.
  WideString(Memory::Allocator& _allocator);
  WideString(Memory::Allocator& _allocator, const WideString& _other);
  WideString(Memory::Allocator& _allocator, const Uint16* _contents);
  WideString(Memory::Allocator& _allocator, const Uint16* _contents, Size _size);

  // Constructors that use system allocator.
  WideString();
  WideString(const WideString& _other);
  WideString(const Uint16* _contents);
  WideString(const Uint16* _contents, Size _size);
  WideString(WideString&& other_);

  ~WideString();

  // Disable all assignment operators because you're not supposed to use WideString
  // for any other purpose than to convert String (which is utf-8) to utf-16 for
  // interfaces expecting that, such as the ones on Windows.
  WideString& operator=(const Uint16*) = delete;
  WideString& operator=(const char*) = delete;
  WideString& operator=(const String&) = delete;

  Size size() const;
  bool is_empty() const;

  Uint16& operator[](Size _index);
  const Uint16& operator[](Size _index) const;

  Uint16* data();
  const Uint16* data() const;

  bool resize(Size _size);

  String to_utf8() const;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator* m_allocator;
  Uint16* m_data;
  Size m_size;
};

// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct StringView {
  constexpr StringView();

  constexpr StringView(const StringView&) = default;
  constexpr StringView& operator=(const StringView&) = default;

  StringView(const String& _string);
  StringView(const char* _contents);

  const char* data() const;
  Size size() const;
  bool is_empty() const;
  bool begins_with(const StringView& _other) const;
  bool ends_with(const StringView& _other) const;

  const char& operator[](Size _index) const;
  bool contains(int _ch) const;

  Optional<String> to_string(Memory::Allocator& _allocator) const;

  Size hash() const;
  Span<const char> span() const;

private:
  const char* m_data;
  Size m_size;
};

// [String]
template<typename... Ts>
String String::format(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  auto result = formatter(_allocator, _format,
    FormatNormalize<Traits::RemoveCVRef<Ts>>{}(Utility::forward<Ts>(_arguments))...);
  if (result) {
    return Utility::move(*result);
  }
  return {_allocator}; // "out of memory"};
}

inline constexpr String::String(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{m_buffer}
  , m_last{m_buffer}
  , m_capacity{m_buffer + INSITU_SIZE}
  , m_buffer{}
{
  m_buffer[0] = '\0';
}

inline constexpr String::String()
  : String{Memory::SystemAllocator::instance()}
{
}

inline String::String(const String& _contents)
  : String{_contents.allocator()}
{
  (void)append(_contents.data());
}

RX_HINT_FORCE_INLINE Size String::size() const {
  return m_last - m_data;
}

RX_HINT_FORCE_INLINE Size String::capacity() const {
  return m_capacity - m_data;
}

RX_HINT_FORCE_INLINE bool String::in_situ() const {
  return m_data == m_buffer;
}

RX_HINT_FORCE_INLINE bool String::is_empty() const {
  return m_last - m_data == 0;
}

inline void String::clear() {
  // NOTE(dweiler): This resize cannot fail.
  (void)resize(0);
}

inline bool String::append(const char* contents, Size size) {
  return append(contents, contents + size);
}

inline bool String::append(const StringView& contents) {
  return append(contents.data(), contents.size());
}

inline bool String::append(char ch) {
  return append(&ch, 1);
}

template<typename... Ts>
inline bool String::formatted_append(const char* _format, Ts&&... _arguments) {
  if constexpr(sizeof...(Ts) > 0) {
    auto contents = format(*m_allocator, _format, Utility::forward<Ts>(_arguments)...);
    return append(contents);
  } else {
    return append(_format);
  }
}

inline bool String::insert_at(Size _position, const String& _contents) {
  return insert_at(_position, _contents.data(), _contents.size());
}

inline char& String::operator[](Size index) {
  // NOTE(dweiler): The <= is not a bug, indexing the null-terminator is allowed.
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

inline const char& String::operator[](Size index) const {
  // NOTE(dweiler): The <= is not a bug, indexing the null-terminator is allowed.
  RX_ASSERT(index <= size(), "out of bounds");
  return m_data[index];
}

RX_HINT_FORCE_INLINE char* String::data() {
  return m_data;
}

RX_HINT_FORCE_INLINE const char* String::data() const {
  return m_data;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& String::allocator() const {
  return *m_allocator;
}

inline Span<char> String::span() {
  return {m_data, size() + 1};
}

inline Span<const char> String::span() const {
  return {m_data, size() + 1};
}

// [WideString]
inline WideString::WideString()
  : WideString{Memory::SystemAllocator::instance()}
{
}

inline WideString::WideString(const Uint16* _contents)
  : WideString{Memory::SystemAllocator::instance(), _contents}
{
}

inline WideString::WideString(const Uint16* _contents, Size _size)
  : WideString{Memory::SystemAllocator::instance(), _contents, _size}
{
}

inline WideString::WideString(const WideString& _other)
  : WideString{_other.allocator(), _other}
{
}

RX_HINT_FORCE_INLINE Size WideString::size() const {
  return m_size;
}

RX_HINT_FORCE_INLINE bool WideString::is_empty() const {
  return m_size == 0;
}

inline Uint16& WideString::operator[](Size _index) {
  RX_ASSERT(_index <= m_size, "out of bounds");
  return m_data[_index];
}

inline const Uint16& WideString::operator[](Size _index) const {
  RX_ASSERT(_index <= m_size, "out of bounds");
  return m_data[_index];
}

RX_HINT_FORCE_INLINE Uint16* WideString::data() {
  return m_data;
}

RX_HINT_FORCE_INLINE const Uint16* WideString::data() const {
  return m_data;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& WideString::allocator() const {
  return *m_allocator;
}

// [StringView]
constexpr StringView::StringView()
  : m_data{nullptr}
  , m_size{0}
{
}

inline StringView::StringView(const String& _string)
  : m_data{_string.data()}
  , m_size{_string.size()}
{
}

inline const char* StringView::data() const {
  return m_data;
}

inline Size StringView::size() const {
  return m_size;
}

inline bool StringView::is_empty() const {
  return m_size == 0;
}

inline const char& StringView::operator[](Size _index) const {
  RX_ASSERT(_index < m_size, "out of bounds");
  return m_data[_index];
}

inline Span<const char> StringView::span() const {
  return { m_data, m_size + 1 };
}

/// Convert UTF-16 to UTF-8
/// \param _utf16_contents The UTF-16 contents.
/// \param _length The number of codepoints in the contents.
/// \param utf8_contents_ The output for UTF-8 codepoints.
/// \return The number of UTF-8 codepoints successfully converted.
/// \note This function can be called with nullptr for \p utf8_contents_ to
/// calculate the number of UTF-8 codepoints such conversion would require to
/// allocate the storage needed for \p utf8_contents_.
RX_API Size utf16_to_utf8(const Uint16* _utf16_contents, Size _length,
  char* utf8_contents_);

/// Convert UTF-8 to UTF-16
/// \param _utf8_contents The UTF-8 contents.
/// \param _length The number of codepoints in the contents.
/// \param utf16_contents_ The output for UTF-16 codepoints.
/// \return The number of UTF-16 codepoints successfully converted.
/// \note This function can be called with nullptr for \p utf16_contents_ to
/// calculate the number of UTF-16 codepoints such conversion would require to
/// allocate the storage needed for \p utf16_contents_.
RX_API Size utf8_to_utf16(const char* _utf8_contents, Size _length,
  Uint16* utf16_contents_);

template<>
struct FormatNormalize<String> {
  const char* operator()(const String& _value) const {
    return _value.data();
  }
};

template<>
struct FormatNormalize<StringView> {
  const char* operator()(const StringView& _value) const {
    return _value.data();
  }
};

/// @{
/// Comparison and relational operators for various String-like types.
RX_API bool operator==(const String& _lhs, const String& _rhs);
RX_API bool operator==(const String& _lhs, const char* _rhs);
RX_API bool operator==(const char* _lhs, const String& _rhs);
RX_API bool operator!=(const String& _lhs, const String& rhs);
RX_API bool operator!=(const String& _lhs, const char* _rhs);
RX_API bool operator!=(const char* _lhs, const String& _rhs);
RX_API bool operator<(const String& _lhs, const String& _rhs);
RX_API bool operator<(const String& _lhs, const char* _rhs);
RX_API bool operator<(const char* _lhs, const String& _rhs);
RX_API bool operator>(const String& _lhs, const String& _rhs);
RX_API bool operator>(const String& _lhs, const char* _rhs);
RX_API bool operator>(const char* _lhs, const String& _rhs);
RX_API bool operator==(const StringView& _lhs, const StringView& _rhs);
RX_API bool operator!=(const StringView& _lhs, const StringView& _rhs);
/// @}

} // namespace Rx

#endif // RX_CORE_STRING_H
