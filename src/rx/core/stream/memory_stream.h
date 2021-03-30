#ifndef RX_CORE_STREAM_MEMORY_STREAM_H
#define RX_CORE_STREAM_MEMORY_STREAM_H
#include "rx/core/stream/context.h"
#include "rx/core/string.h"

namespace Rx::Stream {

struct MemoryStream
  : Context
{
  MemoryStream(String&& name_, Span<const Byte> _span);
  MemoryStream(String&& name_, Span<Byte> span_);

  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stat& stat_) const;

  virtual const String& name() const & {
    return m_name;
  }

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

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_MEMORY_STREAM_H