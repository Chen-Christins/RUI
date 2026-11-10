// dimension.cc
#include "dimension.hpp"

#include <QScreen>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace rui {

namespace {

QScreen* screenFor(const QWidget* context)
{
    if (context) {
        if (auto* screen = context->screen()) {
            return screen;
        }
    }
    return QGuiApplication::primaryScreen();
}

} // namespace

double devicePixelRatio(const QWidget* context)
{
    if (auto* screen = screenFor(context)) {
        return std::max<qreal>(1.0, screen->devicePixelRatio());
    }
    return 1.0;
}

int dp2px(int dp, const QWidget* context)
{
    if (dp <= 0) {
        return dp;
    }
    return std::max(1, static_cast<int>(std::round(dp * devicePixelRatio(context))));
}

int px2dp(int px, const QWidget* context)
{
    if (px <= 0) {
        return px;
    }
    return std::max(1, static_cast<int>(std::round(px / devicePixelRatio(context))));
}

int Dimension::toQtPixels(const QWidget* context) const
{
    if (unit_ == DimensionUnit::Dp || value_ <= 0) {
        return value_;
    }

    return px2dp(value_, context);
}

} // namespace rui
