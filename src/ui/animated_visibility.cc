// animated_visibility.cc
#include "animated_visibility.hpp"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVBoxLayout>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

namespace {

class AnimatedVisibilityWidget : public QWidget {
public:
    AnimatedVisibilityWidget(std::shared_ptr<MutableState<bool>> visible, Component content)
        : QWidget(nullptr)
    {
        utils::setComponentType(this, kUIAnimatedVisibilityProperty);
        setAttribute(Qt::WA_TranslucentBackground);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        if (content) {
            layout->addWidget(content.release());
        }

        opacity_ = new QGraphicsOpacityEffect(this);
        opacity_->setOpacity(visible->get() ? 1.0 : 0.0);
        setGraphicsEffect(opacity_);

        if (!visible->get()) {
            setVisible(false);
        }

        anim_ = new QPropertyAnimation(opacity_, "opacity", this);
        anim_->setDuration(200);
        anim_->setEasingCurve(QEasingCurve::InOutCubic);

        QObject::connect(anim_, &QPropertyAnimation::finished, this, [this]() {
            if (hiding_) {
                setVisible(false);
            }
        });

        visible->observe(this, [this](bool v) {
            animateTo(v);
        });
    }

private:
    void animateTo(bool v)
    {
        anim_->stop();
        hiding_ = !v;
        if (v) {
            setVisible(true);
            anim_->setEndValue(1.0);
        } else {
            anim_->setEndValue(0.0);
        }
        anim_->setStartValue(opacity_->opacity());
        anim_->start();
    }

private:
    QGraphicsOpacityEffect* opacity_ = nullptr;
    QPropertyAnimation* anim_ = nullptr;
    bool hiding_ = false;
};

} // namespace

Component AnimatedVisibility(std::shared_ptr<MutableState<bool>> visible, Component content, Modifier modifier)
{
    auto widget = std::make_unique<AnimatedVisibilityWidget>(std::move(visible), std::move(content));
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
