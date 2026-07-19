// modifier.hpp
#pragma once

#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;

#include "color.hpp"

namespace rui {

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
    Modifier& padding(int left, int top, int right, int bottom);
    Modifier& size(int width, int height);
    Modifier& foreground(Color color);
    Modifier& background(Color color);
    Modifier& fontSize(int px);
    Modifier& border(Color color, int width = 1);
    Modifier& align(Alignment a);

private:
    friend Qt::Alignment toQtAlignment(Alignment a);
    friend void applyModifier(QWidget* widget, const Modifier& modifier);
    friend void applyArrangement(QVBoxLayout* layout, Arrangement a, int itemCount);

private:
    struct Padding { int left = 0, top = 0, right = 0, bottom = 0; };
    struct Size    { int width = -1, height = -1; };

    Padding   padding_{};
    Size      size_{-1, -1};
    Color     foreground_;
    bool      foregroundSet_ = false;
    Color     background_;
    bool      backgroundSet_ = false;
    int       fontSize_      = 0;
    Color     borderColor_;
    int       borderWidth_   = 0;
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
