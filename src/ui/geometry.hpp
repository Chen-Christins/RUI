// geometry.hpp
#pragma once

#include <QMetaType>
#include <cmath>

#include "dimension.hpp"

namespace rui {

// Size —— 二维尺寸。
struct Size {
    Dimension width = 0_dp;
    Dimension height = 0_dp;

    bool operator==(const Size& other) const { return width == other.width && height == other.height; }
};

// Offset —— 二维偏移。
struct Offset {
    Dimension x = 0_dp;
    Dimension y = 0_dp;

    bool operator==(const Offset& other) const { return x == other.x && y == other.y; }
};

// Rect —— 矩形范围。
struct Rect {
    Dimension left = 0_dp;
    Dimension top = 0_dp;
    Dimension right = 0_dp;
    Dimension bottom = 0_dp;

    bool operator==(const Rect& other) const { return left == other.left && top == other.top && right == other.right && bottom == other.bottom; }
};

inline Dimension lerpDimension(const Dimension& a, const Dimension& b, float t)
{
    int va = a.value();
    int vb = b.value();
    int v = static_cast<int>(std::lround(va + (vb - va) * t));
    return Dimension(v, a.unit());
}

inline Size lerp(const Size& a, const Size& b, float t)
{
    return Size{lerpDimension(a.width, b.width, t), lerpDimension(a.height, b.height, t)};
}

inline Offset lerp(const Offset& a, const Offset& b, float t)
{
    return Offset{lerpDimension(a.x, b.x, t), lerpDimension(a.y, b.y, t)};
}

inline Rect lerp(const Rect& a, const Rect& b, float t)
{
    return Rect{
        lerpDimension(a.left, b.left, t),
        lerpDimension(a.top, b.top, t),
        lerpDimension(a.right, b.right, t),
        lerpDimension(a.bottom, b.bottom, t),
    };
}

inline std::ostream& operator<<(std::ostream& os, const Size& s)
{
    return os << "Size(" << s.width << ", " << s.height << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Offset& o)
{
    return os << "Offset(" << o.x << ", " << o.y << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Rect& r)
{
    return os << "Rect(" << r.left << ", " << r.top << ", " << r.right << ", " << r.bottom << ")";
}

} // namespace rui

Q_DECLARE_METATYPE(rui::Size)
Q_DECLARE_METATYPE(rui::Offset)
Q_DECLARE_METATYPE(rui::Rect)
