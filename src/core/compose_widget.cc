#include "compose_widget.hpp"

namespace rui {

void ComposeWidget::recomposeNow()
{
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

void ComposeWidget::scheduleRecompose()
{
    if (recomposeScheduled_) {
        return;
    }
    recomposeScheduled_ = true;
    QMetaObject::invokeMethod(
        this,
        [this]() {
            if (context_->needsRecompose()) {
                recomposeNow();
            }
        },
        Qt::QueuedConnection);
}

} // namespace rui