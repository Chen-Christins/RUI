// window_config.hpp
#pragma once

#include <string>

namespace rui {

/**
 * @brief 窗口配置，PlatformAdapter 主窗口和 NavController::openWindow 共用
 */
struct WindowConfig {
    std::string title;
    std::string key;    // 唯一标识，相同 key 不会重复打开（空 = 允许重复）
    int width = 400;
    int height = 300;
    int minWidth = 0;   // 0 = 不限制
    int minHeight = 0;
    int maxWidth = 0;   // 0 = 不限制
    int maxHeight = 0;
};

} // namespace rui
