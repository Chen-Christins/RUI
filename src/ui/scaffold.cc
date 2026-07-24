#include "scaffold.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

// ── TopAppBar ──

Component TopAppBar(
    std::shared_ptr<MutableState<std::string>> title,
    Component navigationIcon,
    std::vector<Component> actions,
    Modifier modifier
) {
    auto widget = std::make_unique<QWidget>();
    utils::setComponentType(widget.get(), kUITopAppBarProperty);
    widget->setStyleSheet("QWidget { background: #1976D2; }");

    auto* layout = new QHBoxLayout(widget.get());
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(4);

    if (navigationIcon) {
        layout->addWidget(navigationIcon.release());
    }

    auto* titleLabel = new QLabel(QString::fromStdString(title->get()), widget.get());
    titleLabel->setStyleSheet(
        "QLabel { color: white; font-size: 18px; font-weight: bold; border: none; }"
    );
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(titleLabel);

    title->observe([titleLabel](const std::string& value) {
        titleLabel->setText(QString::fromStdString(value));
    });

    for (auto& action : actions) {
        if (action) {
            layout->addWidget(action.release());
        }
    }

    widget->setFixedHeight(56);
    applyModifier(widget.get(), modifier);
    return widget;
}

// ── FloatingActionButton ──

Component FloatingActionButton(
    std::function<void()> onClick,
    Component content,
    Modifier modifier
) {
    auto* btn = new QPushButton();
    btn->setFixedSize(56, 56);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton {"
        "  background: #1976D2;"
        "  border: none;"
        "  border-radius: 28px;"
        "  color: white;"
        "  font-size: 24px;"
        "}"
        "QPushButton:hover {"
        "  background: #1565C0;"
        "}"
        "QPushButton:pressed {"
        "  background: #0D47A1;"
        "}"
    );

    if (content) {
        if (auto* label = qobject_cast<QLabel*>(content.get())) {
            btn->setText(label->text());
        }
    }

    QObject::connect(btn, &QPushButton::clicked, onClick);
    applyModifier(btn, modifier);

    auto* container = new QWidget();
    utils::setComponentType(container, kUIFloatingActionButtonProperty);
    auto* cl = new QVBoxLayout(container);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->addWidget(btn, 0, Qt::AlignRight | Qt::AlignBottom);
    container->setAttribute(Qt::WA_TranslucentBackground);
    container->setFixedSize(72, 72);

    return Component(container);
}

// ── ScaffoldWidget ──

namespace {

class ScaffoldWidget : public QWidget {
public:
    ScaffoldWidget(
        Component topBar,
        Component content,
        Component bottomBar,
        Component fab
    )
        : QWidget(nullptr)
    {
        utils::setComponentType(this, kUIScaffoldProperty);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        if (topBar) {
            topBar_ = topBar.release();
            mainLayout->addWidget(topBar_);
        }

        if (content) {
            content_ = content.release();
            content_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(content_, 1);
        }

        if (bottomBar) {
            bottomBar_ = bottomBar.release();
            mainLayout->addWidget(bottomBar_);
        }

        if (fab) {
            fab_ = fab.release();
            fab_->setParent(this);
            fab_->raise();
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (fab_) {
            int margin = 16;
            fab_->move(width() - fab_->width() - margin,
                       height() - fab_->height() - margin);
        }
    }

    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        if (fab_) {
            int margin = 16;
            fab_->move(width() - fab_->width() - margin,
                       height() - fab_->height() - margin);
            fab_->show();
        }
    }

private:
    QWidget* topBar_ = nullptr;
    QWidget* content_ = nullptr;
    QWidget* bottomBar_ = nullptr;
    QWidget* fab_ = nullptr;
};

} // namespace

Component Scaffold(
    Modifier modifier,
    Component topBar,
    Component content,
    Component bottomBar,
    Component floatingActionButton
) {
    auto widget = std::make_unique<ScaffoldWidget>(
        std::move(topBar), std::move(content),
        std::move(bottomBar), std::move(floatingActionButton)
    );
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
