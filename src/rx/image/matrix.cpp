#include "rx/image/matrix.h"

namespace Rx::Image {

static constexpr const Float32 SRGB_8BIT_TO_LINEAR_FLOAT[] = {
	0.0f, 3.03527e-4f, 6.07054e-4f, 9.10581e-4f,
	0.001214108f, 0.001517635f, 0.001821162f, 0.0021246888f,
	0.002428216f, 0.002731743f, 0.00303527f, 0.0033465358f,
	0.0036765074f, 0.004024717f, 0.004391442f, 0.0047769537f,
	0.005181517f, 0.005605392f, 0.0060488335f, 0.006512091f,
	0.0069954107f, 0.007499032f, 0.008023193f, 0.008568126f,
	0.009134059f, 0.009721218f, 0.010329823f, 0.010960095f,
	0.011612245f, 0.012286489f, 0.0129830325f, 0.013702083f,
	0.014443845f, 0.015208516f, 0.015996294f, 0.016807377f,
	0.017641956f, 0.018500222f, 0.019382363f, 0.020288564f,
	0.021219011f, 0.022173885f, 0.023153368f, 0.024157634f,
	0.025186861f, 0.026241222f, 0.027320893f, 0.02842604f,
	0.029556835f, 0.030713445f, 0.031896032f, 0.033104766f,
	0.034339808f, 0.035601314f, 0.036889452f, 0.038204372f,
	0.039546236f, 0.0409152f, 0.04231141f, 0.04373503f,
	0.045186203f, 0.046665087f, 0.048171826f, 0.049706567f,
	0.051269464f, 0.05286065f, 0.05448028f, 0.056128494f,
	0.057805438f, 0.059511244f, 0.06124606f, 0.06301002f,
	0.06480327f, 0.066625945f, 0.068478175f, 0.0703601f,
	0.07227185f, 0.07421357f, 0.07618539f, 0.07818743f,
	0.08021983f, 0.082282715f, 0.084376216f, 0.086500466f,
	0.08865559f, 0.09084172f, 0.093058966f, 0.09530747f,
	0.097587354f, 0.09989873f, 0.10224174f, 0.10461649f,
	0.107023105f, 0.10946172f, 0.111932434f, 0.11443538f,
	0.11697067f, 0.119538434f, 0.122138776f, 0.12477182f,
	0.12743768f, 0.13013647f, 0.13286832f, 0.13563333f,
	0.13843162f, 0.14126329f, 0.14412847f, 0.14702727f,
	0.14995979f, 0.15292616f, 0.15592647f, 0.15896083f,
	0.16202939f, 0.1651322f, 0.1682694f, 0.17144111f,
	0.1746474f, 0.17788842f, 0.18116425f, 0.18447499f,
	0.18782078f, 0.19120169f, 0.19461784f, 0.19806932f,
	0.20155625f, 0.20507874f, 0.20863687f, 0.21223076f,
	0.21586053f, 0.21952623f, 0.22322798f, 0.2269659f,
	0.23074007f, 0.23455061f, 0.2383976f, 0.24228115f,
	0.24620135f, 0.2501583f, 0.25415212f, 0.25818288f,
	0.2622507f, 0.26635563f, 0.27049783f, 0.27467734f,
	0.2788943f, 0.28314877f, 0.28744087f, 0.29177067f,
	0.2961383f, 0.3005438f, 0.30498734f, 0.30946895f,
	0.31398875f, 0.3185468f, 0.32314324f, 0.32777813f,
	0.33245155f, 0.33716366f, 0.34191445f, 0.3467041f,
	0.35153264f, 0.35640016f, 0.36130682f, 0.36625263f,
	0.3712377f, 0.37626216f, 0.38132605f, 0.38642946f,
	0.3915725f, 0.39675525f, 0.4019778f, 0.40724024f,
	0.41254264f, 0.4178851f, 0.4232677f, 0.42869052f,
	0.43415368f, 0.4396572f, 0.44520122f, 0.45078582f,
	0.45641103f, 0.46207702f, 0.4677838f, 0.4735315f,
	0.4793202f, 0.48514995f, 0.4910209f, 0.496933f,
	0.5028865f, 0.50888133f, 0.5149177f, 0.5209956f,
	0.52711517f, 0.53327644f, 0.5394795f, 0.5457245f,
	0.55201143f, 0.55834043f, 0.5647115f, 0.57112485f,
	0.57758045f, 0.58407843f, 0.59061885f, 0.5972018f,
	0.60382736f, 0.61049557f, 0.6172066f, 0.62396044f,
	0.63075715f, 0.6375969f, 0.6444797f, 0.65140563f,
	0.65837485f, 0.66538733f, 0.67244315f, 0.6795425f,
	0.6866853f, 0.6938718f, 0.7011019f, 0.7083758f,
	0.71569353f, 0.7230551f, 0.73046076f, 0.73791045f,
	0.74540424f, 0.7529422f, 0.7605245f, 0.76815116f,
	0.7758222f, 0.7835378f, 0.791298f, 0.7991027f,
	0.8069523f, 0.8148466f, 0.82278574f, 0.8307699f,
	0.838799f, 0.8468732f, 0.8549926f, 0.8631572f,
	0.8713671f, 0.8796224f, 0.8879231f, 0.8962694f,
	0.9046612f, 0.91309863f, 0.92158186f, 0.9301109f,
	0.9386857f, 0.9473065f, 0.9559733f, 0.9646863f,
	0.9734453f, 0.9822506f, 0.9911021f, 1.0f,
};

Optional<Matrix> Matrix::create_from_bytes(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions, const Byte* _data) {
  auto matrix = create(_allocator, _dimensions);
  if (!matrix) {
    return nullopt;
  }

  // Populate the matrix accordingly.
  for (Size y = 0; y < _dimensions.h; y++) {
    for (Size x = 0; x < _dimensions.w; x++) {
      const Byte* pixel = _data + (_dimensions.w * y + x) * 4;
      Math::Vec4f rgba;
      rgba.r = SRGB_8BIT_TO_LINEAR_FLOAT[pixel[0]];
      rgba.g = SRGB_8BIT_TO_LINEAR_FLOAT[pixel[1]];
      rgba.b = SRGB_8BIT_TO_LINEAR_FLOAT[pixel[2]];
      rgba.a = Float32(pixel[3]) / 255.0f; // Alpha is never sRGB.
      (*matrix)(x, y) = rgba;
    }
  }

  return matrix;
}

Optional<Matrix> Matrix::create(Memory::Allocator& _allocator, const Math::Vec2z& _dimensions) {
  LinearBuffer buffer{_allocator};
  if (!buffer.resize(_dimensions.area() * sizeof(Math::Vec4f))) {
    return nullopt;
  }

  Matrix matrix;
  matrix.m_data = Utility::move(buffer);
  matrix.m_dimensions = _dimensions;

  return matrix;
}

/*
Matrix Matrix::scaled(const Math::Vec2z& _dimensions) const {
  RX_ASSERT(_dimensions != m_dimensions, "cannot scale to same size");

  const Size channels{m_channels};

  // Really slow Bilinear scaler.
  Matrix result{allocator(), _dimensions, channels};

  const Float32 x_ratio = Float32(m_dimensions.w - 1) / _dimensions.w;
  const Float32 y_ratio = Float32(m_dimensions.h - 1) / _dimensions.h;

  for (Size y{0}; y < _dimensions.h; y++) {
    for (Size x{0}; x < _dimensions.w; x++) {
      const auto x_index{static_cast<Sint32>(x_ratio * x)};
      const auto y_index{static_cast<Sint32>(y_ratio * y)};

      const Float32 x_diff{(x_ratio * x) - x_index};
      const Float32 y_diff{(y_ratio * y) - y_index};

      for (Size channel{0}; channel < channels; channel++) {
        const Size index{y_index * m_dimensions.w * channels + x_index * channels + channel};

        const Float32 a{data()[index]};
        const Float32 b{data()[index + channels]};
        const Float32 c{data()[index + m_dimensions.w * channels]};
        const Float32 d{data()[index + m_dimensions.w * channels + channels]};

        result[y * _dimensions.w * channels + x * channels + channel] =
           a * (1.0f - x_diff) * (1.0f - y_diff) +
           b * (       x_diff) * (1.0f - y_diff) +
           c * (       y_diff) * (1.0f - x_diff) +
           d * (       x_diff) * (       y_diff);
      }
    }
  }

  return result;
}*/

} // namespace Rx::Image
