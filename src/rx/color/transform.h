#ifndef RX_COLOR_TRANSFORM_H
#define RX_COLOR_TRANSFORM_H

namespace Rx::Color {

struct CMYK;
struct HSL;
struct HSV;
struct RGB;

// Several color space transformations.
void rgb_to_hsv(const RGB& _rgb, HSV& hsv_);
void hsv_to_rgb(const HSV& _hsv, RGB& rgb_);
void rgb_to_hsl(const RGB& _rgb, HSL& hsl_);
void hsl_to_rgb(const HSL& _hsl, RGB& rgb_);
void rgb_to_cmyk(const RGB& _rgb, CMYK& cmyk_);
void cmyk_to_rgb(const CMYK& _cmyk, RGB& rgb_);

};

#endif // RX_COLOR_TRANSFORM_H
