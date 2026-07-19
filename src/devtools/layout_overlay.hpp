// layout_overlay.hpp
#pragma once

#include <QWidget>
#include <QMap>
#include <QColor>

#include "utils/singleton.hpp"
#include "utils/noncopyable.hpp"

namespace rui {

// 布局高亮覆盖层 — 在被检查的 widget 上绘制半透明彩色边框
class LayoutOverlay : public Singleton<LayoutOverlay>, Noncopyable {
public:
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
friend class Singleton<LayoutOverlay>;
    LayoutOverlay() = default;

private:
    // 是否启用高亮功能
    bool enabled_ = false;
    // 根窗口指针，用于在事件过滤器中捕获 widget 的绘制事件
    QWidget* rootWindow_ = nullptr;
    // 高亮覆盖层 widget → 目标 widget 的映射
    QMap<QWidget*, QWidget*> overlays_;
};

} // namespace rui
