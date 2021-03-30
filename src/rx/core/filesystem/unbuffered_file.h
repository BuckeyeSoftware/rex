#ifndef RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#define RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#include "rx/core/stream/context.h"
#include "rx/core/string.h"

namespace Rx::Filesystem {

struct RX_API UnbufferedFile
  : Stream::Context
{
  RX_MARK_NO_COPY(UnbufferedFile);

  constexpr UnbufferedFile(Memory::Allocator& _allocator);
  UnbufferedFile(UnbufferedFile&& other_);
  ~UnbufferedFile();
  UnbufferedFile& operator=(UnbufferedFile&& file_);

  // Open a file with name |_file_name| in mode |_mode|.
  //
  // The valid modes for opening a file are "r", "w", "rw" and "a".
  //
  // This function returns nullopt if the file could not be opened.
  static Optional<UnbufferedFile> open(Memory::Allocator& _allocator, const char* _file_name, const char* _mode);
  static Optional<UnbufferedFile> open(Memory::Allocator& _allocator, const String& _file_name, const char* _mode);

  // Close the file.
  [[nodiscard]] bool close();

  // Print |_format| with |_args| to file using |_allocator| for formatting.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(3, 0)
  bool print(Memory::Allocator& _allocator, const char* _format, Ts&&... _args);

  // Print a string into the file.
  [[nodiscard]]
  bool print(String&& contents_);

  virtual const String& name() const &;

protected:
  virtual Uint64 on_read(Byte* _data, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stream::Stat& stat_) const;

private:
  UnbufferedFile(Uint32 _flags, void* _impl, String&& name_, const char* _mode);

  void* m_impl;
  String m_name;
  const char* m_mode;
};

inline constexpr UnbufferedFile::UnbufferedFile(Memory::Allocator& _allocator)
  : Stream::Context{0}
  , m_impl{nullptr}
  , m_name{_allocator}
  , m_mode{nullptr}
{
}

inline UnbufferedFile::~UnbufferedFile() {
  (void)close();
}

inline Optional<UnbufferedFile> UnbufferedFile::open(Memory::Allocator& _allocator, const String& _file_name, const char* _mode) {
  return open(_allocator, _file_name.data(), _mode);
}

template<typename... Ts>
bool UnbufferedFile::print(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  return print(String::format(_allocator, _format, Utility::forward<Ts>(_arguments)...));
}

// Helper functions for whole-file reading.
RX_API Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator, const char* _file_name);
RX_API Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator, const char* _file_name);

inline Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_binary_file(_allocator, _file_name.data());
}

inline Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_text_file(_allocator, _file_name.data());
}

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H