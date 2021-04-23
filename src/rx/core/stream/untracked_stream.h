#ifndef RX_CORE_STREAM_UNTRACKED_STREAM_H
#define RX_CORE_STREAM_UNTRACKED_STREAM_H
#include "rx/core/linear_buffer.h"
#include "rx/core/optional.h"

#include "rx/core/stream/operations.h"

namespace Rx {
  struct String;
} // namespace Rx

/// Stream IO.
namespace Rx::Stream {

/// \brief UntrackedStream context
///
/// An UntrackedStream is an interface that "stream-like" types can implement
/// to participate in stream operations as defined by the rest of this namespace.
///
/// Something is "stream-like" if it supports a list of *directed* operations,
/// as described by the methods of this class.
struct RX_API UntrackedStream {
  /// \brief Construct a stream context.
  /// \param _flags The flags indicating what features this stream support.
  constexpr UntrackedStream(Uint32 _flags);

  /// \brief Move constructor.
  /// \param stream_ The stream to move from.
  UntrackedStream(UntrackedStream&& stream_);

  /// Destroy a stream context.
  virtual ~UntrackedStream();

  /// \brief Move assignment operator.
  /// \param stream_ The stream to move from.
  UntrackedStream& operator=(UntrackedStream&& stream_);

  /// Get flags of the stream.
  constexpr Uint32 flags() const;

  /// The name of the stream. This must always be implemented.
  virtual const String& name() const & = 0;

  /// \brief Read the contents of the stream as binary.
  /// \param _allocator The allocator to use to allocate the LinearBuffer.
  /// \returns On a successful call, a LinearBuffer containing the binary
  /// contents. Otherwise, \c nullopt.
  Optional<LinearBuffer> read_binary(Memory::Allocator& _allocator);

  /// \brief Read the contents of the stream as text.
  ///
  /// This function interprets the contents of the stream as text and performs
  /// various conversions to the data. Specifically:
  ///  * Normalize all line endings to LF.
  ///  * Convert all Unicode text (UTF-32, UTF-16, UTF-8) into UTF-8 text.
  ///  * Handle all Unicode BOM (byte-order-marks).
  ///
  /// \param _allocator The allocator to use to allocate the LinearBuffer.
  /// \returns On a successful call, a LinearBuffer containing the text
  /// contents. Otherwise, \c nullopt.
  Optional<LinearBuffer> read_text(Memory::Allocator& _allocator);

  /// \brief Read data.
  /// Reads \p _size bytes from the stream at \p _offset into \p data_.
  /// \returns The number of bytes actually read from \p _offset into \p data_.
  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);

  /// \brief Write data.
  /// Writes \p _size bytes from \p _data into the steam at \p _offset.
  /// \returns The number of bytes actually written at \p _offset from \p _data.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);

  /// \brief Stat the stream for information.
  /// \returns On a successful stat, the stat object. Otherwise, \c nullopt.
  virtual Optional<Stat> on_stat() const;

  /// \brief Flush the stream.
  /// \returns On a successful flush, \c true. Otherwise, \c false.
  virtual bool on_flush();

  /// \brief Zero a region in the stream
  /// Zeros the region [ \p _size, \p _size + \p _offset ) in the stream.
  /// \param _size The number of bytes to zero.
  /// \param _offset The offset to zero.
  /// \returns The number of bytes actually zerored from \p _offset.
  /// \note The zeroing must be within the already allocated stream. This does
  /// not expand ther stream.
  virtual Uint64 on_zero(Uint64 _size, Uint64 _offset);

  /// \brief Copy a region in the stream
  /// \param _dst_offset The destination offset of the copy.
  /// \param _src_offset The source offset of the copy.
  /// \returns The number of bytes actually copied.
  virtual Uint64 on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size);

  /// \brief Truncate the stream.
  /// \param _size The total size.
  /// \returns On a successful truncation, \c true. Otherwise, \c false.
  virtual bool on_truncate(Uint64 _size);

protected:
  Uint32 m_flags;
};

inline constexpr UntrackedStream::UntrackedStream(Uint32 _flags)
  : m_flags{_flags}
{
}

inline UntrackedStream::UntrackedStream(UntrackedStream&& context_)
  : m_flags{Utility::exchange(context_.m_flags, 0)}
{
}

inline UntrackedStream::~UntrackedStream() = default;

inline UntrackedStream& UntrackedStream::operator=(UntrackedStream&& context_) {
  if (this != &context_) {
    m_flags = Utility::exchange(context_.m_flags, 0);
  }
  return *this;
}

inline constexpr Uint32 UntrackedStream::flags() const {
  return m_flags;
}

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_UNTRACKED_STREAM_H
