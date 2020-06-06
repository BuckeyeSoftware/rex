
#if 0
static void write_noise_map(const Vector<rx_f32>& _noise_map, const math::vec2i& _dimensions) {
  filesystem::file out{"test.ppm", "w"};
  out.print("P3\n");
  out.print("%zu %zu\n", _dimensions.w, _dimensions.h);
  out.print("255\n");

  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const auto value{static_cast<rx_s32>(_noise_map[_dimensions.w * y + x] * 255.0f)};
      out.print("% 3d", value);
      out.print("% 3d", value);
      out.print("% 3d", value);
    }
    out.print("\n");
  }
}

static void write_color_map(const Vector<math::vec3f>& _color_map, const math::vec2i& _dimensions) {
  filesystem::file out{"test.ppm", "w"};
  out.print("P3\n");
  out.print("%d %d\n", _dimensions.w, _dimensions.h);
  out.print("255\n");
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const auto value{(_color_map[_dimensions.w * y + x] * math::vec3f(255.0f, 255.0f, 255.0f)).cast<rx_s32>()};
      out.print("% 3d", value.r);
      out.print("% 3d", value.g);
      out.print("% 3d", value.b);
    }
    out.print("\n");
  }
}

static rx_f32 perlin_noise(rx_f32 _x, rx_f32 _y) {
  // return 0.0f;
  math::noise::perlin a{100};
  return a.noise(_x, _y);
}

rx_f32 inverse_lerp(rx_f32 _min, rx_f32 _max, rx_f32 _value) {
  if (math::abs(_max - _min) < 0.0001f) {
    return _min;
  }
  return (_value - _min) / (_max - _min);
}

struct terrain_type {
  math::vec3f color;
  rx_f32 height;
};

static Vector<rx_f32> generate_noise_map(const math::vec2i& _dimensions, rx_f32 _scale, int _octaves, rx_f32 _persistance, rx_f32 _lacunarity) {
  if (_scale <= 0.0f) {
    _scale = 0.0001f;
  }

  Vector<rx_f32> noise_map{static_cast<rx_size>(_dimensions.area())};
  Vector<math::vec2f> octave_offsets{static_cast<rx_size>(_octaves)};
  for (rx_s32 i{0}; i < _octaves; i++) {
    const rx_f32 x{0.0f};
    const rx_f32 y{0.0f};
    octave_offsets[i] = {x, y};
  }

  rx_f32 max_noise_height{-FLT_MAX};
  rx_f32 min_noise_height{ FLT_MAX};

  // Generate from middle out.
  const rx_f32 half_w{_dimensions.x / 2.0f};
  const rx_f32 half_h{_dimensions.y / 2.0f};

  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      rx_f32 amplitude{1.0f};
      rx_f32 frequency{1.0f};
      rx_f32 noise_height{0.0f};

      for (rx_s32 i{0}; i < _octaves; i++) {
        const rx_f32 sample_x{(static_cast<rx_f32>(x) - half_w) / _scale * frequency + octave_offsets[i].x};
        const rx_f32 sample_y{(static_cast<rx_f32>(y) - half_h) / _scale * frequency + octave_offsets[i].y};

        const rx_f32 perlin_value{perlin_noise(sample_x, sample_y)};

        noise_height += perlin_value * amplitude;
        amplitude *= _persistance;
        frequency *= _lacunarity;
      }

      max_noise_height = algorithm::max(max_noise_height, noise_height);
      min_noise_height = algorithm::min(min_noise_height, noise_height);

      noise_map[_dimensions.w * y + x] = noise_height;
    }
  }

  // Normalize
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      noise_map[_dimensions.w * y + x] = inverse_lerp(min_noise_height, max_noise_height, noise_map[_dimensions.w * y + x]);
    }
  }

  return noise_map;
}

static Vector<math::vec3f> generate_map(const Vector<terrain_type>& _terrain_types, const math::vec2i& _dimensions, rx_f32 _scale, int _octaves, rx_f32 _persistence, rx_f32 _lacunarity) {
  const auto& noise_map{generate_noise_map(_dimensions, _scale, _octaves, _persistence, _lacunarity)};
  Vector<math::vec3f> color_map{static_cast<rx_size>(_dimensions.area())};
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const rx_f32 current_height{noise_map[_dimensions.w * y + x]};
      _terrain_types.each_fwd([&](const terrain_type& _terrain_type) {
        if (current_height <= _terrain_type.height) {
          color_map[_dimensions.w * y + x] = _terrain_type.color;
          return false;
        }
        return true;
      });
    }
  }
  return color_map;
}

struct terrain_mesh {
  struct vertex {
    math::vec3f position;
    math::vec2f coordinate;
  };

  Vector<vertex> vertices;
  Vector<rx_u32> triangles;
  rx_size triangle_index;

  terrain_mesh(const math::vec2i& _dimensions) {
    triangle_index = 0;
    vertices.resize(_dimensions.area());
    triangles.resize((_dimensions.w - 1) * (_dimensions.h - 1) * 3 * 2);
  }

  void add_triangle(rx_u32 _a, rx_u32 _b, rx_u32 _c) {
    triangles[triangle_index++] = _a;
    triangles[triangle_index++] = _b;
    triangles[triangle_index++] = _c;
  }
};

static terrain_mesh generate_terrain_mesh(const Vector<rx_f32>& _height_map, const math::vec2i& _dimensions) {
  terrain_mesh mesh{_dimensions};
  rx_size vertex_index{0};
  const rx_f32 top_left_x{(_dimensions.w - 1) / -2.0f};
  const rx_f32 top_left_z{(_dimensions.h - 1) /  2.0f};
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const rx_f32 height{_height_map[_dimensions.w * y + x]};
      mesh.vertices[vertex_index++] = {
        {top_left_x + x, height, top_left_z - y},
        {x / static_cast<rx_f32>(_dimensions.w), y / static_cast<rx_f32>(_dimensions.h)}};
      if (x < _dimensions.w - 1 && y < _dimensions.h - 1) {
        mesh.add_triangle(vertex_index, vertex_index + -_dimensions.w + 1, vertex_index + _dimensions.w);
        mesh.add_triangle(vertex_index + _dimensions.w + 1, vertex_index, vertex_index + 1);
      }
    }
  }
  return mesh;
}

#endif
