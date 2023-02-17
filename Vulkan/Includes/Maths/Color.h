#pragma once

namespace Maths
{
    class RGB;
    class HSV;

    // RGBA color values between 0 and 1.
    class RGBA
    {
    public:
        float r, g, b, a = 1;

        RGBA();
        RGBA(const float& all);
        RGBA(const float& _r, const float& _g, const float& _b, const float& _a);
        RGBA(const RGB& rgb, const float& _a = 1);
        RGB toRGB() const;
        float* ptr();

        RGBA operator+ (const RGBA& c)  const { return { r + c.r,    g + c.g ,   b + c.b,    a + c.a }; }
        RGBA operator+ (const float& v) const { return { r + v  ,    g + v   ,   b + v  ,    a + v   }; }
        RGBA operator- (const RGBA& c)  const { return { r - c.r,    g - c.g ,   b - c.b,    a - c.a }; }
        RGBA operator- (const float& v) const { return { r - v  ,    g - v   ,   b - v  ,    a - v   }; }
        RGBA operator* (const RGBA& c)  const { return { r * c.r,    g * c.g ,   b * c.b,    a * c.a }; }
        RGBA operator* (const float& v) const { return { r * v  ,    g * v   ,   b * v  ,    a * v   }; }
        RGBA operator/ (const RGBA& c)  const { return { r / c.r,    g / c.g ,   b / c.b,    a / c.a }; }
        RGBA operator/ (const float& v) const { return { r / v  ,    g / v   ,   b / v  ,    a / v   }; }
        void operator+=(const RGBA& c)  { r += c.r;   g += c.g;   b += c.b;   a += c.a; }
        void operator+=(const float& v) { r += v;     g += v;     b += v;     a += v;   }
        void operator-=(const RGBA& c)  { r -= c.r;   g -= c.g;   b -= c.b;   a -= c.a; }
        void operator-=(const float& v) { r -= v;     g -= v;     b -= v;     a -= v;   }
        void operator*=(const RGBA& c)  { r *= c.r;   g *= c.g;   b *= c.b;   a *= c.a; }
        void operator*=(const float& v) { r *= v;     g *= v;     b *= v;     a *= v;   }
        void operator/=(const RGBA& c)  { r /= c.r;   g /= c.g;   b /= c.b;   a /= c.a; }
        void operator/=(const float& v) { r /= v;     g /= v;     b /= v;     a /= v;   }
        bool operator==(const RGBA& c) { return   r == c.r && g == c.g && b == c.b && a == c.a; }
        bool operator!=(const RGBA& c) { return   r != c.r || g != c.g || b != c.b || a != c.a; }
    };

    // RGB color values between 0 and 1.
    class RGB 
    { 
    public:
        float r, g, b; 

        RGB();
        RGB(const float& all);
        RGB(const float& _r, const float& _g, const float& _b);
        RGB(const RGBA& rgba);
        RGBA toRGBA() const;
        float* ptr();
    };
    
    // HSVA color values (h in rad, sva between 0 and 1).
    class HSVA
    {
    public:
        float h, s, v, a;

        HSVA();
        HSVA(const float& all);
        HSVA(const float& _h, const float& _s, const float& _v, const float& _a);
        HSVA(const HSV& hsv, const float& _a = 1);
        HSV toHSV() const;
        float* ptr();
    };
    
    // HSV color values (h in rad, sv between 0 and 1).
    class HSV
    {
    public:
        float h, s, v;

        HSV();
        HSV(const float& all);
        HSV(const float& _h, const float& _s, const float& _v);
        HSV(const HSVA& hsva);
        HSVA toHSVA() const;
        float* ptr();
    };


    // -- Color Methods -- //
    
    float ColorGetHue(const RGBA& rgb);                                      // Returns the given RGB color's hue.
    RGBA  ColorLerp  (const float& val, const RGBA& start, const RGBA& end); // Linear interpolation between two given colors.
    HSV   BlendHSV   (const HSV& col0, const HSV& col1);                     // Blend between two HSV colors.
    HSVA  RGBAtoHSVA (const RGBA& color);                                    // Convert an RGBA color (0 <= rgba <= 1) to HSVA.
    RGBA  HSVAtoRGBA (const HSVA& hsva);                                     // Convert an HSVA color to RGBA.
    RGBA  ColorShift (const RGBA& color, const float& hue);                  // Shifts the hue of the given color.
}
