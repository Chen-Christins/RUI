// composable.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "observable_state.hpp"

namespace rui {

class ComposableContext : public std::enable_shared_from_this<ComposableContext> {
public:
    void invalidate();

    bool needsRecompose() const;

    void markComposed();

    template <typename T>
    void observe(std::shared_ptr<MutableState<T>> state);

    template <typename T>
    T& remember(T&& initialValue);

    template <typename K, typename T>
    T& remember(const K& key, T&& initialValue);

    void setRecomposer(std::function<void()> recomposer);

    void reset();

    uint64_t generation() const;

    static ComposableContext* current();

    static void setCurrent(ComposableContext* ctx);

private:
    struct KeyedEntry {
        size_t key = 0;
        std::shared_ptr<void> value;
    };

    mutable std::mutex mutex_;
    bool needRecompose_ = false;
    uint64_t generation_ = 0;
    size_t rememberSlot_ = 0;
    std::function<void()> recomposer_;
    std::vector<std::shared_ptr<void>> remembered_;
    std::vector<KeyedEntry> keyed_;
    inline static thread_local ComposableContext* currentContext_ = nullptr;
};

template <typename T>
void ComposableContext::observe(std::shared_ptr<MutableState<T>> state)
{
    auto weak = std::weak_ptr<ComposableContext>(shared_from_this());
    auto gen = generation();
    state->observe([weak, gen](const T&) {
        if (const auto ctx = weak.lock()) {
            if (ctx->generation() == gen) {
                ctx->invalidate();
            }
        }
    });
}

template <typename T>
T& ComposableContext::remember(T&& initialValue)
{
    if (rememberSlot_ < remembered_.size()) {
        const auto& entry = remembered_[rememberSlot_];
        rememberSlot_++;
        return *static_cast<std::decay_t<T>*>(entry.get());
    }
    auto value = std::make_shared<std::decay_t<T>>(std::forward<T>(initialValue));
    remembered_.push_back(value);
    rememberSlot_++;
    return *static_cast<std::decay_t<T>*>(value.get());
}

template <typename K, typename T>
T& ComposableContext::remember(const K& key, T&& initialValue)
{
    using Storage = std::decay_t<T>;
    const size_t hash = std::hash<std::decay_t<K>>{}(key);
    for (const auto& entry : keyed_) {
        if (entry.key == hash) {
            return *static_cast<Storage*>(entry.value.get());
        }
    }
    auto value = std::make_shared<Storage>(std::forward<T>(initialValue));
    keyed_.push_back(KeyedEntry{hash, value});
    return *static_cast<Storage*>(value.get());
}

template <typename T>
T& remember(T&& initialValue)
{
    return ComposableContext::current()->remember<T>(std::forward<T>(initialValue));
}

template <typename K, typename T>
T& remember(const K& key, T&& initialValue)
{
    return ComposableContext::current()->remember(key, std::forward<T>(initialValue));
}

template <typename T>
void observe(std::shared_ptr<MutableState<T>> state)
{
    if (auto* ctx = ComposableContext::current()) {
        ctx->observe(state);
    }
}

} // namespace rui
