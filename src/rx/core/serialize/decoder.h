#ifndef RX_CORE_SERIALIZE_DECODER_H
#define RX_CORE_SERIALIZE_DECODER_H
#include "rx/core/stream/buffered_stream.h"
#include "rx/core/stream/tracked_stream.h"

namespace Rx::Serialize {

struct Decoder {
  Decoder(Decoder&& decoder_);

  /// Create a decoder.
  ///
  /// \param _allocator The allocator to use for stream buffering.
  /// \param _stream The underlying stream to read from.
  static Optional<Decoder> create(Memory::Allocator& _allocator,
    Stream::UntrackedStream& _stream);

  // Endianess independent.
  Optional<Uint8> get_u8();
  Optional<Sint8> get_s8();

  /// @{
  /// Decoding functions for values of primitive types. These functions convert
  /// the byte order into little or big endian depending on the host.
  Optional<Uint16> get_u16le();
  Optional<Sint16> get_s16le();

  Optional<Uint16> get_u16be();
  Optional<Sint16> get_s16be();

  Optional<Uint32> get_u32le();
  Optional<Sint32> get_s32le();

  Optional<Uint32> get_u32be();
  Optional<Sint32> get_s32be();

  Optional<Uint64> get_u64le();
  Optional<Sint64> get_s64le();

  Optional<Uint64> get_u64be();
  Optional<Sint64> get_s64be();

  Optional<Float32> get_f32le();
  Optional<Float32> get_f32be();

  Optional<Float64> get_f64le();
  Optional<Float64> get_f64be();
  /// @}

  Stream::TrackedStream& stream();

private:
  Decoder(Stream::BufferedStream&& buffered_stream_);

  Stream::BufferedStream m_buffer;
  Stream::TrackedStream m_stream;
};

inline Decoder::Decoder(Stream::BufferedStream&& buffered_stream_)
  : m_buffer{Utility::move(buffered_stream_)}
  , m_stream{m_buffer}
{
}

inline Decoder::Decoder(Decoder&& decoder_)
  : m_buffer{Utility::move(decoder_.m_buffer)}
  , m_stream{m_buffer}
{
}

inline Stream::TrackedStream& Decoder::stream() {
  return m_stream;
}

} // namespace Rx::Serialize

#endif // RX_CORE_SERIALIZE_DECODER_H
