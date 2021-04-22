#ifndef RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#define RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#include "rx/core/stream/context.h"
#include "rx/core/string.h"

/// \file unbuffered_file.h

/// File system access.
namespace Rx::Filesystem {

/// \brief Unbuffered file.
struct RX_API UnbufferedFile
  : Stream::Context
{
  RX_MARK_NO_COPY(UnbufferedFile);

  /// \brief Construct an UnbufferedFile.
  /// \param _allocator The allocator to associate with this unbuffered file.
  constexpr UnbufferedFile(Memory::Allocator& _allocator);

  /// \brief Move constructor.
  /// \param other_ The other unbuffered file to move from.
  ///
  /// Assigns the state of \p other_ to \c *this and sets \p other_ to a default
  /// constructed state.
  UnbufferedFile(UnbufferedFile&& other_);

  /// \brief Destroy an unbuffered file.
  /// \note Will call close(), asserting if the close fails.
  ~UnbufferedFile();

  /// \brief Moves the unbuffered file.
  ///
  /// If \c *this still has an opened file, calls close(). Then, or otherwise,
  /// assigns the state of \p file_ to \c *this and sets \p file_ to a
  /// default constructed state.
  ///
  /// \param file_ Another unbuffered file object to assign this unbuffered file.
  /// \returns \c *this.
  UnbufferedFile& operator=(UnbufferedFile&& file_);

  /// @{
  /// \brief Opens a file.
  ///
  /// \param _allocator The allocator to use for stream state.
  /// \param _file_name The name of the file to open.
  /// \param _mode The file access mode which can be:
  /// | Mode    | Semantics                                                    |
  /// | :-----: | :----------------------------------------------------------- |
  /// | "r"     | Open for reading. The file must exist.                       |
  /// | "w"     | Create and open for writing. Replaces existing file.         |
  /// | "a"     | Open for appending. File is created if it does not exist.    |
  /// | "r+"    | Open for update. The file must exist.                        |
  /// | "w+"    | Create and open file for update. Replaces existing file.     |
  /// | "a+"    | Open for update. File is created if it does not exist.       |
  /// \param _page_size The size of a buffer page in bytes.
  /// \param _page_count The number of pages for the buffer.
  ///
  /// \note All files are treated as binary. There is no notion of a text or
  /// binary stream.
  ///
  /// \note When the access mode is "a", or "a+", the cursor for the stream is
  /// positioned to the end of the file and all output operations move the
  /// position to the end of the file.
  ///
  /// \returns The UnbufferedFile on success, \c nullopt otherwise.
  static Optional<UnbufferedFile> open(Memory::Allocator& _allocator,
    const char* _file_name, const char* _mode);
  static Optional<UnbufferedFile> open(Memory::Allocator& _allocator,
    const String& _file_name, const char* _mode);
  /// @}

  /// \brief Close the file.
  /// \return When closed successfully, \c true. Otherwise, \c false.
  /// \note The close can fail if the file has already been closed.
  [[nodiscard]] bool close();

  /// \brief Print formatted text to the file.
  ///
  /// \param _allocator The allocator to use for formatting the text.
  /// \param _format The format string.
  /// \param _args Arguments for formatting.
  /// \return On a successful print, \c true. Otherwise, \c false.
  ///
  /// \note A print can fail for multiple reasons:
  ///  * The file is not opened.
  ///  * Ran out of memory in \p _allocator to format the text.
  ///  * The underlying Stream::Context::write failed to write all bytes.
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(3, 0)
  bool print(Memory::Allocator& _allocator, const char* _format, Ts&&... _args);

  /// \brief Print a string into the file.
  /// \param contents_ The contents to print
  /// \return On a successful print, \c true. Otherwise, \c false.
  /// \see Note in print() for why this can fail.
  [[nodiscard]]
  bool print(String&& contents_);

  /// Get the name of the file.
  virtual const String& name() const &;

protected:
  virtual Uint64 on_read(Byte* _data, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stream::Stat& stat_) const;
  virtual bool on_truncate(Uint64 _size);
  virtual Uint64 on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size);

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