#include "lazy_list.hpp"

#include <QScrollArea>
#include <QScrollBar>
#include <QBoxLayout>
#include <QFrame>
#include <QSpacerItem>
#include <QTimer>
#include <QWheelEvent>

#include <unordered_map>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

const char* kScrollBarStyle = R"(
QScrollBar:vertical {
    width: 5px;
    margin: 0;
    border: none;
    background: transparent;
}
QScrollBar::handle:vertical {
    min-height: 24px;
    border-radius: 2px;
    background: rgba(120, 120, 120, 96);
}
QScrollBar::handle:vertical:hover {
    background: rgba(96, 96, 96, 144);
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    width: 0;
    height: 0;
    border: none;
    background: transparent;
}
QScrollBar:horizontal {
    height: 5px;
    margin: 0;
    border: none;
    background: transparent;
}
QScrollBar::handle:horizontal {
    min-width: 24px;
    border-radius: 2px;
    background: rgba(120, 120, 120, 96);
}
QScrollBar::handle:horizontal:hover {
    background: rgba(96, 96, 96, 144);
}
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal {
    width: 0;
    height: 0;
    border: none;
    background: transparent;
}
)";

constexpr int kOverlayScrollBarThickness = 5;
constexpr int kOverlayScrollBarInset = 3;
constexpr int kOverlayScrollBarHideDelayMs = 700;

class LazyListWidget : public QScrollArea {
public:
    LazyListWidget(
        int itemCount,
        std::function<Component(int)> factory,
        Qt::Orientation orientation
    )
        : QScrollArea(nullptr)
        , itemCount_(itemCount)
        , factory_(std::move(factory))
        , orientation_(orientation)
    {
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);

        utils::setComponentType(this, orientation == Qt::Vertical ? kUILazyColumnProperty : kUILazyRowProperty);

        content_ = new QWidget(this);
        if (orientation == Qt::Vertical) {
            auto* vl = new QVBoxLayout(content_);
            vl->setContentsMargins(0, 0, 0, 0);
            vl->setSpacing(0);
            layout_ = vl;
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else {
            auto* hl = new QHBoxLayout(content_);
            hl->setContentsMargins(0, 0, 0, 0);
            hl->setSpacing(0);
            layout_ = hl;
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        setWidget(content_);
        initOverlayScrollBar();

        // Default item size.
        if (orientation == Qt::Vertical) {
            estimatedItemSize_ = 48;
        } else {
            estimatedItemSize_ = 100;
        }

        // Initialize spacers.
        if (orientation == Qt::Vertical) {
            topSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
            bottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        } else {
            topSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            bottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        }
        layout_->addItem(topSpacer_);
        layout_->addItem(bottomSpacer_);

        // Update visible items when scrolling.
        if (orientation == Qt::Vertical) {
            connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                updateVisible();
                syncOverlayScrollBar();
                showOverlayScrollBar();
            });
            connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this]() {
                syncOverlayScrollBar();
            });
        } else {
            connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                updateVisible();
                syncOverlayScrollBar();
                showOverlayScrollBar();
            });
            connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() {
                syncOverlayScrollBar();
            });
        }
    }

protected:
    void showEvent(QShowEvent* event) override {
        QScrollArea::showEvent(event);
        updateVisible();
        syncOverlayScrollBar();
    }

    void resizeEvent(QResizeEvent* event) override {
        QScrollArea::resizeEvent(event);
        positionOverlayScrollBar();
        updateVisible();
        syncOverlayScrollBar();
    }

    void wheelEvent(QWheelEvent* event) override {
        QScrollBar* bar = nativeScrollBar();
        if (!bar || bar->maximum() <= bar->minimum()) {
            QScrollArea::wheelEvent(event);
            return;
        }

        int delta = orientation_ == Qt::Vertical
            ? event->pixelDelta().y()
            : event->pixelDelta().x();
        if (delta == 0) {
            delta = orientation_ == Qt::Vertical
                ? event->angleDelta().y() / 120 * bar->singleStep() * 3
                : event->angleDelta().x() / 120 * bar->singleStep() * 3;
        }
        if (delta == 0 && orientation_ == Qt::Horizontal) {
            delta = event->angleDelta().y() / 120 * bar->singleStep() * 3;
        }

        if (delta == 0) {
            QScrollArea::wheelEvent(event);
            return;
        }

        bar->setValue(bar->value() - delta);
        showOverlayScrollBar();
        event->accept();
    }

