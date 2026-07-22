// modifier.hpp
#pragma once

#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;

#include "color.hpp"
#include "dimension.hpp"

namespace rui {

namespace detail {

inline constexpr const char* kComponentTypeProperty = "rui_component_type";

inline void setComponentType(QWidget* widget, const char* type) {
    widget->setProperty(kComponentTypeProperty, type);
}

} // namespace detail

enum class Alignment {
    TopStart, TopCenter, TopEnd,
    CenterStart, Center, CenterEnd,
    BottomStart, BottomCenter, BottomEnd,
};

enum class Arrangement {
    Start,         // 紧凑排列在开头（默认）
    End,           // 紧凑排列在末尾
    Center,        // 居中
    SpaceBetween,  // 两端对齐
    SpaceEvenly,   // 均匀分布
    SpaceAround,   // 子组件两侧等距
};

class Modifier {
public:
    Modifier& padding(Dimension left, Dimension top, Dimension right, Dimension bottom);
    Modifier& size(Dimension width, Dimension height);
    Modifier& foreground(Color color);
    Modifier& background(Color color);
    Modifier& fontSize(Dimension size);
    Modifier& border(Color color, Dimension width = 1_px);
    Modifier& align(Alignment a);

private:
    friend Qt::Alignment toQtAlignment(Alignment a);
    friend void applyModifier(QWidget* widget, const Modifier& modifier);
    friend void applyArrangement(QVBoxLayout* layout, Arrangement a, int itemCount);

private:
    struct Padding { Dimension left, top, right, bottom; };
    struct Size    { Dimension width = dp(-1), height = dp(-1); };

    Padding   padding_{};
    Size      size_{dp(-1), dp(-1)};
    Color     foreground_;
    bool      foregroundSet_ = false;
    Color     background_;
    bool      backgroundSet_ = false;
    Dimension fontSize_;
    Color     borderColor_;
    Dimension borderWidth_;
    Alignment alignment_     = Alignment::TopStart;
    bool      alignmentSet_  = false;
};

void applyModifier(QWidget* widget, const Modifier& modifier);

// 对齐转 Qt 常量
Qt::Alignment toQtAlignment(Alignment a);
// 对 Column 应用 Arrangement
void applyArrangement(QVBoxLayout* layout, Arrangement a, int itemCount);
// 对 Row 应用 Arrangement
void applyArrangement(QHBoxLayout* layout, Arrangement a, int itemCount);

} // namespace rui
