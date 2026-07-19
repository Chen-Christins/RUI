// layout_overlay.hpp
#pragma once

#include <QWidget>
#include <QMap>
#include <QColor>

namespace rui {

// 布局高亮覆盖层 — 在被检查的 widget 上绘制半透明彩色边框
class LayoutOverlay {
public:
    static LayoutOverlay& instance();

    // 在指定 widget 上显示/隐藏高亮
    void highlightWidget(QWidget* widget, const QColor& color = QColor(0, 120, 212, 80));
    void clearHighlight(QWidget* widget);
    void clearAll();

    // 全局开关
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // 给根窗口安装事件过滤器（必须调用一次以启用高亮）
    void installOn(QWidget* rootWindow);

private:
    LayoutOverlay() = default;

private:
    bool enabled_ = false;
    QWidget* rootWindow_ = nullptr;

    // 高亮覆盖层 widget → 目标 widget 的映射
    QMap<QWidget*, QWidget*> overlays_;
};

} // namespace rui
