#include "rx/texture/dxt.h"
#include "rx/core/algorithm/clamp.h"

namespace Rx::Texture {

static constexpr Size k_refine_iterations{3};

enum class Color {
  k_33,
  k_66,
  k_50
};

struct Block {
  Uint16 color0;
  Uint16 color1;
  Uint32 pixels;
};

static Uint16 pack_565(Uint16 _r, Uint16 _g, Uint16 _b) {
  return ((_r & 0xF8) << 8) | ((_g & 0xFC) << 3) | (_b >> 3);
}

static void unpack_565(Uint16 _src, Uint16& r_, Uint16& g_, Uint16& b_) {
  r_ = (((_src >> 11) & 0x1F) * 527 + 15) >> 6;
  g_ = (((_src >> 5) & 0x3F) * 259 + 35) >> 6;
  b_ = ((_src & 0x1F) * 527 + 15) >> 6;
}

template<Color E>
static Uint16 calculate_color(Uint16 _color0, Uint16 _color1) {
  Uint16 r[3];
  Uint16 g[3];
  Uint16 b[3];

  unpack_565(_color0, r[0], g[0], b[0]);
  unpack_565(_color1, r[1], g[1], b[1]);

  if constexpr (E == Color::k_33) {
    r[2] = (2*r[0] + r[1]) / 3;
    g[2] = (2*g[0] + g[1]) / 3;
    b[2] = (2*b[0] + b[1]) / 3;
  } else if constexpr (E == Color::k_66) {
    r[2] = (r[0] + 2*r[1]) / 3;
    g[2] = (g[0] + 2*g[1]) / 3;
    b[2] = (b[0] + 2*b[1]) / 3;
  } else if constexpr (E == Color::k_50) {
    r[2] = (r[0] + r[1]) / 2;
    g[2] = (g[0] + g[1]) / 2;
    b[2] = (b[0] + b[1]) / 2;
  }

  return pack_565(r[2], g[2], b[2]);
}

template<DXTType T>
static Size optimize(Byte* data_, Size _width, Size _height) {
  Size count{0};
  const Size num_blocks{(_width / 4) * (_height / 4)};

  auto *data = reinterpret_cast<Block*>(data_) + (T == DXTType::k_dxt5); // DXT5: alpha block is first
  for (Size i{0}; i != num_blocks; i++, data += (T == DXTType::k_dxt1 ? 1 : 2)) {
    const Uint16 color0{data->color0};
    const Uint16 color1{data->color1};
    const Uint32 pixels{data->pixels};

    if (pixels == 0) {
      // solid color0
      data->color1 = 0;
      count++;
    } else if (pixels == 0x55555555u) {
      // solid color1, fill with color0 instead, possibly encoding the block
      // as 1-bit alpha if color1 is black.
      data->color0 = color1;
      data->color1 = 0;
      data->pixels = 0;
      count++;
    } else if (pixels == 0xAAAAAAAAu) {
      // solid color2, fill with color0 instead, possibly encoding the block
      // as 1-bit alpha if color2 is black.
      data->color0 = (color0 > color1 || T == DXTType::k_dxt5)
        ? calculate_color<Color::k_33>(color0, color1)
        : calculate_color<Color::k_50>(color0, color1);
      data->color1 = 0;
      data->pixels = 0;
      count++;
    } else if (pixels == 0xFFFFFFFFu) {
      // solid color3
      if (color0 > color1 || T == DXTType::k_dxt5) {
        // fill with color0 instead, possibly encoding the block as 1-bit
        // alpha if color3 is black.
        data->color0 = calculate_color<Color::k_66>(color0, color1);
        data->color1 = 0;
        data->pixels = 0;
        count++;
      } else {
        // transparent / solid black
        data->color0 = 0;
        data->color1 = T == DXTType::k_dxt1 ? 0xFFFFu : 0; // kDXT1: transparent black
        if constexpr (T == DXTType::k_dxt5) {
          // solid black
          data->pixels = 0;
        }
        count++;
      }
    } else if (T == DXTType::k_dxt5 && (pixels & 0xAAAAAAAAu) == 0xAAAAAAAAu) {
      // only interpolated colors are used, not the endpoints
      data->color0 = calculate_color<Color::k_66>(color0, color1);
      data->color1 = calculate_color<Color::k_33>(color0, color1);
      data->pixels = ~pixels;
      count++;
    } else if (T == DXTType::k_dxt5 && color0 < color1) {
      // otherwise, ensure the colors are always in the same order
      data->color0 = color1;
      data->color1 = color0;
      data->pixels ^= 0x55555555u;
      count++;
    }
  }

  return count;
}

template<Size C>
static inline void compute_color_line(const Byte *const _uncompressed,
  Float32 (&point_)[3], Float32 (&direction_)[3])
{
  static constexpr Float64 k_sixteen{16.0};
  static constexpr Float64 k_one{1.0};
  static constexpr Float64 k_zero{0.0};
  static constexpr Float64 k_inv16{k_one / k_sixteen};

  Float64 sum_r{k_zero};
  Float64 sum_g{k_zero};
  Float64 sum_b{k_zero};
  Float64 sum_rr{k_zero};
  Float64 sum_gg{k_zero};
  Float64 sum_bb{k_zero};
  Float64 sum_rg{k_zero};
  Float64 sum_rb{k_zero};
  Float64 sum_gb{k_zero};

  for (Size i{0}; i < 16 * C; i += C) {
    sum_r += _uncompressed[i+0];
    sum_g += _uncompressed[i+1];
    sum_b += _uncompressed[i+2];
    sum_rr += _uncompressed[i+0] * _uncompressed[i+0];
    sum_gg += _uncompressed[i+1] * _uncompressed[i+1];
    sum_bb += _uncompressed[i+2] * _uncompressed[i+2];
    sum_rg += _uncompressed[i+0] * _uncompressed[i+1];
    sum_rb += _uncompressed[i+0] * _uncompressed[i+2];
    sum_gb += _uncompressed[i+1] * _uncompressed[i+2];
  }

  // average all sums
  sum_r *= k_inv16;
  sum_g *= k_inv16;
  sum_b *= k_inv16;

  // convert squares to squares of the value minus their average
  sum_rr -= k_sixteen * sum_r * sum_r;
  sum_gg -= k_sixteen * sum_g * sum_g;
  sum_bb -= k_sixteen * sum_b * sum_b;
  sum_rg -= k_sixteen * sum_r * sum_g;
  sum_rb -= k_sixteen * sum_r * sum_b;
  sum_gb -= k_sixteen * sum_g * sum_b;

  // the point on the color line is the average
  point_[0] = sum_r;
  point_[1] = sum_g;
  point_[2] = sum_b;

  // RYGDXT covariance matrix
  direction_[0] = 1.0;
  direction_[1] = 2.718281828;
  direction_[2] = 3.141592654;

  for (Size i{0}; i < k_refine_iterations; i++) {
    sum_r = direction_[0];
    sum_g = direction_[1];
    sum_b = direction_[2];

    direction_[0] = Float32(sum_r * sum_rr + sum_g * sum_rg + sum_b * sum_rb);
    direction_[1] = Float32(sum_r * sum_rg + sum_g * sum_gg + sum_b * sum_gb);
    direction_[2] = Float32(sum_r * sum_rb + sum_g * sum_gb + sum_b * sum_bb);
  }
}

template<Size C>
static inline void lse_master_colors_clamp(Uint16 (&colors_)[2],
  const Byte *const _uncompressed)
{
  Float32 sumx1[]{0.0f, 0.0f, 0.0f };
  Float32 sumx2[]{0.0f, 0.0f, 0.0f };
  compute_color_line<C>(_uncompressed, sumx1, sumx2);

  Float32 length{1.0f / (0.00001f + sumx2[0] * sumx2[0] + sumx2[1] * sumx2[1] + sumx2[2] * sumx2[2])};

  // calculate range for vector values
  Float32 dot_max{sumx2[0] * _uncompressed[0] +
                 sumx2[1] * _uncompressed[1] +
                 sumx2[2] * _uncompressed[2]};

  Float32 dot_min{dot_max};
  for (Size i{1}; i < 16; ++i) {
    const Float32 dot{sumx2[0] * _uncompressed[i * C + 0] +
                     sumx2[1] * _uncompressed[i*C+1] +
                     sumx2[2] * _uncompressed[i*C+2]};

    if (dot < dot_min) {
      dot_min = dot;
    } else if (dot > dot_max) {
      dot_max = dot;
    }
  }

  // calculate offset from the average location
  const Float32 dot{sumx2[0] * sumx1[0] + sumx2[1] * sumx1[1] + sumx2[2] * sumx1[2]};
  dot_min -= dot;
  dot_max -= dot;
  dot_min *= length;
  dot_max *= length;

  // build the master colors
  Uint16 c0[3];
  Uint16 c1[3];
  for (Size i{0}; i < 3; i++) {
    c0[i] = Algorithm::clamp(Uint16(0.5f + sumx1[i] + dot_max * sumx2[i]), 0_u16, 255_u16);
    c1[i] = Algorithm::clamp(Uint16(0.5f + sumx1[i] + dot_min * sumx2[i]), 0_u16, 255_u16);
  }

  // down sample the master colors to RGB565
  const Uint16 i{pack_565(c0[0], c0[1], c0[2])};
  const Uint16 j{pack_565(c1[0], c1[1], c1[2])};
  if (i > j) {
    colors_[0] = i;
    colors_[1] = j;
  } else {
    colors_[1] = i;
    colors_[0] = j;
  }
}

template<Size C>
static inline void compress_color_block(const Byte *const _uncompressed,
  Byte (&compressed_)[8])
{
  Uint16 encode_color[2];
  lse_master_colors_clamp<C>(encode_color, _uncompressed);

  // store 565 color
  compressed_[0] = encode_color[0] & 255;
  compressed_[1] = (encode_color[0] >> 8) & 255;
  compressed_[2] = encode_color[1] & 255;
  compressed_[3] = (encode_color[1] >> 8) & 255;
  for (Size i{4}; i < 8; i++) {
    compressed_[i] = 0;
  }

  // reconstitute master color vectors
  Uint16 c0[3];
  Uint16 c1[3];
  unpack_565(encode_color[0], c0[0], c0[1], c0[2]);
  unpack_565(encode_color[1], c1[0], c1[1], c1[2]);

  Float32 color_line[]{0.0f, 0.0f, 0.0f, 0.0f};
  Float32 length{0.0f};
  for (Size i{0}; i < 3; i++) {
    color_line[i] = float(c1[i] - c0[i]);
    length += color_line[i] * color_line[i];
  }

  if (length > 0.0f) {
    length = 1.0f / length;
  }

  // scaling
  for (Size i{0}; i < 3; i++) {
    color_line[i] *= length;
  }

  // offset portion of dot product
  const Float32 dot_offset{color_line[0] * c0[0] + color_line[1] * c0[1] + color_line[2] * c0[2]};
  // store rest of bits
  Size next_bit{8 * 4};
  for (Size i{0}; i < 16; i++) {
    // find the dot product for this color, to place it on the line with
    // a range of [-1, 1]
    const Float32 dot_product{color_line[0] * _uncompressed[i * C + 0] +
                             color_line[1] * _uncompressed[i*C+1] +
                             color_line[2] * _uncompressed[i*C+2] - dot_offset};
    // map to [0, 3]
    const int next_value{Algorithm::clamp(int(dot_product * 3.0f + 0.5f), 0, 3)};
    compressed_[next_bit >> 3] |= "\x0\x2\x3\x1"[next_value] << (next_bit & 7);
    next_bit += 2;
  }
}

static inline void compress_alpha_block(const Byte *const _uncompressed,
  Byte (&compressed_)[8])
{
  Byte a0{_uncompressed[3]};
  Byte a1{_uncompressed[3]};

  for (Size i{4 + 3}; i < 16 * 4; i += 4) {
    if (_uncompressed[i] > a0) {
      a0 = _uncompressed[i];
    }
    if (_uncompressed[i] < a1) {
      a1 = _uncompressed[i];
    }
  }

  compressed_[0] = a0;
  compressed_[1] = a1;

  for (Size i{2}; i < 8; i++) {
    compressed_[i] = 0;
  }

  Size next_bit{8 * 2};
  const Float32 scale{7.9999f / (a0 - a1)};

  for (Size i{3}; i < 16 * 4; i += 4) {
    const auto value{"\x1\x7\x6\x5\x4\x3\x2\x0"[Size((_uncompressed[i] - a1) * scale) & 7]};
    compressed_[next_bit >> 3] |= value << (next_bit & 7);

    // spans two bytes
    if ((next_bit & 7) > 5) {
      compressed_[1 + (next_bit >> 3)] |= value >> (8 - (next_bit & 7));
    }

    next_bit += 3;
  }
}

template<DXTType T>
Optional<LinearBuffer> dxt_compress(Memory::Allocator& _allocator,
                              const Byte *const _uncompressed, Size _width, Size _height,
                              Size _channels, Size& out_size_, Size& optimized_blocks_)
{
  Size index{0};
  const Size chan_step{_channels < 3 ? 0_z : 1_z};
  const Size has_alpha{1 - (_channels & 1)};

  out_size_ = ((_width + 3) >> 2) * ((_height + 3) >> 2) * (T == DXTType::k_dxt1 ? 8 : 16);

  LinearBuffer compressed{_allocator};
  if (!compressed.resize(out_size_)) {
    return nullopt;
  }

  Byte ublock[16 * (T == DXTType::k_dxt1 ? 3 : 4)];
  Byte cblock[8];

  for (Size j{0}; j < _height; j += 4) {
    for (Size i{0}; i < _width; i += 4) {
      Size z{0};

      const Size my{j + 4 >= _height ? _height - j : 4};
      const Size mx{i + 4 >= _width ? _width - i : 4};

      for (Size y{0}; y < my; y++) {
        for (Size x{0}; x < mx; x++) {
          for (Size p{0}; p < 3; p++) {
            ublock[z++] = _uncompressed[((((j+y)*_width)*_channels)+((i+x)*_channels))+(chan_step * p)];
          }
          if constexpr (T == DXTType::k_dxt5) {
            ublock[z++] = has_alpha * _uncompressed[(j+y)*_width*_channels+(i+x)*_channels+_channels-1] + (1 - has_alpha) * 255;
          }
        }

        for (Size x{mx}; x < 4; x++) {
          for (Size p{0}; p < (T == DXTType::k_dxt1 ? 3 : 4); p++) {
            ublock[z++] = ublock[p];
          }
        }
      }

      for (Size y{my}; y < 4; y++) {
        for (Size x{0}; x < 4; x++) {
          for (Size p{0}; p < (T == DXTType::k_dxt1 ? 3 : 4); p++) {
            ublock[z++] = ublock[p];
          }
        }
      }

      if constexpr (T == DXTType::k_dxt5) {
        compress_alpha_block(ublock, cblock);
        for (Size x{0}; x < 8; x++) {
          compressed[index++] = cblock[x];
        }
      }

      compress_color_block<(T == DXTType::k_dxt1 ? 3 : 4)>(ublock, cblock);
      for (Size x{0}; x < 8; x++) {
        compressed[index++] = cblock[x];
      }
    }
  }

  optimized_blocks_ = optimize<T>(compressed.data(), _width, _height);

  return compressed;
}

template Optional<LinearBuffer> dxt_compress<DXTType::k_dxt1>(
  Memory::Allocator& _allocator, const Byte *const _uncompressed,
  Size _width, Size _height, Size _channels, Size& out_size_,
  Size& optimized_blocks_);

template Optional<LinearBuffer> dxt_compress<DXTType::k_dxt5>(
  Memory::Allocator& _allocator, const Byte *const _uncompressed,
  Size _width, Size _height, Size _channels, Size& out_size_,
  Size& optimized_blocks_);

} // namespace Rx::Texture
