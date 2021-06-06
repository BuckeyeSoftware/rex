#include <string.h> // strcmp, memmove
#include <stdarg.h> // va_{list, start, end, copy}
#include <stdio.h> // vsnprintf

#include "rx/core/string.h" // string

#include "rx/core/utility/swap.h"

#include "rx/core/hash/string.h"

#include "rx/core/memory/search.h"
#include "rx/core/memory/copy.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/hints/unlikely.h"

namespace Rx {

Size utf8_to_utf16(const char* _utf8_contents, Size _length,
  Uint16* utf16_contents_)
{
  Size elements{0};
  Uint32 code_point{0};

  for (Size i{0}; i < _length; i++) {
    const char* element{_utf8_contents + i};
    const Byte element_ch{static_cast<Byte>(*element)};

    if (element_ch <= 0x7f) {
      code_point = static_cast<Uint16>(element_ch);
    } else if (element_ch <= 0xbf) {
      code_point = (code_point << 6) | (element_ch & 0x3f);
    } else if (element_ch <= 0xdf) {
      code_point = element_ch & 0x1f;
    } else if (element_ch <= 0xef) {
      code_point = element_ch & 0x0f;
    } else {
      code_point = element_ch & 0x07;
    }

    element++;
    if ((*element & 0xc0) != 0x80 && code_point <= 0x10ffff) {
      if (code_point > 0xffff) {
        elements += 2;
        if (utf16_contents_) {
          *utf16_contents_++ = static_cast<Uint16>(0xd800 + (code_point >> 10));
          *utf16_contents_++ = static_cast<Uint16>(0xdc00 + (code_point & 0x03ff));
        }
      } else if (code_point < 0xd800 || code_point >= 0xe000) {
        elements += 1;
        if (utf16_contents_) {
          *utf16_contents_++ = static_cast<Uint16>(code_point);
        }
      }
    }
  }

  return elements;
}

Size utf16_to_utf8(const Uint16* _utf16_contents, Size _length,
  char* utf8_contents_)
{
  Size elements{0};
  Uint32 code_point{0};

  for (Size i{0}; i < _length; i++) {
    const Uint16* element{_utf16_contents + i};

    if (*element >= 0xd800 && *element <= 0xdbff) {
      code_point = ((*element - 0xd800) << 10) + 0x10000;
    } else {
      if (*element >= 0xdc00 && *element <= 0xdfff) {
        code_point |= *element - 0xdc00;
      } else {
        code_point = *element;
      }

      if (code_point < 0x7f) {
        elements += 1;
        if (utf8_contents_) {
          *utf8_contents_++ = static_cast<char>(code_point);
        }
      } else if (code_point <= 0x7ff) {
        elements += 2;
        if (utf8_contents_) {
          *utf8_contents_++ = static_cast<char>(0xc0 | ((code_point >> 6) & 0x1f));
          *utf8_contents_++ = static_cast<char>(0x80 | (code_point & 0x3f));
        }
      } else if (code_point <= 0xffff) {
        elements += 3;
        if (utf8_contents_) {
          *utf8_contents_++ = static_cast<char>(0xe0 | ((code_point >> 12) & 0x0f));
          *utf8_contents_++ = static_cast<char>(0x80 | ((code_point >> 6) & 0x3f));
          *utf8_contents_++ = static_cast<char>(0x80 | (code_point & 0x3f));
        }
      } else {
        elements += 4;
        if (utf8_contents_) {
          *utf8_contents_++ = static_cast<char>(0xf0 | ((code_point >> 18) & 0x07));
          *utf8_contents_++ = static_cast<char>(0x80 | ((code_point >> 12) & 0x3f));
          *utf8_contents_++ = static_cast<char>(0x80 | ((code_point >> 6) & 0x3f));
          *utf8_contents_++ = static_cast<char>(0x80 | (code_point & 0x3f));
        }
      }

      code_point = 0;
    }
  }

  return elements;
}

char* String::read_line(char*& line_) {
  if (!line_ || !*line_) {
    return nullptr;
  }
  char* line = line_;
  line_ += strcspn(line_, "\n");
  *line_++ = '\0';
  return line;
}

String String::formatter(Memory::Allocator& _allocator, const char* _format, ...) {
  String result{_allocator};

  va_list va;
  va_start(va, _format);
  const Size need_length = format_buffer_va_list({nullptr, 0}, _format, va);
  va_end(va);

  RX_ASSERT(result.resize(need_length), "out of memory");

  va_start(va, _format);
  format_buffer_va_list(result.span(), _format, va);
  va_end(va);

  return result;
}

String::String(Memory::Allocator& _allocator, const char* _contents)
  : String{_allocator, _contents, _contents + strlen(_contents)}
{
}

String::String(Memory::Allocator& _allocator, const char* _first,
               const char* _last)
  : String{_allocator}
{
  (void)append(_first, _last);
}

String::String(String&& contents_)
  : m_allocator{contents_.m_allocator}
{
  if (contents_.m_data == contents_.m_buffer) {
    m_data = m_buffer;
    m_last = m_buffer;
    m_capacity = m_buffer + INSITU_SIZE;
    (void)reserve(contents_.size());
    (void)append(contents_.data(), contents_.size());
  } else {
    m_data = contents_.m_data;
    m_last = contents_.m_last;
    m_capacity = contents_.m_capacity;
  }

  contents_.m_data = contents_.m_buffer;
  contents_.m_last = contents_.m_buffer;
  contents_.m_capacity = contents_.m_buffer + INSITU_SIZE;

  // NOTE(dweiler): This cannot fail. Use (void) to suppress [[nodiscard]].
  (void)contents_.resize(0);
}

// NOTE(dweiler): The subtraction of one is to account for the null-terminator.
String::String(Memory::View _view)
  : m_allocator{_view.owner}
  , m_data{reinterpret_cast<char*>(_view.data)}
  , m_last{m_data + _view.size - 1}
  , m_capacity{m_data + _view.capacity - 1}
{
  // Validate the view represents a string by ensuring |m_last| is equal in
  // address to the position of the first null-terminator in the view.
  //
  // This will indicate no null-terminator exists in the middle and that the
  // view's size field represents the string's length including the null.
  RX_ASSERT(m_last == reinterpret_cast<char*>(Memory::search(m_data, _view.capacity, '\0')),
    "malformed string view");
}

String::~String() {
  if (m_data != m_buffer) {
    m_allocator->deallocate(m_data);
  }
}

String& String::operator=(String&& contents_) {
  if (&contents_ != this) {
    String(Utility::move(contents_)).swap(*this);
  }
  return *this;
}

bool String::reserve(Size _capacity) {
  if (m_data + _capacity + 1 <= m_capacity) {
    return true;
  }

  char* data = nullptr;
  const auto size = static_cast<Size>(m_last - m_data);
  if (m_data == m_buffer) {
    data = reinterpret_cast<char*>(m_allocator->allocate(_capacity + 1));
    if (RX_HINT_UNLIKELY(!data)) {
      return false;
    }
    // Copy data out of |m_buffer| to |data|.
    Memory::copy(data, m_buffer, size + 1);
  } else {
    data = reinterpret_cast<char*>(m_allocator->reallocate(m_data, _capacity + 1));
    if (RX_HINT_UNLIKELY(!data)) {
      return false;
    }
  }

  m_data = data;
  m_last = m_data + size;
  m_capacity = m_data + _capacity;

  return true;
}

bool String::resize(Size _size) {
  const auto previous_size = static_cast<Size>(m_last - m_data);

  if (RX_HINT_UNLIKELY(!reserve(_size))) {
    return false;
  }

  if (_size > previous_size) {
    char* element{m_last};
    char* end{m_data + _size + 1};
    for (; element < end; ++element) {
      *element = '\0';
    }
  } else if (m_last != m_data) {
    m_data[_size] = '\0';
  }

  m_last = m_data + _size;

  return true;
}

Optional<Size> String::find_first_of(int _ch) const {
  if (char* search = strchr(m_data, _ch)) {
    return search - m_data;
  }
  return nullopt;
}

Optional<Size> String::find_first_of(const char* _contents) const {
  if (const char* search = strstr(m_data, _contents)) {
    return search - m_data;
  }
  return nullopt;
}

Optional<Size> String::find_last_of(int _ch) const {
  if (const char* search = strrchr(m_data, _ch)) {
    return search - m_data;
  }
  return nullopt;
}

Optional<Size> String::find_last_of(const char* _contents) const {
  auto reverse_strstr = [](const char* _haystack, const char* _needle) {
    const char* r = nullptr;
    for (;;) {
      const char* p = strstr(_haystack, _needle);
      if (!p) {
        return r;
      }
      r = p;
      _haystack = p + 1;
    }
  };

  if (const char* search = reverse_strstr(m_data, _contents)) {
    return search - m_data;
  }

  return nullopt;
}

bool String::append(const char* _first, const char* _last) {
  const auto new_size = static_cast<Size>(m_last - m_data + _last - _first + 1);
  if (m_data + new_size > m_capacity && !reserve((new_size * 3) / 2)) {
    return false;
  }

  const auto length = static_cast<Size>(_last - _first);
  Memory::copy(m_last, _first, length);

  m_last += length;
  *m_last = '\0';

  return true;
}

bool String::append(const char* _contents) {
  return append(_contents, strlen(_contents));
}

bool String::insert_at(Size _position, const char* _contents, Size _size) {
  const Size old_this_size{size()};
  const Size old_that_size{_size};

  if (RX_HINT_UNLIKELY(!resize(old_this_size + old_that_size))) {
    return false;
  }

  char* cursor{m_data + _position};
  memmove(cursor + old_that_size, cursor, old_this_size - _position);
  memmove(cursor, _contents, old_that_size);

  return true;
}

bool String::insert_at(Size _position, const char* _contents) {
  return insert_at(_position, _contents, strlen(_contents));
}

String String::lstrip(const char* _set) const {
  const char* ch = m_data;
  for (; *ch && strchr(_set, *ch); ch++);
  return {allocator(), ch};
}

String String::rstrip(const char* _set) const {
  const char* ch = m_last - 1;
  for (; ch > m_data && strchr(_set, *ch); ch--);
  return {allocator(), m_data, ch + 1};
}

String String::strip(const char* _set) const {
  const char* beg = m_data;
  const char* end = m_last - 1;
  for (; *beg && strchr(_set, *beg); beg++);
  for (; end > beg && strchr(_set, *end); end--);
  return {allocator(), beg, end + 1};
}

String String::substring(Size _offset, Size _length) const {
  const char* begin = m_data + _offset;
  RX_ASSERT(begin < m_data + size(), "out of bounds");
  if (_length == 0) {
    return {allocator(), begin};
  }
  // NOTE(dweiler): You can substring the whole string.
  RX_ASSERT(begin + _length <= m_data + size(), "out of bounds");
  return {allocator(), begin, begin + _length};
}

Size String::scan(const char* _scan_format, ...) const {
  va_list va;
  va_start(va, _scan_format);
  const auto result{vsscanf(m_data, _scan_format, va)};
  va_end(va);
  return static_cast<Size>(result);
}

char String::pop_back() {
  if (m_last == m_data) {
    return *data();
  }
  m_last--;
  const char last{static_cast<char>(*m_last)};
  *m_last = 0;
  return last;
}

void String::erase(Size _begin, Size _end) {
  RX_ASSERT(_begin < _end, "invalid range");
  RX_ASSERT(_begin < size(), "out of bounds");
  RX_ASSERT(_end <= size(), "out of bounds");

  char *const begin{m_data + _begin};
  char *const end{m_data + _end};

  const auto length{m_last - end};

  memmove(begin, end, length);
  m_last = begin + length;
  *m_last = '\0';
}

String String::human_size_format(Size _size) {
  static constexpr const char* SUFFIXES[] = {
    "B", "KiB", "MiB", "GiB", "TiB"
  };

  Float64 bytes = static_cast<Float64>(_size);
  Size i = 0;
  for (; bytes >= 1024.0 && i < sizeof SUFFIXES / sizeof *SUFFIXES; i++) {
    bytes /= 1024.0;
  }
  RX_ASSERT(i != sizeof SUFFIXES / sizeof *SUFFIXES, "out of bounds");

  char buffer[2*(DBL_MANT_DIG + DBL_MAX_EXP)];

  // NOTE: The truncation is wanted here!
#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
  snprintf(buffer, sizeof buffer, "%.*f",
    static_cast<int>(sizeof buffer), bytes);
#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif

  char* period = strchr(buffer, '.');
  RX_ASSERT(period, "failed to format");
  period[3] = '\0';
  // TODO(dweiler): Consider using NullAllocator here as human_size_format
  // should fit completely in-situ.
  return format(Memory::SystemAllocator::instance(), "%s %s", buffer, SUFFIXES[i]);
}

bool String::begins_with(const char* _prefix) const {
  return strstr(m_data, _prefix) == m_data;
}

bool String::begins_with(const String& _prefix) const {
  return strstr(m_data, _prefix.data()) == m_data;
}

bool String::ends_with(const char* _suffix) const {
  if (size() < strlen(_suffix)) {
    return false;
  }

  return strcmp(m_data + size() - strlen(_suffix), _suffix) == 0;
}

bool String::ends_with(const String& _suffix) const {
  if (size() < _suffix.size()) {
    return false;
  }

  return strcmp(m_data + size() - _suffix.size(), _suffix.data()) == 0;
}

bool String::contains(const char* _needle) const {
  return strstr(m_data, _needle);
}

bool String::contains(const String& _needle) const {
  return strstr(m_data, _needle.data());
}

Size String::hash() const {
  return Hash::string(m_data, size());
}

Optional<Memory::View> String::disown() {
  Byte* data = reinterpret_cast<Byte*>(m_data);

  Size n_size = 0;
  Size n_capacity = 0;

  if (m_data == m_buffer) {
    // Cannot disown the memory of small string optimization. Make copy.
    n_size = size() + 1;
    n_capacity = n_size;
    data = m_allocator->allocate(n_capacity);
    if (!data) {
      return nullopt;
    }
    Memory::copy(data, reinterpret_cast<Byte*>(m_data), n_capacity);
  } else {
    n_size = size() + 1;
    n_capacity = capacity() + 1;
  }

  // Reset the string back to small string optimization state.
  m_data = m_buffer;
  m_last = m_buffer;
  m_capacity = m_buffer + INSITU_SIZE;

  return Memory::View{&allocator(), data, n_size, n_capacity};
}

WideString String::to_utf16() const {
  const Size length = utf8_to_utf16(m_data, size(), nullptr);
  WideString contents{allocator()};
  contents.resize(length);
  utf8_to_utf16(m_data, length, contents.data());
  return contents;
}

// Complicated because of small string optimization.
void String::swap(String& other_) {
  // Swapping with self is a no-op.
  if (&other_ == this) {
    return;
  }

  Utility::swap(m_data, other_.m_data);
  Utility::swap(m_last, other_.m_last);
  Utility::swap(m_capacity, other_.m_capacity);

  char buffer[INSITU_SIZE];

  if (m_data == other_.m_buffer) {
    const char* element = other_.m_buffer;
    const char* end = m_last;
    char* out = buffer;

    while (element != end) {
      *out++ = *element++;
    }
  }

  if (other_.m_data == m_buffer) {
    other_.m_last = other_.m_last - other_.m_data + other_.m_buffer;
    other_.m_data = other_.m_buffer;
    other_.m_capacity = other_.m_buffer + INSITU_SIZE;

    char* element = other_.m_data;
    const char* end = other_.m_last;
    const char* in = m_buffer;

    while (element != end) {
      *element++ = *in++;
    }

    *other_.m_last = '\0';
  }

  if (m_data == other_.m_buffer) {
    m_last = m_last - m_data + m_buffer;
    m_data = m_buffer;
    m_capacity = m_buffer + INSITU_SIZE;

    char* element = m_data;
    const char* end = m_last;
    const char* in = buffer;

    while (element != end) {
      *element++ = *in++;
    }

    *m_last = '\0';
  }
}

bool operator==(const String& _lhs, const String& _rhs) {
  if (&_lhs == &_rhs) {
    return true;
  }

  if (_lhs.size() != _rhs.size()) {
    return false;
  }

  return !strcmp(_lhs.data(), _rhs.data());
}

bool operator==(const String& _lhs, const char* _rhs) {
  return !strcmp(_lhs.data(), _rhs);
}

bool operator==(const char* _lhs, const String& _rhs) {
  return !strcmp(_lhs, _rhs.data());;
}

bool operator!=(const String& _lhs, const String& _rhs) {
  if (&_lhs == &_rhs) {
    return false;
  }

  if (_lhs.size() != _rhs.size()) {
    return true;
  }

  return strcmp(_lhs.data(), _rhs.data());
}

bool operator!=(const String& _lhs, const char* _rhs) {
  return strcmp(_lhs.data(), _rhs);
}

bool operator!=(const char* _lhs, const String& _rhs) {
  return strcmp(_lhs, _rhs.data());
}

bool operator<(const String& _lhs, const String& _rhs) {
  return &_lhs == &_rhs ? false : strcmp(_lhs.data(), _rhs.data()) < 0;
}

bool operator<(const String& _lhs, const char* _rhs) {
  return strcmp(_lhs.data(), _rhs) < 0;
}

bool operator<(const char* _lhs, const String& _rhs) {
  return strcmp(_lhs, _rhs.data()) < 0;
}

bool operator>(const String& _lhs, const String& _rhs) {
  return &_lhs == &_rhs ? false : strcmp(_lhs.data(), _rhs.data()) > 0;
}

bool operator>(const String& _lhs, const char* _rhs) {
  return strcmp(_lhs.data(), _rhs) > 0;
}

bool operator>(const char* _lhs, const String& _rhs) {
  return strcmp(_lhs, _rhs.data()) > 0;
}

bool WideString::resize(Size _size) {
  Uint16* resize = reinterpret_cast<Uint16*>(m_allocator->reallocate(m_data, sizeof *m_data, _size + 1));
  if (!RX_HINT_UNLIKELY(resize)) {
    return false;
  }

  m_data = resize;
  m_data[_size] = 0;
  m_size = _size;
  return true;
}

String WideString::to_utf8() const {
  Size size{utf16_to_utf8(m_data, m_size, nullptr)};
  String contents{allocator()};
  (void)contents.resize(size);
  utf16_to_utf8(m_data, m_size, contents.data());
  return contents;
}

static Size utf16_len(const Uint16* _data) {
  Size length = 0;
  while (_data[length]) {
    ++length;
  }
  return length;
}

// WideString
WideString::WideString(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{0}
{
}

WideString::WideString(Memory::Allocator& _allocator, const WideString& _other)
  : WideString{_allocator, _other.data(), _other.size()}
{
}

WideString::WideString(Memory::Allocator& _allocator, const Uint16* _contents)
  : WideString{_allocator, _contents, utf16_len(_contents)}
{
}

WideString::WideString(Memory::Allocator& _allocator, const Uint16* _contents,
                       Size _size)
  : m_allocator{&_allocator}
  , m_size{_size}
{
  m_data = reinterpret_cast<Uint16*>(m_allocator->allocate(sizeof *m_data, _size + 1));
  RX_ASSERT(m_data, "out of memory");

  Memory::copy(m_data, _contents, _size + 1);
}

WideString::WideString(WideString&& other_)
  : m_allocator{other_.m_allocator}
  , m_data{Utility::exchange(other_.m_data, nullptr)}
  , m_size{Utility::exchange(other_.m_size, 0)}
{
}

WideString::~WideString() {
  m_allocator->deallocate(m_data);
}

} // namespace Rx
