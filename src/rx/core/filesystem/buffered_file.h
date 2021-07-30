#ifndef RX_CORE_FILESYSTEM_BUFFERED_FILE_H
#define RX_CORE_FILESYSTEM_BUFFERED_FILE_H
#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/stream/buffered_stream.h"

/// \file buffered_file.h

namespace Rx::Filesystem {

/// \brief Buffered file.
///
/// A BufferedFile has the same interface as an UnbufferedFile except stream
/// operations are buffered by a Stream::BufferedStream.
///
/// \see Stream::BufferedStream for more information.
struct RX_API BufferedFile
  : Stream::BufferedStream
{
  /// \brief Construct a BufferedFile.
  /// \param _allocator The allocator to associate with this buffered file.
  constexpr BufferedFile(Memory::Allocator& _allocator);

  ~BufferedFile();

  /// \brief Move constructor.
  /// \param other_ The other buffered file to move from.
  ///
  /// Assigns the state of \p other_ to \c *this and sets \p other_ to a default
  /// constructed state.
  ///
  /// \note May assert if flushing the existing BufferedFile fails.
  BufferedFile(BufferedFile&& other_);

  /// \brief Moves the buffered file.
  ///
  /// If \c *this still has an opened file, calls close(). Then, or otherwise,
  /// assigns the state of \p file_ to \c *this and sets \p file_ to a
  /// default constructed state.
  ///
  /// \param file_ Another buffered file object to assign this buffered file.
  /// \returns \c *this.
  BufferedFile& operator=(BufferedFile&& file_);

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
  /// \returns The BufferedFile on success, nullopt otherwise.
  static Optional<BufferedFile> open(Memory::Allocator& _allocator,
    const StringView& _file_name, const StringView& _mode,
    Uint16 _page_size = BUFFER_PAGE_SIZE, Uint8 _page_count = BUFFER_PAGE_COUNT);

private:
  BufferedFile(BufferedStream&& buffered_stream_, UnbufferedFile&& unbuffered_file_);
  UnbufferedFile m_unbuffered_file;
};

inline constexpr BufferedFile::BufferedFile(Memory::Allocator& _allocator)
  : BufferedStream{_allocator}
  , m_unbuffered_file{_allocator}
{
}

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_FILE_H
