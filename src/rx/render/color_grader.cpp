#include <string.h> // memcpy
#include <stdlib.h> // strtol
#include <stdio.h> // sscanf

#include "rx/render/color_grader.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"

#include "rx/core/global.h"

#include "rx/core/math/fract.h"
#include "rx/core/math/half.h"

#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/filesystem/file.h"

#include "rx/texture/loader.h"

namespace Rx::Render {

struct Cube {
  Cube(Memory::Allocator& _allocator) : data{_allocator}, size{0} {}

  [[nodiscard]] bool resize(Size _size) {
    size = _size;
    return data.resize(size * size * size);
  }

  Vector<Math::Vec4h> data;
  Size size;
};

// Remap |_x| with input range [|_in_min|, |_in_max|] to output in range [|_out_min|, |_out_max|].
static Math::Half remap(Float32 _x, Float32 _in_min, Float32 _in_max, Float32 _out_min, Float32 _out_max) {
  return (_x - _in_min) * (_out_max - _out_min) / (_in_max - _in_min) + _out_min;
}

// Remap the triplet |_value| into half-float [0, 1] range based on the domain |_min| and |max|.
static Math::Vec3h remap(const Math::Vec3f& _value, const Math::Vec3f& _min, const Math::Vec3f& _max) {
  return {
    remap(_value.x, _min.x, _max.x, 0.0f, 1.0f),
    remap(_value.y, _min.y, _max.y, 0.0f, 1.0f),
    remap(_value.z, _min.z, _max.z, 0.0f, 1.0f)
  };
};

// Loads an Adobe Cube file.
static Optional<Cube> load_cube(Memory::Allocator& _allocator, const String& _file_name) {
  Cube cube{_allocator};

  auto data = Filesystem::read_text_file(_allocator, _file_name);
  if (!data) {
    return nullopt;
  }

  // Read each line of the file.
  auto next_line = [&](char*& line_) -> char* {
    if (!line_ || !*line_) {
      return nullptr;
    }
    char* line = line_;
    line_ += strcspn(line_, "\n");
    *line_++ = '\0';
    return line;
  };

  // When no domain is specified, the default domain is [{0, 0, 0}, {1, 1, 1}].
  Math::Vec3f min{0.0f, 0.0f, 0.0f};
  Math::Vec3f max{1.0f, 1.0f, 1.0f};

  Vector<Math::Vec3h> samples{_allocator};
  auto token = reinterpret_cast<char*>(data->data());
  while (auto line = next_line(token)) {
    // Skip empty and comment lines.
    if (!*line || *line == '#') {
      continue;
    }

    // Skip "TITLE" line.
    if (!strncmp(line, "TITLE", 5)) {
      continue;
    }

    // Read domain min and max extents. We'll be remapping samples to [0, 1].
    if (sscanf(line, "DOMAIN_MIN %f %f %f", &min.x, &min.y, &min.z) == 3) {
      continue;
    }
    if (sscanf(line, "DOMAIN_MAX %f %f %f", &max.x, &max.y, &max.z) == 3) {
      continue;
    }

    // Read the 3D LUT size.
    if (Size size; sscanf(line, "LUT_3D_SIZE %zu", &size) == 1) {
      if (!cube.resize(size)) {
        return nullopt;
      }
      continue;
    }

    // Cannot read any RGB triplets if LUT_3D_SIZE was not seen.
    if (!cube.size) {
      break;
    }

    // Read a sample, remap it, and append.
    Math::Vec3f sample{0.0f, 0.0f, 0.0f};
    if (sscanf(line, "%f %f %f", &sample.r, &sample.g, &sample.b) != 3) {
      return nullopt;
    }

    const auto value = remap(sample, min, max);
    if (!samples.emplace_back(value.r, value.g, value.b)) {
      return nullopt;
    }
  }

  // Swizzle it from vertical to horizontal.
  const auto size = cube.size;
  for (Size y = 0; y < size; y++) {
    for (Size z = 0; z < size; z++) {
      for (Size x = 0; x < size; x++) {
        const auto& src = samples[z * size * size + (y * size + x)];
        cube.data[y * size * size + (z * size + x)] = {src.r, src.g, src.b, 1.0f};
      }
    }
  }

  return cube;
}

// [ColorGrader]
Optional<ColorGrader::Entry> ColorGrader::load(const String& _file_name) {
  auto& allocator = m_frontend->allocator();

  auto find_or_create_atlas = [&](Size _size) -> Atlas* {
    Concurrency::ScopeLock lock{m_atlases_lock};
    // Check if an atlas already exists for |_size|.
    if (auto atlas = m_atlases.find(_size)) {
      return atlas;
    }
    // Create a new atlas.
    if (auto atlas = Atlas::create(this, _size)) {
      return m_atlases.insert(_size, Utility::move(*atlas));
    }
    return nullptr;
  };

  // Check if Adobe .CUBE first.
  if (_file_name.ends_with(".cube") || _file_name.ends_with(".CUBE")) {
    auto data = load_cube(allocator, _file_name);
    if (!data) {
      return nullopt;
    }
    auto atlas = find_or_create_atlas(data->size);
    if (!atlas) {
      return nullopt;
    }

    auto entry = atlas->allocate_uninitialized();
    if (!entry) {
      return nullopt;
    }

    entry->write(data->data.data());

    return entry;
  }

  // Use regular texture loader.
  Texture::Loader loader{allocator};
  if (!loader.load(_file_name, Texture::PixelFormat::RGBA_U8, {4096, 4096})) {
    return nullopt;
  }

  auto atlas = find_or_create_atlas(loader.dimensions().h);
  if (!atlas) {
    return nullopt;
  }

  auto entry = atlas->allocate_uninitialized();
  if (!entry) {
    return nullopt;
  }

  // Write the data into the entry.
  auto data = reinterpret_cast<const Math::Vec4b*>(loader.data().data());
  entry->write(data);

  return entry;
}

void ColorGrader::update() {
  Concurrency::ScopeLock lock{m_atlases_lock};
  m_atlases.each_value([](Atlas& _atlas) {
    _atlas.update();
  });
}

// [ColorGrader::Entry]
void ColorGrader::Entry::write(const Math::Vec4f* _samples) {
  // Convert |_samples| to half-precision float.
  Concurrency::ScopeLock lock{m_atlas->m_scratch_lock};
  auto& samples = m_atlas->m_scratch;
  const auto n_samples = m_atlas->m_size * m_atlas->m_size * m_atlas->m_size;
  for (Size i = 0; i < n_samples; i++) {
    samples[i] = _samples[i].cast<Math::Half>();
  }
  return write(samples.data());
}

void ColorGrader::Entry::write(const Math::Vec4h* _samples) {
  const auto texture = m_atlas->m_texture;
  const auto size = m_atlas->m_size;

  // Need to swizzle Z and Y here.
  const auto src = _samples;
  const auto dst = reinterpret_cast<Math::Vec4h*>(texture->map(0));
  for (Size z = 0; z < size; z++) {
    for (Size y = 0; y < size; y++) {
      memcpy(dst + (z + size * m_handle) * size * size + y * size,
        src + z * size + y * size * size, sizeof(Math::Vec4h) * size);
    }
  }

  // This code is what would be used if we didn't need to swizzle incoming.
  //
  // auto src = _samples;
  // auto dst = m_texture->map(0) + (size * _handle * size * size) * sizeof(Math::Vec4h);
  // auto len = size * size * sizeof(Math::Vec4h);
  // for (Size z = 0; z < size; z++) {
  //   memcpy(dst, src, len);
  //   src += len;
  //   dst += len;
  // }

  // Mark the entry as dirty in the atlas.
  m_atlas->m_dirty.set(m_handle);
}

void ColorGrader::Entry::write(const Math::Vec4b* _samples) {
  // Convert |_samples| to half-precision float.
  Concurrency::ScopeLock lock{m_atlas->m_scratch_lock};
  auto& samples = m_atlas->m_scratch;
  const auto n_samples = m_atlas->m_texture->dimensions().area();
  for (Size i = 0; i < n_samples; i++) {
    samples[i] = (_samples[i].cast<Float32>() * (1.0f / 255.0f)).cast<Math::Half>();
  }
  return write(samples.data());
}

Math::Vec2f ColorGrader::Entry::properties() const {
  const auto size = m_atlas->m_texture->dimensions().w;
  // Calculate the scale and offset in the atlas for this entry. This clamps the
  // UVs in such a way to stay on pixel centers to avoid sampling adjacent slices
  // when at the boundary slice of the entry.
  const auto uvs_per_slice = 1.0f / MAX_DEPTH;
  const auto uvs_per_lut = (uvs_per_slice * size);
  const auto scale = (size - 1.0f) / MAX_DEPTH;
  const auto offset = uvs_per_slice * 0.5f + uvs_per_lut * m_handle;
  return {scale, offset};
}

// [ColorGrader::Atlas]
Optional<ColorGrader::Atlas> ColorGrader::Atlas::create(ColorGrader* _context, Size _size) {
  if (_size == 0) {
    return nullopt;
  }

  auto frontend = _context->m_frontend;
  auto& allocator = frontend->allocator();

  // Create the neutral table.
  Vector<Math::Vec4h> neutral{allocator};
  if (!neutral.resize(_size * _size * _size)) {
    return nullopt;
  }

  const Math::Vec2z dimensions{_size * _size, _size};
  for (Size y = 0; y < dimensions.h; y++) {
    for (Size x = 0; x < dimensions.w; x++) {
      auto u = Float32(x) / (dimensions.w - 1) * ((dimensions.w - 1.0f) / dimensions.w);
      auto v = Float32(y) / (dimensions.h - 1) * ((dimensions.h - 1.0f) / dimensions.h);

      Math::Vec4f uvw;
      uvw.x = Math::fract(u * _size);

      uvw.y = v;
      uvw.z = u - uvw[0] / _size;
      uvw.w = 1.0f;

      uvw.x *= _size / Float32(_size - 1);
      uvw.y *= _size / Float32(_size - 1);
      uvw.z *= _size / Float32(_size - 1);

      neutral[dimensions.w * y + x] = uvw.cast<Math::Half>();
    }
  }

  auto allocated = Bitset::create(allocator, MAX_DEPTH / _size);
  auto dirty = Bitset::create(allocator, MAX_DEPTH / _size);
  if (!allocated || !dirty) {
    return nullopt;
  }

  auto texture = frontend->create_texture3D(RX_RENDER_TAG("Atlas"));
  if (!texture) {
    return nullopt;
  }

  texture->record_type(Frontend::Texture::Type::DYNAMIC);
  texture->record_levels(1);
  texture->record_filter({true, false, false});
  texture->record_format(Frontend::Texture::DataFormat::RGBA_F16);
  texture->record_dimensions({_size, _size, MAX_DEPTH});
  texture->record_wrap({
    Render::Frontend::Texture::WrapType::CLAMP_TO_EDGE,
    Render::Frontend::Texture::WrapType::CLAMP_TO_EDGE,
    Render::Frontend::Texture::WrapType::CLAMP_TO_EDGE});

  frontend->initialize_texture(RX_RENDER_TAG("Atlas"), texture);

  Atlas atlas;
  atlas.m_color_grader = _context;
  atlas.m_texture = texture;
  atlas.m_allocated = Utility::move(*allocated);
  atlas.m_dirty = Utility::move(*dirty);
  atlas.m_neutral = Utility::move(neutral);
  atlas.m_size = _size;

  return atlas;
}

void ColorGrader::Atlas::release() {
  if (m_color_grader) {
    m_color_grader->m_frontend->destroy_texture(RX_RENDER_TAG("Atlas"), m_texture);
  }
}

Optional<ColorGrader::Entry> ColorGrader::Atlas::allocate() {
  auto entry = allocate_uninitialized();
  if (!entry) {
    return nullopt;
  }
  entry->write(m_neutral.data());
  return entry;
}

void ColorGrader::Atlas::update() {
  // Nothing to update.
  if (m_dirty.count_set_bits() == 0) {
    return;
  }

  const auto size = m_texture->dimensions().w;

  // Record all the edits to the atlas and update the texture.
  m_dirty.each_set([&](Bitset::BitType _texture) {
    m_texture->record_edit(0, {0, 0, Size(size * _texture)}, {size, size, size});
  });

  m_color_grader->m_frontend->update_texture(RX_RENDER_TAG("Atlas"), m_texture);

  m_dirty.clear();
}

Optional<ColorGrader::Entry> ColorGrader::Atlas::allocate_uninitialized() {
  auto index = m_allocated.find_first_unset();
  if (!index) {
    return nullopt;
  }
  m_allocated.set(*index);
  return Entry{this, Uint16(*index)};
}

} // namespace Rx::Render
