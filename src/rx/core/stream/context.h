#ifndef RX_CORE_STREAM_CONTEXT_H
#define RX_CORE_STREAM_CONTEXT_H
#include "rx/core/linear_buffer.h"
#include "rx/core/optional.h"

#include "rx/core/stream/operations.h"

namespace Rx {
  struct String;
} // namespace Rx

namespace Rx::Stream {

struct RX_API Context {
  constexpr Context(Uint32 _flags);
  Context(Context&& stream_);
  virtual ~Context();
  Context& operator=(Context&& stream_);

  // Read |_size| bytes into |_data|. Returns the amount of bytes written.
  [[nodiscard]] Uint64 read(Byte* _data, Uint64 _size);

  // Write |_size| bytes from |_data|. Returns the amount of bytes written.
  [[nodiscard]] Uint64 write(const Byte* _data, Uint64 _size);

  // Stat stream for info.
  Optional<Stat> stat() const;

  // Flush the stream.
  [[nodiscard]] bool flush();

  // Seek stream |_where| bytes relative to |_whence|. Returns true on success.
  [[nodiscard]] bool seek(Sint64 _where, Whence _whence);

  // Rewind the stream. Convenience and prose alias for seek(0, SET).
  void rewind();

  // Where in the stream we are.
  Uint64 tell() const;

  // The size of the stream.
  Optional<Uint64> size() const;

  // Get flags of the stream.
  constexpr Uint32 flags() const;

  // Check if end-of-stream.
  constexpr bool is_eos() const;

  // The name of the stream. This must always be implemented.
  virtual const String& name() const & = 0;

  Optional<LinearBuffer> read_binary(Memory::Allocator& _allocator);
  Optional<LinearBuffer> read_text(Memory::Allocator& _allocator);

  // Read |_size| bytes from stream at |_offset| into |_data|.
  virtual Uint64 on_read(Byte* _data, Uint64 _size, Uint64 _offset);

  // Write |_size| bytes from |_data| into stream at |_offset|.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);

  // Stat the stream.
  virtual bool on_stat(Stat& stat_) const;

  // Optional flushing of the stream.
  virtual bool on_flush();

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
