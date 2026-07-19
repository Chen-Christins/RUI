// window_config.hpp
#pragma once

#include <string>

namespace rui {

/**
 * @brief 窗口配置，PlatformAdapter 主窗口和 NavController::openWindow 共用
 */
struct WindowConfig {
    std::string title;
    int width = 400;
    int height = 300;
    int minWidth = 0;   // 0 = 不限制
    int minHeight = 0;
    int maxWidth = 0;   // 0 = 不限制
    int maxHeight = 0;
};

} // namespace rui
