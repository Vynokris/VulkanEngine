#pragma once

namespace Maths
{
    class RGB;
    class RGBA;
    class HSV;
    class HSVA;

    // - RGBA: red, green, blue and alpha between 0 and 1 - //
    class RGBA
    {
    public:
        // -- Attributes -- //
        float r, g, b, a; // Red, green, blue and alpha components.

        // -- Constructors -- //
        RGBA();                                                                   // Default constructor (r = g = b = 0, a = 1).
        RGBA(const float& all);                                                   // RGBA with equal values.
        RGBA(const float& _r, const float& _g, const float& _b, const float& _a); // RGBA color with 4 values.
        RGBA(const RGB& rgb, const float& _a = 1);                                // RGBA color from RGB.
        RGBA(const HSVA& hsva);                                                   // RGBA color from HSVA.

        // -- Methods -- //

        float GetHue() const;                        // Returns this RGBA color's hue.
        void  ShiftHue     (const float& hue);       // Shifts the hue of this RGBA color.
        RGBA  GetShiftedHue(const float& hue) const; // Returns a copy of this RGBA color with its hue shifted.
        static RGBA Lerp(const RGBA& start, const RGBA& end, const float val); // Computes linear interpolation between two given colors.

        // -- Miscellaneous -- //
        RGB ToRGB() const; // Convert RGBA color to RGB.
        float* AsPtr();      // Returns a pointer to the r component.

        // -- Operators -- //
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
        bool operator==(const RGBA& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
        bool operator!=(const RGBA& c) const { return r != c.r || g != c.g || b != c.b || a != c.a; }
    };

    // - RGB: red, green and blue between 0 and 1 - //
    class RGB 
    { 
    public:
        // -- Attributes -- //
        float r, g, b;  // Red, green and blue components.

        // -- Constructors -- //
        RGB();                                                  // Default constructor (r = g = b = 0).
        RGB(const float& all);                                  // RGB with equal values.
        RGB(const float& _r, const float& _g, const float& _b); // RGB color with 3 values.
        RGB(const RGBA& rgba);                                  // RGB color from RGBA.

        // -- Miscellaneous -- //
        RGBA ToRGBA(const float& a = 1) const; // Convert RGB color to RGBA.
        float* AsPtr();                          // Returns a pointer to the r component.
    };

    // - HSVA: hue, saturtion, value and alpha (h in radians, sva between 0 and 1) - //
    class HSVA
    {
    public:
        // -- Attributes -- //
        float h, s, v, a = 1; // Hue, saturtion, value and alpha components.

        // -- Constructors -- //
        HSVA();                                                                   // Default constructor (h = s = v = 0, a = 1).
        HSVA(const float& all);                                                   // HSVA color with equal values.
        HSVA(const float& _h, const float& _s, const float& _v, const float& _a); // HSVA color with 4 values.
        HSVA(const HSV& hsv, const float& _a = 1);                                // HSVA color from HSV.
        HSVA(const RGBA& rgba);                                                   // HSVA color from RGBA.

        // -- Miscellaneous -- //
        HSV ToHSV() const; // Convert HSVA color to HSV.
        float* AsPtr();      // Returns a pointer to the h component.
    };
    
    // - HSV: hue, saturation and value (h in radians, sv between 0 and 1) - //
    class HSV
    {
    public:
        // -- Attributes -- //
        float h, s, v; // Hue, saturtion and value components.

        // -- Constructors -- //
        HSV();                                                  // Default constructor (h = s = v = 0).
        HSV(const float& all);                                  // HSV color with equal values.
        HSV(const float& _h, const float& _s, const float& _v); // HSV color with 3 values.
        HSV(const HSVA& hsva);                                  // HSV color from HSVA.

        // -- Miscellaneous -- //
        HSVA ToHSVA(const float& a = 1) const; // Convert HSV color to HSVA.
        float* AsPtr();                          // Returns a pointer to the h component.
    };
}
