#ifndef RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#define RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H
#include "rx/core/stream/untracked_stream.h"
#include "rx/core/string.h"

/// \file unbuffered_file.h

/// File system access.
namespace Rx::Filesystem {

/// \brief Unbuffered file.
struct RX_API UnbufferedFile
  : Stream::UntrackedStream
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
    const StringView& _file_name, const StringView& _mode);
  /// @}

  /// \brief Close the file.
  /// \return When closed successfully, \c true. Otherwise, \c false.
  /// \note The close can fail if the file has already been closed.
  [[nodiscard]] bool close();

  /// Get the name of the file.
  virtual const String& name() const &;

protected:
  virtual Uint64 on_read(Byte* _data, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual Optional<Stream::Stat> on_stat() const;
  virtual bool on_truncate(Uint64 _size);
  virtual Uint64 on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size);

private:
  UnbufferedFile(Uint32 _flags, void* _impl, String&& name_, const char* _mode);

  void* m_impl;
  String m_name;
  const char* m_mode;
};

inline constexpr UnbufferedFile::UnbufferedFile(Memory::Allocator& _allocator)
  : Stream::UntrackedStream{0}
  , m_impl{nullptr}
  , m_name{_allocator}
  , m_mode{nullptr}
{
}

inline UnbufferedFile::~UnbufferedFile() {
  (void)close();
}

// Helper functions for whole-file reading.
Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator,
  const StringView& _file_name);
Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator,
  const StringView& _file_name);

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_UNBUFFERED_FILE_H