// composable.hpp
#pragma once

#include <functional>

namespace rui {

// 组合上下文 - 管理重组生命周期
class ComposableContext {
public:
    virtual ~ComposableContext() = default;
    void invalidate() { needRecompose_ = true; }
    bool needsRecompose() const { return needRecompose_; }

private:
    bool needRecompose_ = false;
};

using ComposableFunction = std::function<void()>;

// Composable装饰器宏 - 标记可组合函数
#define Composable

} // namespace rui
