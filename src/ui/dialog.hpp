// dialog.hpp
#pragma once

#include <functional>
#include <string>

#include "component.hpp"
#include "modifier.hpp"

namespace rui {

// AlertDialog —— 模态弹窗，返回 QDialog（可通过 qobject_cast<QDialog*> 调用 exec/show）。
Component AlertDialog(
    std::function<void()> onDismissRequest = nullptr,
    Component confirmButton = nullptr,
    Component dismissButton = nullptr,
    const std::string& title = "",
    const std::string& text = "",
    Modifier modifier = Modifier{});

} // namespace rui
