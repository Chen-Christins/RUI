// dimension.hpp
#pragma once

class QWidget;

namespace rui {

enum class DimensionUnit {
    Dp,
    Px,
};

class Dimension {
public:
    constexpr Dimension() = default;
    constexpr Dimension(int value, DimensionUnit unit) : value_(value), unit_(unit) {}

    constexpr int value() const { return value_; }
    constexpr DimensionUnit unit() const { return unit_; }

    int toQtPixels(const QWidget* context = nullptr) const;

private:
    int value_ = 0;
    DimensionUnit unit_ = DimensionUnit::Dp;
};

double devicePixelRatio(const QWidget* context = nullptr);
int dp2px(int dp, const QWidget* context = nullptr);
int px2dp(int px, const QWidget* context = nullptr);

constexpr Dimension dp(int value) {
    return Dimension(value, DimensionUnit::Dp);
}

constexpr Dimension px(int value) {
    return Dimension(value, DimensionUnit::Px);
}

constexpr Dimension operator""_dp(unsigned long long value) {
    return dp(static_cast<int>(value));
}

constexpr Dimension operator""_px(unsigned long long value) {
    return px(static_cast<int>(value));
}

} // namespace rui
