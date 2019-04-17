#include <rx/math/half.h> // half

#include <rx/core/statics.h> // static_global

namespace rx::math {

union bits {
  constexpr bits(rx_u32 u);
  constexpr bits(rx_f32 f);

  rx_u32 u;
  rx_f32 f;
};

inline constexpr bits::bits(rx_u32 u)
  : u{u}
{
}

inline constexpr bits::bits(rx_f32 f)
  : f{f}
{
}

static constexpr const rx_u32 k_magic{113 << 23};
static constexpr const rx_u32 k_shift_exp{0x7C00 << 13}; // exp mask after shift
static constexpr bits k_magic_bits{k_magic};

struct half_lut {
  half_lut() {
    for (int i{0}, e{0}; i < 256; i++) {
      e = i - 127;
      if (e < -24) {
        base[i|0x000] = 0x0000;
        base[i|0x100] = 0x8000;
        shift[i|0x000] = 24;
        shift[i|0x100] = 24;
      } else if (e < -14) {
        base[i|0x000] = 0x0400 >> (-e-14);
        base[i|0x100] = (0x0400 >> (-e-14)) | 0x8000;
        shift[i|0x000] = -e-1;
        shift[i|0x100] = -e-1;
      } else if (e <= 15) {
        base[i|0x000] = (e+15) << 10;
        base[i|0x100] = ((e+15) << 10) | 0x8000;
        shift[i|0x000] = 13;
        shift[i|0x100] = 13;
      } else if (e < 128) {
        base[i|0x000] = 0x7C00;
        base[i|0x100] = 0xFC00;
        shift[i|0x000] = 24;
        shift[i|0x100] = 24;
      } else {
        base[i|0x000] = 0x7C00;
        base[i|0x100] = 0xFC00;
        shift[i|0x000] = 13;
        shift[i|0x100] = 13;
      }
    }
  }

  rx_u32 base[512];
  rx_u8 shift[512];
};

static const static_global<half_lut> g_table("half_lut");

half half::to_half(rx_f32 f) {
  const bits shape{f};
  return half(static_cast<rx_u16>(g_table->base[(shape.u >> 23) & 0x1FF] +
    ((shape.u & 0x007FFFFF) >> g_table->shift[(shape.u >> 23) & 0x1FF])));
}

rx_f32 half::to_f32() const {
  bits out{static_cast<rx_u32>((m_bits & 0x7FFF) << 13)}; // exp/mantissa
  const auto exp{k_shift_exp & out.u}; // exp
  out.u += (127 - 15) << 23; // adjust exp
  if (exp == k_shift_exp) {
    out.u += (128 - 16) << 23; // adjust for inf/nan
  } else if (exp == 0) {
    out.u += 1 << 23; // adjust for zero/denorm
    out.f -= k_magic_bits.f; // renormalize
  }
  out.u |= (m_bits & 0x8000) << 16; // sign bit
  return out.f;
}

} // namespace rx::math
