#ifndef RX_CORE_STREAM_MEMORY_STREAM_H
#define RX_CORE_STREAM_MEMORY_STREAM_H
#include "rx/core/stream/context.h"
#include "rx/core/string.h"

namespace Rx::Stream {

/// \brief %Stream back by memory.
///
/// A MemoryStream has the same interface as a Context except stream
/// operations occur on memory given to the MemoryStream.
struct MemoryStream
  : Context
{
  /// @{
  /// Construct a MemoryStream
  ///
  /// \param name_ The name to associate with this stream.
  /// \param _span The span of memory to associate with this stream.
  ///
  /// \note The const span version configures the stream as read-only, while the
  /// non-const version permits read and write operations.
  MemoryStream(String&& name_, Span<const Byte> _span);
  MemoryStream(String&& name_, Span<Byte> span_);
  /// @}

  /// \brief Read data.
  /// Reads \p _size bytes from the stream at \p _offset into \p data_.
  /// \returns The number of bytes actually read from \p _offset into \p data_.
  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);

  /// \brief Write data.
  /// Writes \p _size bytes from \p _data into the steam at \p _offset.
  /// \returns The number of bytes actually written at \p _offset from \p _data.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);

  /// \brief Stat the stream for information.
  /// Updates the stat object referenced by \p stat_.
  /// \returns On a successful stat, \c true. Otherwise, \c false.
  virtual bool on_stat(Stat& stat_) const;

  /// The name of the stream.
  virtual const String& name() const &;

private:
  Byte* m_data;
  Uint64 m_capacity;
  Uint64 m_size;
  String m_name;
};

inline MemoryStream::MemoryStream(String&& name_, Span<const Byte> _span)
  : Context{READ | STAT}
  , m_data{const_cast<Byte*>(_span.data())}
  , m_capacity{_span.size()}
  , m_size{_span.size()}
  , m_name{Utility::move(name_)}
{
}

inline MemoryStream::MemoryStream(String&& name_, Span<Byte> span_)
  : Context{READ | WRITE | STAT}
  , m_data{span_.data()}
  , m_capacity{span_.size()}
  , m_size{0}
  , m_name{Utility::move(name_)}
{
}

inline const String& MemoryStream::name() const & {
  return m_name;
}

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_MEMORY_STREAM_H