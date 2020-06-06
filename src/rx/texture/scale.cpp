#include "rx/texture/scale.h"
#include "rx/core/algorithm/clamp.h"

namespace Rx::Texture {

template<typename T>
static inline bool is_pot(T _value)  {
  return (_value & (_value - 1)) == 0;
}

template<Size C>
void halve(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_)
{
  using T = Size;
  for (const Byte* y_end{_src + _sh * _stride}; _src < y_end; ) {
    for (const Byte* x_end{_src + _sw * C}, *x_src{_src}; x_src < x_end;
      x_src += 2 * C, dst_ += C)
    {
      for (Size i{0}; i < C; i++) {
        dst_[i] = (T{x_src[i]} + T{x_src[i+C]} + T{x_src[_stride+i]} +
          T{x_src[_stride+i+C]}) >> 2;
      }
    }
    _src += 2 * _stride;
  }
}

template<Size C>
void shift(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw, Size _dh)
{
  const Size w_frac{_sw / _dw};
  const Size h_frac{_sh / _dh};

  Size w_shift{0};
  Size h_shift{0};

  while (_dw << w_shift < _sw) {
    w_shift++;
  }

  while (_dh << h_shift < _sh) {
    h_shift++;
  }

  const Size t_shift{w_shift + h_shift};

  for (const Byte* y_end{_src + _sh * _stride}; _src < y_end; ) {
    for (const Byte* x_end{_src + _sw * C}, *x_src{_src}; x_src < x_end;
      x_src += w_frac * C, dst_ += C)
    {
      Size r[C]{0};
      for (const Byte* y_cur{x_src}, *x_end{y_cur + w_frac * C},
        *y_end{_src + h_frac * _stride}; y_cur < y_end; y_cur += _stride,
          x_end += _stride)
        {
        for (const Byte* x_cur{y_cur}; x_cur < x_end; x_cur += C) {
          for (Size i{0}; i < C; i++) {
            r[i] += x_cur[i];
          }
        }
      }
      for (Size i{0}; i < C; i++) {
        dst_[i] = static_cast<Byte>(r[i] >> t_shift);
      }
    }
    _src += h_frac * _stride;
  }
}

template<Size C>
void scale(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw, Size _dh)
{
  const Size w_frac{(_sw << 12_z) / _dw};
  const Size h_frac{(_sh << 12_z) / _dh};

  const Size d_area{_dw * _dh};
  const Size s_area{_sw * _sh};

  Sint32 area_oflow{0};
  Sint32 area_uflow{0};
  for (; (d_area >> area_oflow) > s_area; area_oflow++);
  for (; (d_area << area_uflow) < s_area; area_uflow++);

  const auto c_scale{static_cast<Size>(Algorithm::clamp(area_uflow, area_oflow - 12, 12))};
  const auto a_scale{static_cast<Size>(Algorithm::clamp(12 + area_uflow - area_oflow, 0, 24))};

  const auto d_scale{a_scale + 12 - c_scale};
  const auto area{(static_cast<Uint64>(d_area) << a_scale) / s_area};

  _dw *= w_frac;
  _dh *= h_frac;

  for (Size y{0}; y < _dh; y += h_frac) {
    const Size yn{y + h_frac - 1};
    const Size yi{y >> 12_z};
    const Size h{(yn >> 12_z) - yi};
    const Size yl{((yn | (-Sint32(h) >> 24)) & 0xfff_z) + 1 - (y & 0xfff_z)};
    const Size yh{(yn & 0xfff_z) + 1};
    const Byte* y_src{_src + yi * _stride};

    for (Size x{0}; x < _dw; x += w_frac, dst_ += C) {
      const Size xn{x + w_frac - 1};
      const Size xi{x >> 12_z};
      const Size w{(xn >> 12_z) - xi};
      const Size xl{((w + 0xfff_z) & 0x1000_z) - (x & 0xfff_z)};
      const Size xh{(xn & 0xfff_z) + 1};
      const Byte* x_src{y_src + xi * C};
      const Byte* x_end{x_src + w * C};

      Size r[C]{0};
      for (const Byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
        for (Size i{0}; i < C; i++) {
          r[i] += x_cur[i];
        }
      }

      for (Size i{0}; i < C; i++) {
        r[i] = (yl * (r[i] + ((x_src[i] * xl + x_end[i] * xh) >> 12_z))) >> c_scale;
      }

      if (h) {
        x_src += _stride;
        x_end += _stride;

        for (Size h_cur{h}; --h_cur; x_src += _stride, x_end += _stride) {
          Size p[C]{0};

          for (const Byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
            for (Size i{0}; i < C; i++) {
              p[i] += x_cur[i];
            }
          }

          for (Size i{0}; i < C; i++) {
            r[i] += ((p[i] << 12_z) + x_src[i] * xl + x_end[i] * xh) >> c_scale;
          }
        }

        Size p[C]{0};
        for (const Byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
          for (Size i{0}; i < C; i++) {
            p[i] += x_cur[i];
          }
        }

        for (Size i{0}; i < C; i++) {
          r[i] += (yh * (p[i] + ((x_src[i] * xl + x_end[i] * xh) >> 12_z))) >> c_scale;
        }
      }

      for (Size i{0}; i < C; i++) {
        dst_[i] = static_cast<Byte>((r[i] * area) >> d_scale);
      }
    }
  }
}

// explicitly instantiate for 1, 2, 3 and 4 channel varaitons
template void halve<1>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_);
template void halve<2>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_);
template void halve<3>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_);
template void halve<4>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_);

template void shift<1>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void shift<2>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void shift<3>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void shift<4>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);

template void scale<1>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void scale<2>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void scale<3>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);
template void scale<4>(const Byte *RX_HINT_RESTRICT _src, Size _sw,
  Size _sh, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);

void scale(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _bpp, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh)
{
  if (_sw == _dw*2 && _sh == _dh*2) {
    switch (_bpp) {
    case 1:
      return halve<1>(_src, _sw, _sh, _stride, dst_);
    case 2:
      return halve<2>(_src, _sw, _sh, _stride, dst_);
    case 3:
      return halve<3>(_src, _sw, _sh, _stride, dst_);
    case 4:
      return halve<4>(_src, _sw, _sh, _stride, dst_);
    }
  } else if (_sw < _dw || _sh < _dh || !is_pot(_sw) || !is_pot(_sh) || !is_pot(_dw) || !is_pot(_dh)) {
    switch (_bpp) {
    case 1:
      return scale<1>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 2:
      return scale<2>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 3:
      return scale<3>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 4:
      return scale<4>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    }
  } else {
    switch (_bpp) {
    case 1:
      return shift<1>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 2:
      return shift<2>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 3:
      return shift<3>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    case 4:
      return shift<4>(_src, _sw, _sh, _stride, dst_, _dw, _dh);
    }
  }
}

} // namespace rx::texture
