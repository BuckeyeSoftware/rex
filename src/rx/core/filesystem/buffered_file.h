#ifndef RX_CORE_FILESYSTEM_BUFFERED_FILE_H
#define RX_CORE_FILESYSTEM_BUFFERED_FILE_H
#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/buffered_stream.h"

namespace Rx::Filesystem {

// Buffered file.
struct BufferedFile
  : Stream
{
  // Default buffer page size and count values for a file buffer.
  static constexpr const auto BUFFER_PAGE_SIZE = 4096_u16;
  static constexpr const auto BUFFER_PAGE_COUNT = 64_u8;

  constexpr BufferedFile(Memory::Allocator& _allocator);
  BufferedFile(BufferedFile&& other_);
  BufferedFile& operator=(BufferedFile&& file_);

  // Open a file with name |_file_name| in mode |_mode|.
  //
  // The valid modes for opening a file are "r", "w", "rw" and "a".
  //
  // This function returns nullopt if the file could not be opened.
  static Optional<BufferedFile> open(Memory::Allocator& _allocator,
    const char* _file_name, const char* _mode,
    Uint16 _page_size = BUFFER_PAGE_SIZE, Uint8 _page_count = BUFFER_PAGE_COUNT);

  static Optional<BufferedFile> open(Memory::Allocator& _allocator,
    const String& _file_name, const char* _mode,
    Uint16 _page_size = BUFFER_PAGE_SIZE, Uint8 _page_count = BUFFER_PAGE_COUNT);

  // Close the file.
  [[nodiscard]] bool close();

  // Print |_format| with |_args| to file using |_allocator| for formatting.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(3, 0)
  bool print(Memory::Allocator& _allocator, const char* _format, Ts&&... _args);

  // Print a string into the file.
  [[nodiscard]] bool print(String&& contents_);

  virtual const String& name() const &;

protected:
  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stat& stat_) const;
  virtual bool on_flush();

private:
  BufferedFile(BufferedStream&& buffered_stream_, UnbufferedFile&& unbuffered_file_);

  UnbufferedFile m_unbuffered_file;
  BufferedStream m_buffered_stream;
};

inline constexpr BufferedFile::BufferedFile(Memory::Allocator& _allocator)
  : Stream{0}
  , m_unbuffered_file{_allocator}
  , m_buffered_stream{_allocator}
{
}

inline Optional<BufferedFile> BufferedFile::open(Memory::Allocator& _allocator,
  const String& _file_name, const char* _mode,
  Uint16 _page_size, Uint8 _page_count)
{
  return open(_allocator, _file_name.data(), _mode, _page_size, _page_count);
}

template<typename... Ts>
inline bool BufferedFile::print(Memory::Allocator& _allocator, const char* _format, Ts&&... _args) {
  return print(String::format(_allocator, _format, Utility::forward<Ts>(_args)...));
}

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_FILE_H
