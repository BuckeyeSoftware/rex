#ifndef RX_TEXTURE_WRITER_H
#define RX_TEXTURE_WRITER_H
#include "rx/texture/format.h"

#include "rx/math/vec2.h"

namespace Rx::Stream { struct Context; }
namespace Rx::Memory { struct Allocator; }

namespace Rx::Texture {

/// Writes a texture file to the output stream from a raw byte array of pixels
/// in the specified pixel format with specified dimensions.
bool write(Memory::Allocator& _allocator, FileFormat _file_format,
  PixelFormat _pixel_format, const Math::Vec2z& _dimensions, const Byte* _data,
  Stream::Context& output_);

} // namespace Rx::Texture

#endif // RX_TEXTURE_WRITER_H