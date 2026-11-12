// effects.hpp
#pragma once

#include <functional>
#include <memory>

#include "composable.hpp"
#include "coroutine_scope.hpp"

namespace rui {

// 获取绑定到当前组合的作用域，组合销毁时自动取消所有任务。
inline std::shared_ptr<CoroutineScope> rememberCoroutineScope()
{
    auto& scope = remember<std::shared_ptr<CoroutineScope>>(std::shared_ptr<CoroutineScope>());
    if (!scope) {
        scope = std::make_shared<CoroutineScope>();
    }
    return scope;
}

// LaunchedEffect —— key 变化时取消旧协程并重新启动 block。
template <typename Key, typename Block>
void LaunchedEffect(const Key& key, Block&& block)
{
    struct Slot {
        Key key;
        std::shared_ptr<CoroutineScope> scope;
    };

    auto& slot = remember<Slot>(Slot{key, nullptr});
    if (slot.scope && slot.key == key) {
        return;
    }
    if (slot.scope) {
        slot.scope->cancel();
    }
    slot.key = key;
    slot.scope = std::make_shared<CoroutineScope>();
    slot.scope->launch(block());
}

// DisposableEffect —— key 变化或离开组合时执行 onDispose。
template <typename Key, typename Effect>
void DisposableEffect(const Key& key, Effect&& effect)
{
    struct Slot {
        Key key;
        std::function<void()> onDispose;
    };

    auto& slot = remember<Slot>(Slot{key, nullptr});
    if (slot.onDispose && slot.key == key) {
        return;
    }
    if (slot.onDispose) {
        slot.onDispose();
    }
    slot.key = key;
    slot.onDispose = effect();
}

// SideEffect —— 每次重组时执行。
template <typename Block>
void SideEffect(Block&& block)
{
    block();
}

// produceState —— 将异步/协程数据转为可观察状态。
// key 变化时取消旧协程，以 initialValue 重建状态并启动新的 producer。
// producer 接收新状态，返回一个 Job 协程，在其内通过 state->set(...) 逐步更新。
template <typename T, typename Key, typename Producer>
std::shared_ptr<MutableState<T>> produceState(T initialValue, const Key& key, Producer&& producer)
{
    struct Slot {
        Key key;
        std::shared_ptr<MutableState<T>> state;
        std::shared_ptr<CoroutineScope> scope;
    };

    auto& slot = remember<Slot>(Slot{key, nullptr, nullptr});
    if (slot.scope && slot.key == key) {
        return slot.state;
    }
    if (slot.scope) {
        slot.scope->cancel();
    }
    slot.key = key;
    slot.state = std::make_shared<MutableState<T>>(std::move(initialValue));
    slot.scope = std::make_shared<CoroutineScope>();
    slot.scope->launch(producer(slot.state));
    return slot.state;
}

} // namespace rui
