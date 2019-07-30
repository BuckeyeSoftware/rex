#include "rx/core/math/sqrt.h"

#include "rx/math/quat.h"
#include "rx/math/mat3x3.h"
#include "rx/math/mat3x4.h"

namespace rx::math {

template<typename T, template<class> typename M>
static quat<T> matrix_to_quat(const M<T>& _mat) {
  const rx_f32 trace{_mat.x.x + _mat.y.y + _mat.z.z};

  if (trace > 0.0f) {
    const rx_f32 r{sqrt(1.0f + trace)};
    const rx_f32 i{0.5f/r};

    return {
      (_mat.z.y - _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i,
      (_mat.y.x - _mat.x.y) * i,
      0.5f * r
    };
  } else if (_mat.x.x > _mat.y.y && _mat.x.x > _mat.z.x) {
    const rx_f32 r{sqrt(1.0f + _mat.x.x - _mat.y.y - _mat.z.z)};
    const rx_f32 i{0.5f/r};

    return {
      0.5f * r,
      (_mat.y.x + _mat.x.y) * i,
      (_mat.x.z + _mat.z.x) * i,
      (_mat.z.y - _mat.y.z) * i
    };
  } else if (_mat.y.y > _mat.z.z) {
    const rx_f32 r{sqrt(1.0f + _mat.y.y - _mat.x.x - _mat.z.z)};
    const rx_f32 i{0.5f/r};

    return {
      (_mat.y.x + _mat.x.y) * i,
      0.5f * r,
      (_mat.z.y + _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i
    };
  }

  const rx_f32 r{sqrt(1.0f + _mat.z.z - _mat.x.x - _mat.y.y)};
  const rx_f32 i{0.5f/r};

  return {
    (_mat.x.z + _mat.z.x) * i,
    (_mat.z.y + _mat.y.z) * i,
    0.5f * r,
    (_mat.y.x - _mat.x.y) * i
  };
}

template<typename T>
quat<T>::quat(const mat3x3<T>& _mat)
  : quat{matrix_to_quat(_mat)}
{
}

template<typename T>
quat<T>::quat(const mat3x4<T>& _mat)
  : quat{matrix_to_quat(_mat)}
{
}

template quat<rx_f32>::quat(const mat3x3<rx_f32>& _mat);
template quat<rx_f32>::quat(const mat3x4<rx_f32>& _mat);

rx_f32 length(const quatf& _value) {
  return sqrt(dot(_value, _value));
}

quatf normalize(const quatf& _value) {
  return _value * (1.0f / math::length(_value));
}

} // namespace rx::math