// layout_overlay.hpp
#pragma once

#include <QWidget>
#include <QMap>

#include "ui/color.hpp"
#include "utils/singleton.hpp"
#include "utils/noncopyable.hpp"

namespace rui {

// 布局高亮覆盖层 — 在被检查的 widget 上绘制半透明彩色边框
class LayoutOverlay : public Singleton<LayoutOverlay>, Noncopyable {
public:
    void highlightWidget(QWidget* widget, const Color& color = Color(0, 120, 212, 80));
    void clearHighlight(QWidget* widget);
    void clearAll();

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    void installOn(QWidget* rootWindow);

private:
    friend class Singleton<LayoutOverlay>;
    LayoutOverlay() = default;

    bool enabled_ = false;
    QWidget* rootWindow_ = nullptr;
    QMap<QWidget*, QWidget*> overlays_;
};

} // namespace rui
