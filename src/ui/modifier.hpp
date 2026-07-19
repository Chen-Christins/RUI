// modifier.hpp
#pragma once

#include <QWidget>

namespace rui {

// 修饰符系统
class Modifier {
public:
    Modifier& padding(int left, int top, int right, int bottom) {
        padding_ = {left, top, right, bottom};
        return *this;
    }

    Modifier& size(int width, int height) {
        size_ = {width, height};
        return *this;
    }

    struct Padding {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };
    
    struct Size {
        int width = -1;
        int height = -1;
    };

    Padding padding_{};
    Size size_{-1, -1};
};

// 将 modifier 应用到 widget（尺寸 + 内边距）
void applyModifier(QWidget* widget, const Modifier& modifier);

} // namespace rui
