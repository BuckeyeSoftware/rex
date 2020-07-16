#include "rx/image/normal_map.h"
#include "rx/math/mat3x3.h"

namespace Rx::Image {

// Standard Sobel filter using 3x3 convolution kernel.
static Math::Vec3f sobel(const Math::Mat3x3f _convolution, Float32 _inv_strength) {
  const Float32 t{_convolution[0][0] + 2.0f * _convolution[0][1] + _convolution[0][2]};
  const Float32 b{_convolution[2][0] + 2.0f * _convolution[2][1] + _convolution[2][2]};
  const Float32 r{_convolution[0][2] + 2.0f * _convolution[1][2] + _convolution[2][2]};
  const Float32 l{_convolution[0][0] + 2.0f * _convolution[1][0] + _convolution[2][0]};

  const Float32 dx{b - t};
  const Float32 dy{r - l};
  const Float32 dz{_inv_strength};

  return Math::normalize({dx, dy, dz});
}

// Customized Sobel filter without the 2.0f multiplication, i.e Prewitt.
static Math::Vec3f prewitt(const Math::Mat3x3f _convolution, Float32 _inv_strength) {
  const Float32 t{_convolution[0][0] + _convolution[0][1] + _convolution[0][2]};
  const Float32 b{_convolution[2][0] + _convolution[2][1] + _convolution[2][2]};
  const Float32 r{_convolution[0][2] + _convolution[1][2] + _convolution[2][2]};
  const Float32 l{_convolution[0][0] + _convolution[1][0] + _convolution[2][0]};

  const Float32 dx{t - b};
  const Float32 dy{r - l};
  const Float32 dz{_inv_strength};

  return Math::normalize({dx, dy, dz});
}

// "Softlight" blending as used in many raster photo editors like GIMP and
// Photoshop.
//
// The general algorithm here is outlined well here:
//  https://en.wikipedia.org/wiki/Blend_modes#Soft_Light
static int blend_softlight(int _color1, int _color2) {
  const auto a{static_cast<Float32>(_color1)};
  const auto b{static_cast<Float32>(_color2)};

  return static_cast<int>(b * 2.0f < 255.0f
    ? (((a + 127.5f) * b) / 255.0f)
    : (255.0f - (((382.5f - a) * (255.0f - b)) / 255.0f)));
}

Matrix NormalMap::generate(IntensityMap::Mode _mode,
                           const Math::Vec4f& _multiplier, Kernel _kernel, Float32 _strength,
                           int _flags, Float32 _detail) const
{
  Matrix result{m_image.allocator(), m_image.dimensions(), 3};

  const Math::Vec2i& dimensions{m_image.dimensions().cast<Sint32>()};

  auto edges = [&](Sint32 _index, Sint32 _max) {
    if (_index >= _max) {
      return (_flags & k_tile) ? _max - _index : _max - 1;
    } else if (_index < 0) {
      return (_flags & k_tile) ? _max + _index : 0;
    }
    return _index;
  };

  IntensityMap intensity{m_image, _mode, _multiplier};
  if (_flags & k_invert) {
    intensity.invert();
  }

  const Float32 inv_strength{1.0f / _strength};

  for (Sint32 y{0}; y < dimensions.h; y++) {
    for (Sint32 x{0}; x < dimensions.w; x++) {
      const Float32 tl{intensity(edges(x - 1, dimensions.w), edges(y - 1, dimensions.h))},
                   t {intensity(edges(x - 1, dimensions.w), edges(y,     dimensions.h))},
                   tr{intensity(edges(x - 1, dimensions.w), edges(y + 1, dimensions.h))},
                   r {intensity(edges(x,     dimensions.w), edges(y + 1, dimensions.h))},
                   br{intensity(edges(x + 1, dimensions.w), edges(y + 1, dimensions.h))},
                   b {intensity(edges(x + 1, dimensions.w), edges(y,     dimensions.h))},
                   bl{intensity(edges(x + 1, dimensions.w), edges(y - 1, dimensions.h))},
                   l {intensity(edges(x,     dimensions.w), edges(y - 1, dimensions.h))};

      const Math::Mat3x3f convolution{{tl, t, tr}, {l, 0, r}, {bl, b, br}};

      // NOTE(dweiler): Under gcc the compiler is able to hoist the switch outside
      // these for loops, check if it occurs in clang too.
      Math::Vec3f normal{0.0f, 0.0f, 0.0f};

      switch (_kernel) {
      case Kernel::k_sobel:
        normal = sobel(convolution, inv_strength);
        break;
      case Kernel::k_prewitt:
        normal = prewitt(convolution, inv_strength);
        break;
      }

      Float32* rgb{result(x, y)};

      // Transform -1..1 to 0..1
      rgb[0] = normal.r * 0.5 + 0.5f;
      rgb[1] = normal.g * 0.5 + 0.5f;
      rgb[2] = normal.b * 0.5 + 0.5f;
    }
  }

  if (_flags & k_detail) {
    // Create downscaled version of |m_image|

    // TODO(dweiler): proper detail scaling
    Matrix scaled = m_image.scaled(m_image.dimensions() / 4_z);

    // Run the calculation on the downscaled image, except don't run a detailing
    // pass for the scaled image.
    Matrix downscaled = NormalMap(scaled).generate(_mode, _multiplier,
                                                   _kernel, _detail, _flags & ~k_detail, 0.0f);

    // Scale it back up
    downscaled = downscaled.scaled(m_image.dimensions());

    // Mix using Softlight blending.
    for (Sint32 y{0}; y < dimensions.h; y++) {
      for (Sint32 x{0}; x < dimensions.w; x++) {
        const Float32* rgb_lhs{result(x, y)};
        const Float32* rgb_rhs{downscaled(x, y)};

        const Sint32 r{blend_softlight(static_cast<int>(rgb_lhs[0] * 255.0), static_cast<int>(rgb_rhs[0] * 255.0f))};
        const Sint32 g{blend_softlight(static_cast<int>(rgb_lhs[1] * 255.0), static_cast<int>(rgb_rhs[1] * 255.0f))};
        const Sint32 b{blend_softlight(static_cast<int>(rgb_lhs[2] * 255.0), static_cast<int>(rgb_rhs[2] * 255.0f))};

        Float32* dst{result(x, y)};

        dst[0] = static_cast<Float32>(r) / 255.0f;
        dst[1] = static_cast<Float32>(g) / 255.0f;
        dst[2] = static_cast<Float32>(b) / 255.0f;
      }
    }
  }

  return result;
}

} // namespace rx::image
