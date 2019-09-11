#include "rx/texture/scale.h"
#include "rx/core/algorithm/clamp.h"

namespace rx::texture {

template<typename T>
static inline bool is_pot(T _value)  {
  return (_value & (_value - 1)) == 0;
}

template<rx_size C>
void halve(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_)
{
  using T = rx_size;
  for (const rx_byte* y_end{_src + _sh * _stride}; _src < y_end; ) {
    for (const rx_byte* x_end{_src + _sw * C}, *x_src{_src}; x_src < x_end;
      x_src += 2 * C, dst_ += C)
    {
      for (rx_size i{0}; i < C; i++) {
        dst_[i] = (T{x_src[i]} + T{x_src[i+C]} + T{x_src[_stride+i]} +
          T{x_src[_stride+i+C]}) >> 2;
      }
    }
    _src += 2 * _stride;
  }
}

template<rx_size C>
void shift(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh)
{
  const rx_size w_frac{_sw / _dw};
  const rx_size h_frac{_sh / _dh};

  rx_size w_shift{0};
  rx_size h_shift{0};

  while (_dw << w_shift < _sw) {
    w_shift++;
  }

  while (_dh << h_shift < _sh) {
    h_shift++;
  }

  const rx_size t_shift{w_shift + h_shift};

  for (const rx_byte* y_end{_src + _sh * _stride}; _src < y_end; ) {
    for (const rx_byte* x_end{_src + _sw * C}, *x_src{_src}; x_src < x_end;
      x_src += w_frac * C, dst_ += C)
    {
      rx_size r[C]{0};
      for (const rx_byte* y_cur{x_src}, *x_end{y_cur + w_frac * C},
        *y_end{_src + h_frac * _stride}; y_cur < y_end; y_cur += _stride,
          x_end += _stride)
        {
        for (const rx_byte* x_cur{y_cur}; x_cur < x_end; x_cur += C) {
          for (rx_size i{0}; i < C; i++) {
            r[i] += x_cur[i];
          }
        }
      }
      for (rx_size i{0}; i < C; i++) {
        dst_[i] = static_cast<rx_byte>(r[i] >> t_shift);
      }
    }
    _src += h_frac * _stride;
  }
}

template<rx_size C>
void scale(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh)
{
  const rx_size w_frac{(_sw << 12_z) / _dw};
  const rx_size h_frac{(_sh << 12_z) / _dh};

  const rx_size d_area{_dw * _dh};
  const rx_size s_area{_sw * _sh};

  rx_s32 area_oflow{0};
  rx_s32 area_uflow{0};
  for (; (d_area >> area_oflow) > s_area; area_oflow++);
  for (; (d_area << area_uflow) < s_area; area_uflow++);

  const auto c_scale{static_cast<rx_size>(algorithm::clamp(area_uflow, area_oflow - 12, 12))};
  const auto a_scale{static_cast<rx_size>(algorithm::clamp(12 + area_uflow - area_oflow, 0, 24))};

  const rx_size d_scale{a_scale + 12 - c_scale};
  const rx_size area{(static_cast<rx_u64>(d_area) << a_scale) / s_area};

  _dw *= w_frac;
  _dh *= h_frac;

  for (rx_size y{0}; y < _dh; y += h_frac) {
    const rx_size yn{y + h_frac - 1};
    const rx_size yi{y >> 12_z};
    const rx_size h{(yn >> 12_z) - yi};
    const rx_size yl{((yn | (-rx_s32(h) >> 24)) & 0xfff_z) + 1 - (y & 0xfff_z)};
    const rx_size yh{(yn & 0xfff_z) + 1};
    const rx_byte* y_src{_src + yi * _stride};

    for (rx_size x{0}; x < _dw; x += w_frac, dst_ += C) {
      const rx_size xn{x + w_frac - 1};
      const rx_size xi{x >> 12_z};
      const rx_size w{(xn >> 12_z) - xi};
      const rx_size xl{((w + 0xfff_z) & 0x1000_z) - (x & 0xfff_z)};
      const rx_size xh{(xn & 0xfff_z) + 1};
      const rx_byte* x_src{y_src + xi * C};
      const rx_byte* x_end{x_src + w * C};

      rx_size r[C]{0};
      for (const rx_byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
        for (rx_size i{0}; i < C; i++) {
          r[i] += x_cur[i];
        }
      }

      for (rx_size i{0}; i < C; i++) {
        r[i] = (yl * (r[i] + ((x_src[i] * xl + x_end[i] * xh) >> 12_z))) >> c_scale;
      }

      if (h) {
        x_src += _stride;
        x_end += _stride;
      
        for (rx_size h_cur{h}; --h_cur; x_src += _stride, x_end += _stride) {
          rx_size p[C]{0};
        
          for (const rx_byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
            for (rx_size i{0}; i < C; i++) {
              p[i] += x_cur[i];
            }
          }

          for (rx_size i{0}; i < C; i++) {
            r[i] += ((p[i] << 12_z) + x_src[i] * xl + x_end[i] * xh) >> c_scale;
          }
        }

        rx_size p[C]{0};
        for (const rx_byte* x_cur{x_src + C}; x_cur < x_end; x_cur += C) {
          for (rx_size i{0}; i < C; i++) {
            p[i] += x_cur[i];
          }
        }

        for (rx_size i{0}; i < C; i++) {
          r[i] += (yh * (p[i] + ((x_src[i] * xl + x_end[i] * xh) >> 12_z))) >> c_scale;
        }
      }

      for (rx_size i{0}; i < C; i++) {
        dst_[i] = static_cast<rx_byte>((r[i] * area) >> d_scale);
      }
    }
  }
}

// explicitly instantiate for 1, 2, 3 and 4 channel varaitons
template void halve<1>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_);
template void halve<2>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_);
template void halve<3>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_);
template void halve<4>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_);

template void shift<1>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh);
template void shift<2>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh);
template void shift<3>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh);
template void shift<4>(const rx_byte* _src, rx_size _sw, rx_size _sh,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh);

template void scale<1>(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);
template void scale<2>(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);
template void scale<3>(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);
template void scale<4>(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);

void scale(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _bpp,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh)
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