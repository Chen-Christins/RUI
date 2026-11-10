// compose_widget.hpp
#pragma once

#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include "composable.hpp"

namespace rui {

class ComposeWidget : public QWidget {
public:
    template <typename F>
    explicit ComposeWidget(F&& factory);

private:
    void recomposeNow();

    void scheduleRecompose();

private:
    std::shared_ptr<ComposableContext> context_;

    std::function<std::unique_ptr<QWidget>()> factory_;

    QVBoxLayout* layout_ = nullptr;

    std::atomic_bool recomposeScheduled_ = false;
};

template <typename F>
ComposeWidget::ComposeWidget(F&& factory)
    : context_(std::make_shared<ComposableContext>())
    , factory_(std::forward<F>(factory))
{
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    layout_ = l;

    context_->setRecomposer([this]() {
        scheduleRecompose();
    });
    recomposeNow();
}

template <typename F>
std::unique_ptr<QWidget> compose(F&& factory)
{
    return std::make_unique<ComposeWidget>(std::forward<F>(factory));
}

} // namespace rui
