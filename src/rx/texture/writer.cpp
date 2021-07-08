#include "rx/texture/writer.h"
#include "rx/texture/convert.h"

#include "rx/core/stream/advancing_stream.h"

#include "lib/stb_image_write.h"

namespace Rx::Texture {

// STBIW only supports writing component order RGBA, so any BGR / BGRA needs to
// be swizzled correctly before writing. Similarly, STBIW only supports 8-bit
// pixels in all but HDR output, so to handle conversion of RGBA_F32 to SRGBA_U8
// and all 8-bit pixel formats to RGBA_F32 when outputting HDR this function
// is used to query when input data needs to be reformatted for STBIW.
static Optional<PixelFormat> needs_reformat(PixelFormat _pixel_format, bool _hdr) {
  // This is needed because ternary cannot be used on Optional<T> in C++.
  auto if_hdr = [](bool _hdr, auto _on_true, auto _on_false) -> Optional<PixelFormat> {
    if (_hdr) {
      return _on_true;
    } else {
      return _on_false;
    }
  };

  switch (_pixel_format) {
  case PixelFormat::R_U8:     // to RGBA_F32 when HDR, leave otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, nullopt);
  case PixelFormat::RGB_U8:   // to RGBA_F32 when HDR, leave otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, nullopt);
  case PixelFormat::RGBA_U8:  // to RGBA_F32 when HDR, leave otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, nullopt);
  case PixelFormat::BGR_U8:   // to RGBA_F32 when HDR, to RGBA_U8 otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, PixelFormat::RGB_U8);
  case PixelFormat::BGRA_U8:  // to RGBA_F32 when HDR, to RGBA_U8 otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, PixelFormat::RGBA_U8);
  case PixelFormat::SRGB_U8:  // to RGBA_F32 when HDR, leave otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, nullopt);
  case PixelFormat::SRGBA_U8: // to RGBA_F32 when HDR, leave otherwise.
    return if_hdr(_hdr, PixelFormat::RGBA_F32, nullopt);
  case PixelFormat::RGBA_F32: // leave when HDR, to SRGBA_U8 otherwise.
    return if_hdr(_hdr, nullopt, PixelFormat::SRGBA_U8);
  }

  RX_HINT_UNREACHABLE();
}

struct PixelFormatInfo {
  Size bits_per_component;
  Size components;
  Size bits_per_pixel() const {
    return bits_per_component * components;
  }
};

static inline PixelFormatInfo pixel_format_info(PixelFormat _pixel_format) {
  switch (_pixel_format) {
  case PixelFormat::R_U8:
    return {8, 1};
  case PixelFormat::RGB_U8:
    return {8, 3};
  case PixelFormat::RGBA_U8:
    return {8, 4};
  case PixelFormat::BGR_U8:
    return {8, 3};
  case PixelFormat::BGRA_U8:
    return {8, 4};
  case PixelFormat::SRGB_U8:
    return {8, 3};
  case PixelFormat::SRGBA_U8:
    return {8, 4};
  case PixelFormat::RGBA_F32:
    return {32, 4};
  }

  RX_HINT_UNREACHABLE();
}

// STBIW's write callback does not have a return result so it cannot report
// write errors through it's return value. Here just wrap the TrackedStream in
// a context struct with values to track the expected and wrote byte counts.
// When these do not match, it means the underlying stream encountered a write
// error. The write callback |cb| will stop writing to the stream when this
// occurs.
struct Context {
  Stream::AdvancingStream stream;
  Uint64 wrote = 0;
  Uint64 expected = 0;
  bool valid() const { return wrote == expected; }
};

static void cb(void* _context, void* _data, int _size) {
  const auto context = static_cast<Context*>(_context);
  const auto data = reinterpret_cast<const Byte*>(_data);
  if (context->valid()) {
    context->expected = _size;
    context->wrote = context->stream.write(data, _size);
  }
}

bool write(Memory::Allocator& _allocator, FileFormat _file_format,
  PixelFormat _pixel_format, const Math::Vec2z& _dimensions, const Byte* _data,
  Stream::Context& output_)
{
  Context context{output_};
  const auto ctx = static_cast<void*>(&context);

  Optional<LinearBuffer> reformatted;
  const auto reformat = needs_reformat(_pixel_format, _file_format == FileFormat::HDR);
  if (reformat) {
    reformatted = convert(_allocator, _data, _dimensions.area(), _pixel_format, *reformat);
    if (!reformatted) {
      return false;
    }
  }

  const auto info = pixel_format_info(reformat ? *reformat : _pixel_format);
  const auto data = reformatted ? reformatted->data() : _data;

  const auto dimensions = _dimensions.cast<Sint32>();
  const auto stride = (info.bits_per_pixel() * dimensions.w) / 8;

  const auto w = dimensions.w;
  const auto h = dimensions.h;
  const auto c = info.components;

  switch (_file_format) {
  case FileFormat::PNG:
    return stbi_write_png_to_func(cb, ctx, w, h, c, data, stride) != 0 && context.valid();
  case FileFormat::TGA:
    return stbi_write_tga_to_func(cb, ctx, w, h, c, data) != 0 && context.valid();
  case FileFormat::HDR:
    // Special case since HDR only accepts float samples and the function needs
    // a pointer to float.
    {
      const auto samples = reinterpret_cast<const float*>(data);
      return stbi_write_hdr_to_func(cb, ctx, w, h, c, samples) != 0
          && context.valid();
    }
    break;
  case FileFormat::JPG:
    return stbi_write_jpg_to_func(cb, ctx, w, h, c, data, 75) != 0 && context.valid();
  }

  return false;
}

} // namespace Rx::Texture