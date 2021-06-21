#ifndef RX_CORE_SERIALIZE_ENCODER_H
#define RX_CORE_SERIALIZE_ENCODER_H
#include "rx/core/stream/buffered_stream.h"
#include "rx/core/stream/tracked_stream.h"

namespace Rx::Serialize {

struct Encoder {
  Encoder(Encoder&& encoder_);

  /// Create an encoder.
  ///
  /// \param _allocator The allocator to use for stream buffering.
  /// \param _stream The underlying stream to write into.
  static Optional<Encoder> create(Memory::Allocator& _allocator,
    Stream::UntrackedStream& _stream);

  // Endianess independent.
  bool put_u8(Uint8 _value);
  bool put_s8(Sint8 _value);

  /// @{
  /// Encoding functions for values of primitive types. These functions convert
  /// the input byte order into little or big endian depending on the host.
  ///
  /// Each function returns a boolean value indicating if the put was successful
  /// or not.
  bool put_u16le(Uint16 _value);
  bool put_s16le(Sint16 _value);

  bool put_u16be(Uint16 _value);
  bool put_s16be(Sint16 _value);

  bool put_u32le(Uint32 _value);
  bool put_s32le(Sint32 _value);

  bool put_u32be(Uint32 _value);
  bool put_s32be(Sint32 _value);

  bool put_u64le(Uint64 _value);
  bool put_s64le(Uint64 _value);

  bool put_u64be(Uint64 _value);
  bool put_s64be(Uint64 _value);

  bool put_f32le(Float32 _value);
  bool put_f32be(Float32 _value);

  bool put_f64le(Float64 _value);
  bool put_f64be(Float64 _value);
  /// @}

  Stream::TrackedStream& stream();

private:
  Encoder(Stream::BufferedStream&& buffered_stream_);

  Stream::BufferedStream m_buffer;
  Stream::TrackedStream m_stream;
};

inline Encoder::Encoder(Stream::BufferedStream&& buffered_stream_)
  : m_buffer{Utility::move(buffered_stream_)}
  , m_stream{m_buffer}
{
}

inline Encoder::Encoder(Encoder&& encoder_)
  : m_buffer{Utility::move(encoder_.m_buffer)}
  , m_stream{m_buffer}
{
  // Copy seek poisition from |encoder_|.
  m_stream.seek(encoder_.m_stream.tell(), Stream::Whence::SET);
}

inline Stream::TrackedStream& Encoder::stream() {
  return m_stream;
}

} // namespace Rx::Serialize

#endif // RX_CORE_SERIALIZE_ENCODER_H