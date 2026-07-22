// composable.hpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include <functional>
#include <memory>
#include <vector>

#include "observable_state.hpp"

namespace rui {

#define Composable

class ComposableContext : public std::enable_shared_from_this<ComposableContext> {
public:
    void invalidate();
    bool needsRecompose() const;
    void markComposed();

    template <typename T>
    void observe(std::shared_ptr<MutableState<T>> state) {
        auto weak = std::weak_ptr<ComposableContext>(shared_from_this());
        auto gen = generation_;
        state->observe([weak, gen](const T&) {
            if (auto ctx = weak.lock()) {
                if (ctx->generation_ == gen) {
                    ctx->invalidate();
                }
            }
        });
    }

    template <typename T>
    T& remember(T&& initialValue) {
        if (rememberSlot_ < remembered_.size()) {
            auto& entry = remembered_[rememberSlot_];
            rememberSlot_++;
            return *static_cast<std::decay_t<T>*>(entry.get());
        }
        auto value = std::make_shared<std::decay_t<T>>(std::forward<T>(initialValue));
        remembered_.push_back(value);
        rememberSlot_++;
        return *static_cast<std::decay_t<T>*>(value.get());
    }

    void setRecomposer(std::function<void()> recomposer);
    void reset();
    static ComposableContext* current();
    static void setCurrent(ComposableContext* ctx);

private:
    bool needRecompose_ = false;
    uint64_t generation_ = 0;
    size_t rememberSlot_ = 0;
    std::function<void()> recomposer_;
    std::vector<std::shared_ptr<void>> remembered_;
    inline static thread_local ComposableContext* currentContext_ = nullptr;
};

template <typename T>
T& remember(T&& initialValue) {
    return ComposableContext::current()->remember<T>(std::forward<T>(initialValue));
}

template <typename T>
void observe(std::shared_ptr<MutableState<T>> state) {
    if (auto* ctx = ComposableContext::current()) {
        ctx->observe(state);
    }
}

class ComposeWidget : public QWidget {
public:
    template <typename F>
    explicit ComposeWidget(F&& factory) 
            : context_(std::make_shared<ComposableContext>())
            , factory_(std::forward<F>(factory)) {
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        layout_ = l;

        context_->setRecomposer([this]() {
            scheduleRecompose();
        });
        recomposeNow();
    }

private:
    void recomposeNow();
    void scheduleRecompose();

private:
    std::shared_ptr<ComposableContext> context_;
    std::function<std::unique_ptr<QWidget>()> factory_;
    QVBoxLayout* layout_ = nullptr;
    bool recomposeScheduled_ = false;
};

template <typename F>
std::unique_ptr<QWidget> compose(F&& factory) {
    return std::make_unique<ComposeWidget>(std::forward<F>(factory));
}

} // namespace rui
