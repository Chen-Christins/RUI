#pragma once

#include <functional>
#include <string>

#include "color.hpp"
#include "../core/composition_local.hpp"

namespace rui {

struct ColorScheme {
    Color primary = Color::blue();
    Color onPrimary = Color::white();
    Color secondary = Color::darkBlue();
    Color onSecondary = Color::white();
    Color background = Color::white();
    Color onBackground = Color::grey900();
    Color surface = Color::white();
    Color onSurface = Color::grey900();
    Color error = Color::red();
    Color onError = Color::white();
    Color inverseSurfaceColor = Color::grey900();
    Color inverseOnSurfaceColor = Color::white();

    bool operator==(const ColorScheme& other) const
    {
        return primary == other.primary && onPrimary == other.onPrimary &&
               secondary == other.secondary && onSecondary == other.onSecondary &&
               background == other.background && onBackground == other.onBackground &&
               surface == other.surface && onSurface == other.onSurface &&
               error == other.error && onError == other.onError &&
               inverseSurfaceColor == other.inverseSurfaceColor &&
               inverseOnSurfaceColor == other.inverseOnSurfaceColor;
    }

    bool operator!=(const ColorScheme& other) const { return !(*this == other); }

    ColorScheme copyWith(
        Color newPrimary = Color{},
        Color newSecondary = Color{},
        Color newBackground = Color{},
        Color newSurface = Color{},
        Color newError = Color{}) const
    {
        ColorScheme copy = *this;
        if (newPrimary != Color{}) {
            copy.primary = newPrimary;
        }
        if (newSecondary != Color{}) {
            copy.secondary = newSecondary;
        }
        if (newBackground != Color{}) {
            copy.background = newBackground;
        }
        if (newSurface != Color{}) {
            copy.surface = newSurface;
        }
        if (newError != Color{}) {
            copy.error = newError;
        }
        return copy;
    }

    static ColorScheme light()
    {
        return ColorScheme{};
    }

    static ColorScheme dark()
    {
        ColorScheme scheme;
        scheme.primary = Color::lightBlue();
        scheme.onPrimary = Color::black();
        scheme.secondary = Color::lightOrange();
        scheme.onSecondary = Color::black();
        scheme.background = Color::grey900();
        scheme.onBackground = Color::grey50();
        scheme.surface = Color::grey800();
        scheme.onSurface = Color::grey50();
        scheme.error = Color::lightRed();
        scheme.onError = Color::black();
        scheme.inverseSurfaceColor = Color::grey50();
        scheme.inverseOnSurfaceColor = Color::grey900();
        return scheme;
    }

    Color inverseSurface() const { return inverseSurfaceColor; }
    Color inverseOnSurface() const { return inverseOnSurfaceColor; }
};

inline Color resolveThemeColor(Color color, const ColorScheme& scheme)
{
    if (!color.isThemeToken()) {
        return color;
    }
    switch (color.themeToken()) {
    case 1:
        return scheme.primary.withAlpha(color.a());
    case 2:
        return scheme.surface.withAlpha(color.a());
    case 3:
        return scheme.error.withAlpha(color.a());
    case 4:
        return scheme.onSurface.withAlpha(color.a());
    default:
        return color;
    }
}

inline std::string themeColorCss(Color color)
{
    if (color.isThemeToken()) {
        return {};
    }
    return color.toQColor().name(QColor::HexArgb).toStdString();
}

inline std::string themeColorCss(Color color, const ColorScheme& scheme)
{
    return themeColorCss(resolveThemeColor(color, scheme));
}

struct Typography {
    int bodyFontSize = 14;
    int titleFontSize = 20;
    int headlineFontSize = 28;
    int labelFontSize = 12;
    int bodyFontWeight = 400;
    int titleFontWeight = 600;

    bool operator==(const Typography& other) const
    {
        return bodyFontSize == other.bodyFontSize && titleFontSize == other.titleFontSize &&
               headlineFontSize == other.headlineFontSize && labelFontSize == other.labelFontSize &&
               bodyFontWeight == other.bodyFontWeight && titleFontWeight == other.titleFontWeight;
    }

    bool operator!=(const Typography& other) const { return !(*this == other); }

    Typography copyWith(
        int newBodyFontSize = -1,
        int newTitleFontSize = -1,
        int newHeadlineFontSize = -1,
        int newLabelFontSize = -1,
        int newBodyFontWeight = -1,
        int newTitleFontWeight = -1) const
    {
        Typography copy = *this;
        if (newBodyFontSize >= 0) {
            copy.bodyFontSize = newBodyFontSize;
        }
        if (newTitleFontSize >= 0) {
            copy.titleFontSize = newTitleFontSize;
        }
        if (newHeadlineFontSize >= 0) {
            copy.headlineFontSize = newHeadlineFontSize;
        }
        if (newLabelFontSize >= 0) {
            copy.labelFontSize = newLabelFontSize;
        }
        if (newBodyFontWeight >= 0) {
            copy.bodyFontWeight = newBodyFontWeight;
        }
        if (newTitleFontWeight >= 0) {
            copy.titleFontWeight = newTitleFontWeight;
        }
        return copy;
    }
};

inline CompositionLocal<ColorScheme> LocalColorScheme{[]() {
    return ColorScheme::light();
}};

inline CompositionLocal<Typography> LocalTypography{[]() {
    return Typography{};
}};

inline ColorScheme currentColorScheme()
{
    return LocalColorScheme.current();
}

inline Typography currentTypography()
{
    return LocalTypography.current();
}

template <typename Content>
auto RuiTheme(
    ColorScheme colorScheme,
    Typography typography,
    Content&& content)
{
    return CompositionLocalProvider(
        LocalColorScheme,
        std::move(colorScheme),
        LocalTypography,
        std::move(typography),
        std::forward<Content>(content)
    );
}

template <typename Content>
auto RuiTheme(Content&& content)
{
    return RuiTheme(ColorScheme::light(), Typography{}, std::forward<Content>(content));
}

template <typename Content>
Component RuiTheme(
    std::shared_ptr<MutableState<ColorScheme>> colorScheme,
    std::shared_ptr<MutableState<Typography>> typography,
    Content&& content)
{
    return compose([colorScheme, typography, content = std::forward<Content>(content)]() mutable {
        if (colorScheme) {
            observe(colorScheme);
        }
        if (typography) {
            observe(typography);
        }
        return RuiTheme(
            colorScheme ? colorScheme->get() : ColorScheme::light(),
            typography ? typography->get() : Typography{},
            content
        );
    });
}

} // namespace rui
