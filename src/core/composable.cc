// composable.cc
#include "composable.hpp"

#include <QMetaObject>

namespace rui {

void ComposableContext::invalidate() {
    if (needRecompose_) {
        return;
    }
    needRecompose_ = true;
    if (recomposer_) {
        recomposer_();
    }
}

bool ComposableContext::needsRecompose() const {
    return needRecompose_;
}

void ComposableContext::markComposed() {
    needRecompose_ = false;
}

void ComposableContext::setRecomposer(std::function<void()> recomposer) {
    recomposer_ = std::move(recomposer);
}

void ComposableContext::reset() {
    needRecompose_ = false;
    ++generation_;
    rememberSlot_ = 0;
}

ComposableContext* ComposableContext::current() {
    return currentContext_;
}

void ComposableContext::setCurrent(ComposableContext* ctx) {
    currentContext_ = ctx;
}

void ComposeWidget::recomposeNow() {
    recomposeScheduled_ = false;

    QLayoutItem* item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    context_->reset();

    auto* old = ComposableContext::current();
    ComposableContext::setCurrent(context_.get());

    auto result = factory_();

    ComposableContext::setCurrent(old);

    if (result) {
        layout_->addWidget(result.release());
    }

    context_->markComposed();
}

void ComposeWidget::scheduleRecompose() {
    if (recomposeScheduled_) {
        return;
    }
    recomposeScheduled_ = true;
    QMetaObject::invokeMethod(this, [this]() {
        if (context_->needsRecompose()) {
            recomposeNow();
        }
    }, Qt::QueuedConnection);
}

} // namespace rui
