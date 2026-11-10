#include "compose_widget.hpp"

#include <exception>

#include "utils/logger.hpp"

namespace rui {

namespace {

class CompositionContextGuard {
public:
    explicit CompositionContextGuard(ComposableContext* context)
        : previous_(ComposableContext::current())
    {
        ComposableContext::setCurrent(context);
    }

    ~CompositionContextGuard() {
        ComposableContext::setCurrent(previous_);
    }

private:
    ComposableContext* previous_ = nullptr;
};

} // namespace

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

    std::unique_ptr<QWidget> result;
    try {
        CompositionContextGuard contextGuard(context_.get());
        result = factory_();
    } catch (const std::exception& exception) {
        auto logger = Logger::getLogger(QStringLiteral("ComposeWidget"));
        ERROR(logger) << "Composition failed: " << exception.what();
        context_->markComposed();
        return;
    } catch (...) {
        auto logger = Logger::getLogger(QStringLiteral("ComposeWidget"));
        ERROR(logger) << "Composition failed with an unknown exception";
        context_->markComposed();
        return;
    }

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
