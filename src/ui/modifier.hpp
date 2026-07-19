// modifier.hpp
#pragma once

#include <QWidget>

#include "color.hpp"

namespace rui {

// 修饰符系统 —— 控制组件的 padding、尺寸、颜色、字体、边框
class Modifier {
public:
    Modifier& padding(int left, int top, int right, int bottom);
    Modifier& size(int width, int height);
    Modifier& foreground(Color color);
    Modifier& background(Color color);
    Modifier& fontSize(int px);
    Modifier& border(Color color, int width = 1);

    struct Padding {
        int left = 0, top = 0, right = 0, bottom = 0;
    };
    struct Size {
        int width = -1, height = -1;
    };

    Padding padding_{};
    Size size_{-1, -1};
    Color foreground_;
    bool foregroundSet_ = false;
    Color background_;
    bool backgroundSet_ = false;
    int fontSize_ = 0; // 0 = 未设置
    Color borderColor_;
    int borderWidth_ = 0; // 0 = 未设置
};

void applyModifier(QWidget* widget, const Modifier& modifier);

} // namespace rui