private:
    QScrollBar* nativeScrollBar() const {
        return orientation_ == Qt::Vertical ? verticalScrollBar() : horizontalScrollBar();
    }

    void initOverlayScrollBar() {
        overlayScrollBar_ = new QScrollBar(orientation_, viewport());
        overlayScrollBar_->setStyleSheet(kScrollBarStyle);
        overlayScrollBar_->hide();

        overlayHideTimer_ = new QTimer(this);
        overlayHideTimer_->setSingleShot(true);
        connect(overlayHideTimer_, &QTimer::timeout, this, [this]() {
            if (!overlayScrollBar_) {
                return;
            }
            if (overlayScrollBar_->isSliderDown() || overlayScrollBar_->underMouse()) {
                overlayHideTimer_->start(kOverlayScrollBarHideDelayMs);
                return;
            }
            overlayScrollBar_->hide();
        });

        connect(overlayScrollBar_, &QScrollBar::valueChanged, this, [this](int value) {
            if (syncingOverlay_) {
                return;
            }
            nativeScrollBar()->setValue(value);
            showOverlayScrollBar();
        });

        positionOverlayScrollBar();
        syncOverlayScrollBar();
    }

    void positionOverlayScrollBar() {
        if (!overlayScrollBar_) {
            return;
        }

        const int inset = kOverlayScrollBarInset;
        const int thickness = kOverlayScrollBarThickness;
        if (orientation_ == Qt::Vertical) {
            overlayScrollBar_->setGeometry(
                viewport()->width() - thickness - inset,
                inset,
                thickness,
                std::max(0, viewport()->height() - inset * 2)
            );
        } else {
            overlayScrollBar_->setGeometry(
                inset,
                viewport()->height() - thickness - inset,
                std::max(0, viewport()->width() - inset * 2),
                thickness
            );
        }
        overlayScrollBar_->raise();
    }

    void syncOverlayScrollBar() {
        if (!overlayScrollBar_) {
            return;
        }

        QScrollBar* source = nativeScrollBar();
        syncingOverlay_ = true;
        overlayScrollBar_->setRange(source->minimum(), source->maximum());
        overlayScrollBar_->setPageStep(source->pageStep());
        overlayScrollBar_->setSingleStep(source->singleStep());
        overlayScrollBar_->setValue(source->value());
        syncingOverlay_ = false;

        if (source->maximum() <= source->minimum()) {
            overlayScrollBar_->hide();
        }
    }

    void showOverlayScrollBar() {
        if (!overlayScrollBar_) {
            return;
        }
        if (nativeScrollBar()->maximum() <= nativeScrollBar()->minimum()) {
            return;
        }

        positionOverlayScrollBar();
        overlayScrollBar_->show();
        overlayScrollBar_->raise();
        overlayHideTimer_->start(kOverlayScrollBarHideDelayMs);
    }

    void updateVisible() {
        if (itemCount_ <= 0 || estimatedItemSize_ <= 0) {
            return;
        }

        int scrollPos = orientation_ == Qt::Vertical
            ? verticalScrollBar()->value()
            : horizontalScrollBar()->value();
        int viewportSize = orientation_ == Qt::Vertical
            ? viewport()->height()
            : viewport()->width();

        if (viewportSize <= 0) {
            return;
        }

        int buffer = 2;
        int firstVisible = std::max(0, scrollPos / estimatedItemSize_ - buffer);
        int lastVisible = std::min(itemCount_ - 1,
            (scrollPos + viewportSize) / estimatedItemSize_ + buffer);

        // Remove items that are outside the visible range.
        for (auto it = visibleItems_.begin(); it != visibleItems_.end(); ) {
            int idx = it->first;
            if (idx < firstVisible || idx > lastVisible) {
                layout_->removeWidget(it->second);
                it->second->deleteLater();
                it = visibleItems_.erase(it);
            } else {
                ++it;
            }
        }

        // Add newly visible items.
        for (int i = firstVisible; i <= lastVisible; i++) {
            if (visibleItems_.find(i) != visibleItems_.end()) {
                continue;
            }

            auto w = factory_(i);
            if (!w) {
                continue;
            }
            QWidget* raw = w.release();

            int insertPos = 1;
            for (int j = firstVisible; j < i; j++) {
                if (visibleItems_.find(j) != visibleItems_.end()) {
                    insertPos++;
                }
            }
            layout_->insertWidget(insertPos, raw);

            if (orientation_ == Qt::Vertical) {
                raw->setFixedHeight(estimatedItemSize_);
            } else {
                raw->setFixedWidth(estimatedItemSize_);
            }
            visibleItems_[i] = raw;
        }

        // Update spacers.
        int topSize = firstVisible * estimatedItemSize_;
        int bottomSize = (itemCount_ - 1 - lastVisible) * estimatedItemSize_;
        if (bottomSize < 0) {
            bottomSize = 0;
        }

        if (orientation_ == Qt::Vertical) {
            topSpacer_->changeSize(0, topSize, QSizePolicy::Minimum, QSizePolicy::Fixed);
            bottomSpacer_->changeSize(0, bottomSize, QSizePolicy::Minimum, QSizePolicy::Fixed);
        } else {
            topSpacer_->changeSize(topSize, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            bottomSpacer_->changeSize(bottomSize, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        }

        layout_->invalidate();
    }

private:
    int itemCount_ = 0;
    std::function<Component(int)> factory_;
    Qt::Orientation orientation_ = Qt::Vertical;
    int estimatedItemSize_ = 48;

    QWidget* content_ = nullptr;
    QBoxLayout* layout_ = nullptr;
    QSpacerItem* topSpacer_ = nullptr;
    QSpacerItem* bottomSpacer_ = nullptr;
    QScrollBar* overlayScrollBar_ = nullptr;
    QTimer* overlayHideTimer_ = nullptr;
    bool syncingOverlay_ = false;

    std::unordered_map<int, QWidget*> visibleItems_;
};

} // namespace

Component LazyColumn(
    int itemCount,
    std::function<Component(int)> factory,
    Modifier modifier
) {
    auto widget = std::make_unique<LazyListWidget>(itemCount, std::move(factory), Qt::Vertical);
    applyModifier(widget.get(), modifier);
    return widget;
}

Component LazyRow(
    int itemCount,
    std::function<Component(int)> factory,
    Modifier modifier
) {
    auto widget = std::make_unique<LazyListWidget>(itemCount, std::move(factory), Qt::Horizontal);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
