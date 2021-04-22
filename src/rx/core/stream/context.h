#ifndef RX_CORE_STREAM_CONTEXT_H
#define RX_CORE_STREAM_CONTEXT_H
#include "rx/core/linear_buffer.h"
#include "rx/core/optional.h"

#include "rx/core/stream/operations.h"

namespace Rx {
  struct String;
} // namespace Rx

/// Streaming IO.
namespace Rx::Stream {

/// \brief %Stream context
///
/// An interface streams can implement.
struct RX_API Context {
  /// \brief Construct a stream context.
  /// \param _flags The flags indicating what features this stream support
  constexpr Context(Uint32 _flags);

  /// \brief Move constructor.
  /// \param stream_ The stream to move from.
  Context(Context&& stream_);

  /// Destroy a stream context.
  virtual ~Context();

  /// \brief Move assignment operator.
  /// \param stream_ The stream to move from.
  Context& operator=(Context&& stream_);

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
  /// \returns The number of bytes actually written.s
  [[nodiscard]] Uint64 write(const Byte* _data, Uint64 _size);

  /// Stat stream for info.
  Optional<Stat> stat() const;

  /// \brief Flush the stream.
  /// \returns On a successful flush, \c true. Otherwise, \c false.
  /// \note Flushing can fail if the flush couldn't write all the bytes
  /// necessary, or the underlying stream does not support flushing. You can
  /// check that a stream supports flushing by checking for the #FLUSH flag in
  /// flags().
  [[nodiscard]] bool flush();

  /// \brief Truncate the stream.
  /// \param _size The total size.
  /// \returns On a successful truncation, \c true. Otherwise, \c false.
  /// \note Truncation can fail if the underlying stream does not support
  /// truncation. You can check that a stream supports truncation by checking
  /// for the #TRUNCATION flag in flags().
  [[nodiscard]] bool truncate(Uint64 _size);

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
  ///
  /// \note You can check that a stream supports seeking by checking for the
  /// #SEEK flag in flags().
  [[nodiscard]] bool seek(Sint64 _where, Whence _whence);

  /// \brief Rewind the stream.
  /// This is a convenience and faster routine for `seek(0, SET)`.
  void rewind();

  /// Determine the byte offset in the stream the cursor is at.
  Uint64 tell() const;

  /// \brief Get the size of the stream.
  /// \returns On a successful call, the size. Otherwise, \c nullopt.
  /// \note Querying the size can fail if the underlying stream does not support
  /// stating. You can check that a stream supports stating byt checking for the
  /// #STAT flag in flags().
  Optional<Uint64> size() const;

  /// Get flags of the stream.
  constexpr Uint32 flags() const;

  /// Check if cursor is at end-of-stream.
  constexpr bool is_eos() const;

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
  /// \param stat_ The stat object to fill.
  /// Updates the stat object referenced by \p stat_.
  /// \returns On a successful stat, \c true. Otherwise, \c false.
  virtual bool on_stat(Stat& stat_) const;

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
  // End-of-stream flag. This is set when |on_read| returns a truncated result.
  static inline constexpr Uint32 EOS = 1 << 31;
  Uint32 m_flags;
  Uint64 m_offset;
};

inline constexpr Context::Context(Uint32 _flags)
  : m_flags{_flags}
  , m_offset{0}
{
}

inline Context::Context(Context&& context_)
  : m_flags{Utility::exchange(context_.m_flags, 0)}
  , m_offset{Utility::exchange(context_.m_offset, 0)}
{
}

inline Context::~Context() = default;

inline Context& Context::operator=(Context&& context_) {
  if (this != &context_) {
    m_flags = Utility::exchange(context_.m_flags, 0);
    m_offset = Utility::exchange(context_.m_offset, 0);
  }
  return *this;
}

inline Uint64 Context::tell() const {
  return m_offset;
}

inline Optional<Uint64> Context::size() const {
  if (auto s = stat()) {
    return s->size;
  }
  return nullopt;
}

inline constexpr bool Context::is_eos() const {
  return m_flags & EOS;
}

inline constexpr Uint32 Context::flags() const {
  return m_flags;
}

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_CONTEXT_H
