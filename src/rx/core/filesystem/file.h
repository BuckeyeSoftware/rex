#ifndef RX_CORE_FILESYSTEM_FILE_H
#define RX_CORE_FILESYSTEM_FILE_H
#include "rx/core/stream.h"
#include "rx/core/string.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Filesystem {

struct RX_API File
  final : Stream
{
  RX_MARK_NO_COPY(File);

  constexpr File();
  File(File&& other_);
  ~File();
  File& operator=(File&& file_);

  // Open a file with name |_file_name| in mode |_mode|.
  //
  // The valid modes for opening a file are "r", "w", "rw" and "a".
  //
  // This function returns nullopt if the file could not be opened.
  static Optional<File> open(const char* _file_name, const char* _mode);
  static Optional<File> open(const String& _file_name, const char* _mode);
  static Optional<File> open(Memory::Allocator& _allocator, const char* _file_name, const char* _mode);
  static Optional<File> open(Memory::Allocator& _allocator, const String& _file_name, const char* _mode);

  // Close the file.
  [[nodiscard]]
  bool close();

  // Print |_format| with |_args| to file using |_allocator| for formatting.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(3, 0)
  bool print(Memory::Allocator& _allocator, const char* _format, Ts&&... _args);

  // Print |_format| with |_args| to file using system allocator for formatting.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool print(const char* _format, Ts&&... _args);

  // Print a string into the file.
  [[nodiscard]]
  bool print(String&& contents_);

  virtual const String& name() const &;

protected:
  virtual Uint64 on_read(Byte* _data, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stat& stat_) const;

private:
  constexpr File(Uint32 _flags, void* _impl, const char* _mode)
    : Stream{_flags}
    , m_impl{_impl}
    , m_mode{_mode}
  {
  }

  void* m_impl;
  String m_name;
  const char* m_mode;
};

inline constexpr File::File()
  : Stream{0}
  , m_impl{nullptr}
  , m_mode{nullptr}
{
}

inline Optional<File> File::open(const char* _file_name, const char* _mode) {
  return open(Memory::SystemAllocator::instance(), _file_name, _mode);
}

inline Optional<File> File::open(const String& _file_name, const char* _mode) {
  return open(Memory::SystemAllocator::instance(), _file_name.data(), _mode);
}

inline Optional<File> File::open(Memory::Allocator& _allocator, const String& _file_name, const char* _mode) {
  return open(_allocator, _file_name.data(), _mode);
}

inline File::File(File&& other_)
  : Stream{Utility::move(other_)}
  , m_impl{Utility::exchange(other_.m_impl, nullptr)}
  , m_name{Utility::move(other_.m_name)}
  , m_mode{Utility::exchange(other_.m_mode, nullptr)}
{
}

inline File::~File() {
  (void)close();
}

inline const String& File::name() const & {
  return m_name;
}

template<typename... Ts>
bool File::print(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  return print(String::format(_allocator, _format, Utility::forward<Ts>(_arguments)...));
}

template<typename... Ts>
bool File::print(const char* _format, Ts&&... _arguments) {
  return print(Memory::SystemAllocator::instance(), _format, Utility::forward<Ts>(_arguments)...);
}

RX_API Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator, const char* _file_name);
RX_API Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator, const char* _file_name);

inline Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_binary_file(_allocator, _file_name.data());
}

inline Optional<LinearBuffer> read_binary_file(const String& _file_name) {
  return read_binary_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<LinearBuffer> read_binary_file(const char* _file_name) {
  return read_binary_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_text_file(_allocator, _file_name.data());
}

inline Optional<LinearBuffer> read_text_file(const String& _file_name) {
  return read_text_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<LinearBuffer> read_text_file(const char* _file_name) {
  return read_text_file(Memory::SystemAllocator::instance(), _file_name);
}

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_FILE_H
