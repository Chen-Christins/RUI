#pragma once

#include <functional>

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

struct Typography {
    int bodyFontSize = 14;
    int titleFontSize = 20;
    int headlineFontSize = 28;
    int labelFontSize = 12;
    int bodyFontWeight = 400;
    int titleFontWeight = 600;
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
        [&]() {
            return CompositionLocalProvider(
                LocalTypography,
                std::move(typography),
                std::forward<Content>(content)
            );
        }
    );
}

template <typename Content>
auto RuiTheme(Content&& content)
{
    return RuiTheme(ColorScheme::light(), Typography{}, std::forward<Content>(content));
}

} // namespace rui
