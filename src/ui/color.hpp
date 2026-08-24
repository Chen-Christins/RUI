// color.hpp
#pragma once

#include <QColor>

#include <ostream>
#include <string>

namespace rui {

// RUI 颜色封装 —— 轻量值类型，可隐式转换为 QColor
class Color {
public:
    constexpr Color() = default;

    constexpr Color(int r, int g, int b, int a = 255) 
        : r_(clamp(r))
        , g_(clamp(g))
        , b_(clamp(b))
        , a_(clamp(a)) {
    }

    Color(const QColor& c);

    static Color fromHex(const std::string& hex);

    int r() const { return r_; }
    int g() const { return g_; }
    int b() const { return b_; }
    int a() const { return a_; }

    QColor toQColor() const;
    operator QColor() const { return toQColor(); }

    Color withAlpha(int a) const { return Color(r_, g_, b_, a); }

    bool operator==(const Color& other) const {
        return r_ == other.r_ && g_ == other.g_ && b_ == other.b_ && a_ == other.a_;
    }

    static Color transparent() { return Color(0, 0, 0, 0); }
    static Color white() { return Color(255, 255, 255); }
    static Color black() { return Color(0, 0, 0); }

    // Material Blue
    static Color blue() { return Color(33, 150, 243); }
    static Color lightBlue() { return Color(100, 181, 246); }
    static Color darkBlue() { return Color(25, 118, 210); }

    // Material Red
    static Color red() { return Color(244, 67, 54); }
    static Color lightRed() { return Color(239, 154, 154); }
    static Color darkRed() { return Color(211, 47, 47); }

    // Material Green
    static Color green() { return Color(76, 175, 80); }
    static Color lightGreen() { return Color(165, 214, 167); }
    static Color darkGreen() { return Color(56, 142, 60); }

    // Material Orange
    static Color orange() { return Color(255, 152, 0); }
    static Color lightOrange() { return Color(255, 183, 77); }
    static Color darkOrange() { return Color(245, 124, 0); }

    // Material Grey
    static Color grey50() { return Color(250, 250, 250); }
    static Color grey100() { return Color(245, 245, 245); }
    static Color grey200() { return Color(238, 238, 238); }
    static Color grey300() { return Color(224, 224, 224); }
    static Color grey400() { return Color(189, 189, 189); }
    static Color grey500() { return Color(158, 158, 158); }
    static Color grey600() { return Color(117, 117, 117); }
    static Color grey700() { return Color(97, 97, 97); }
    static Color grey800() { return Color(66, 66, 66); }
    static Color grey900() { return Color(33, 33, 33); }

private:
    static constexpr uint8_t clamp(int v) {
        return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
    }

    uint8_t r_ = 0;
    uint8_t g_ = 0;
    uint8_t b_ = 0;
    uint8_t a_ = 255;
};

inline std::ostream& operator<<(std::ostream& os, const Color& c) {
    return os << "rgba(" << c.r() << ',' << c.g() << ',' << c.b() << ',' << c.a() << ')';
}

} // namespace rui
