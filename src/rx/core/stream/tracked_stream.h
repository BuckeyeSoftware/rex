#ifndef RX_CORE_STREAM_TRACKED_STREAM_H
#define RX_CORE_STREAM_TRACKED_STREAM_H
#include "rx/core/stream/operations.h"
#include "rx/core/string.h"

#include <string.h> // TODO(dweiler): remove.

namespace Rx::Stream {

struct UntrackedStream;

/// \brief A stream with internal cursor.
///
/// A TrackedStream connects an UntrackedStream and provides a managed stream
/// cursor. You can have multiple TrackedStreams per UntrackedStream.
struct TrackedStream {
  constexpr TrackedStream(UntrackedStream& _stream);

  /// \brief Read from the stream.
  ///
  /// Reads \p _size bytes into \p data_ from the stream, advacing the cursor.
  ///
  /// \param data_ The data to write into.
  /// \param _size The number of bytes to read.
  /// \returns The number of bytes actually read.
  [[nodiscard]] Uint64 read(Byte* _data, Uint64 _size);

  /// \brief Write to the stream.
  ///
  /// Writes \p _size bytes from \p _data into the stream, advacing the cursor.
  ///
  /// \param _data The data to write.
  /// \param _size The amount of bytes from \p _data to write.
  /// \returns The number of bytes actually written.
  [[nodiscard]] Uint64 write(const Byte* _data, Uint64 _size);

  /// \brief Seek the stream.
  ///
  /// Seeks the stream and sets it's cursor \p _where bytes relative to
  /// \p _whence.
  ///
  /// \param _where The number of bytes to seek.
  /// \param _whence Relative to a location in the cursor.
  /// \returns On a successful seek, \c true. Otherwise, \c false.
  ///
  /// \note A seek can fail if the underlying stream does not support it or
  /// the seeked position is out of bounds of the stream.
  [[nodiscard]] bool seek(Sint64 _where, Whence _whence);

  /// \brief Rewind the stream.
  /// This is a convenience and faster way to rewind a stream, functionally
  /// the same as
  /// \code{.cpp}
  ///   seek(0, Whence::SET)
  /// \endcode
  void rewind();

  /// Stat the stream.
  /// \return A stat object if successful or \c nullopt on failure.
  Optional<Stat> stat() const;

  /// \brief Flush the stream.
  [[nodiscard]] bool flush();

  /// \brief Truncate the stream.
  /// \param _size The size to truncate to.
  [[nodiscard]] bool truncate(Uint64 _size);

  /// Determine the byte offset in the stream the cursor is at.
  Uint64 tell() const;

  /// Flags of the underlying stream.
  Uint64 flags() const;

  /// Write formatted text to the stream.
  ///
  /// \param _allocator The allocator to use for formatting the string.
  /// \param _format The format string.
  /// \param _arguments The format arguments.
  ///
  /// \return On successful formatting and writing of the formatted contents,
  /// \c true. Otherwise, when formatting fails or not all characters of the
  /// formatted message are written, \c false.
  template<typename... Ts>
  RX_HINT_FORMAT(3, 0) bool print(Memory::Allocator& _allocator,
    const char* _format, Ts&&... _arguments);

private:
  UntrackedStream& m_stream;
  Uint64 m_offset;
  bool m_is_eos;
};

inline constexpr TrackedStream::TrackedStream(UntrackedStream& _stream)
  : m_stream{_stream}
  , m_offset{0}
  , m_is_eos{false}
{
}

inline Uint64 TrackedStream::tell() const {
  return m_offset;
}

template<typename... Ts>
bool TrackedStream::print(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  // Don't use String::format unless there are format arguments.
  if constexpr(sizeof...(Ts) != 0) {
    const auto format = String::format(_allocator, _format, Utility::forward<Ts>(_arguments)...);
    const auto data = reinterpret_cast<const Byte*>(format.data());
    const auto size = format.size();
    return write(data, size) == size;
  } else {
    const auto data = reinterpret_cast<const Byte*>(_format);
    const auto size = strlen(_format);
    return write(data, size) == size;
  }
}

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_TRACKED_STREAM_H