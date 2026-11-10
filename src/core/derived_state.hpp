// derived_state.hpp
#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "composable.hpp"
#include "observable_state.hpp"

namespace rui {

// DerivedState —— 派生状态：仅在依赖变化时重新计算。
// 求值期间读取到的 MutableState 会被自动追踪，任一依赖变化即触发重算。
template <typename T>
class DerivedState : public std::enable_shared_from_this<DerivedState<T>> {
public:
    using Compute = std::function<T()>;

    explicit DerivedState(Compute compute)
        : compute_(std::move(compute))
        , value_(std::make_shared<MutableState<T>>(T{}))
    {
    }

    // 建立失效回调并完成首次求值；需在 shared_ptr 构造完成后调用。
    void init()
    {
        tracker_.setOnInvalidate([weak = this->weak_from_this()]() {
            if (auto self = weak.lock()) {
                self->recompute();
            }
        });
        recompute();
    }

    std::shared_ptr<MutableState<T>> value() const { return value_; }

    void recompute()
    {
        tracker_.clearSubscriptions();
        detail::ReadTracker::begin(&tracker_);
        T v = compute_();
        detail::ReadTracker::end();
        value_->set(v);
    }

private:
    Compute compute_;
    std::shared_ptr<MutableState<T>> value_;
    detail::ReadTracker tracker_;
};

// derivedStateOf —— 在组合中创建并记住派生状态。
template <typename T>
std::shared_ptr<MutableState<T>> derivedStateOf(std::function<T()> compute)
{
    auto make = [&]() {
        auto derived = std::make_shared<DerivedState<T>>(std::move(compute));
        derived->init();
        return derived;
    };

    if (auto* ctx = ComposableContext::current()) {
        auto& slot = ctx->remember<std::shared_ptr<DerivedState<T>>>(std::shared_ptr<DerivedState<T>>());
        if (!slot) {
            slot = make();
        }
        return slot->value();
    }
    return make()->value();
}

} // namespace rui
