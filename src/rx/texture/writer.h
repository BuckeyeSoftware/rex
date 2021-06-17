#ifndef RX_TEXTURE_WRITER_H
#define RX_TEXTURE_WRITER_H
#include "rx/texture/loader.h"

namespace Rx::Texture {

enum class FileFormat {
  PNG,
  BMP,
  TGA,
  HDR,
  JPG
};

/// Writes a texture file to the output stream from a raw byte array of pixels
/// in the specified pixel format with specified dimensions.
bool write(Memory::Allocator& _allocator, FileFormat _file_format,
  PixelFormat _pixel_format, const Math::Vec2z& _dimensions, const Byte* _data,
  Stream::UntrackedStream& output_);

} // namespace Rx::Texture

#endif // RX_TEXTURE_WRITER_H