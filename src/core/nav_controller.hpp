// nav_controller.hpp
#pragma once

#include <QStackedWidget>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../utils/noncopyable.hpp"
#include "window_config.hpp"

namespace rui {

/**
 * @brief 导航控制器，管理界面跳转
 *
 * 支持两种跳转模式：
 * - push:  推入新界面，保留当前界面在栈中，可通过 pop() 返回
 * - replace: 替换当前界面，旧界面通过 deleteLater 安全销毁
 * - pop:    返回上一界面
 *
 * 所有权说明：所有 screen widget 通过 Qt parent-child 机制由 QStackedWidget 拥有，
 * 不在栈中的旧界面通过 deleteLater() 延迟销毁，避免双重所有权。
 */
class NavController : Noncopyable {
public:
    NavController();

    void push(std::unique_ptr<QWidget> screen);

    void replace(std::unique_ptr<QWidget> screen);

    bool pop();

    bool canPop() const;

    QStackedWidget* container() const;

    /**
     * @brief 打开一个独立窗口（顶层窗口，不在导航栈中）
     * @param content 窗口内容 widget
     * @param config  窗口配置（标题、尺寸、最大/最小限制）
     */
    void openWindow(std::unique_ptr<QWidget> content, const WindowConfig& config = {});

private:
    QStackedWidget* container_;
    std::vector<QWidget*> screens_;                         // 裸指针，所有权归 QStackedWidget
    std::unordered_map<std::string, QWidget*> openWindows_; // key → 独立窗口，防止重复打开
};

} // namespace rui
