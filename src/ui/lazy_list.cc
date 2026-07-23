#include "lazy_list.hpp"

#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QBoxLayout>
#include <QSpacerItem>

#include <unordered_map>

#include "component_type.hpp"
#include "../utils/util.hpp"

namespace rui {

namespace {

class LazyListWidget : public QScrollArea {
public:
    LazyListWidget(
        int itemCount,
        std::function<std::unique_ptr<QWidget>(int)> factory,
        Qt::Orientation orientation
    )
        : QScrollArea(nullptr)
        , itemCount_(itemCount)
        , factory_(std::move(factory))
        , orientation_(orientation)
    {
        setWidgetResizable(true);
        utils::setComponentType(this, orientation == Qt::Vertical ? "LazyColumn" : "LazyRow");

        content_ = new QWidget(this);
        if (orientation == Qt::Vertical) {
            auto* vl = new QVBoxLayout(content_);
            vl->setContentsMargins(0, 0, 0, 0);
            vl->setSpacing(0);
            layout_ = vl;
            setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else {
            auto* hl = new QHBoxLayout(content_);
            hl->setContentsMargins(0, 0, 0, 0);
            hl->setSpacing(0);
            layout_ = hl;
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        setWidget(content_);

        // 默认 item 尺寸
        if (orientation == Qt::Vertical) {
            estimatedItemSize_ = 48;
        } else {
            estimatedItemSize_ = 100;
        }

        // 初始化 spacers
        if (orientation == Qt::Vertical) {
            topSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
            bottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        } else {
            topSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            bottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        }
        layout_->addItem(topSpacer_);
        layout_->addItem(bottomSpacer_);

        // 滚动时更新
        if (orientation == Qt::Vertical) {
            connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                updateVisible();
            });
        } else {
            connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                updateVisible();
            });
        }
    }

protected:
    void showEvent(QShowEvent* event) override {
        QScrollArea::showEvent(event);
        updateVisible();
    }

    void resizeEvent(QResizeEvent* event) override {
        QScrollArea::resizeEvent(event);
        updateVisible();
    }

private:
    void updateVisible() {
        if (itemCount_ <= 0 || estimatedItemSize_ <= 0) return;

        int scrollPos = orientation_ == Qt::Vertical
            ? verticalScrollBar()->value()
            : horizontalScrollBar()->value();
        int viewportSize = orientation_ == Qt::Vertical
            ? viewport()->height()
            : viewport()->width();

        if (viewportSize <= 0) return;

        int buffer = 2;
        int firstVisible = std::max(0, scrollPos / estimatedItemSize_ - buffer);
        int lastVisible = std::min(itemCount_ - 1,
            (scrollPos + viewportSize) / estimatedItemSize_ + buffer);

        // 清理不可见 item
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

        // 添加新可见 item
        for (int i = firstVisible; i <= lastVisible; i++) {
            if (visibleItems_.find(i) != visibleItems_.end()) continue;

            auto w = factory_(i);
            if (!w) continue;
            QWidget* raw = w.release();

            int insertPos = 1;
            for (int j = firstVisible; j < i; j++) {
                if (visibleItems_.find(j) != visibleItems_.end()) insertPos++;
            }
            layout_->insertWidget(insertPos, raw);

            if (orientation_ == Qt::Vertical) {
                raw->setFixedHeight(estimatedItemSize_);
            } else {
                raw->setFixedWidth(estimatedItemSize_);
            }
            visibleItems_[i] = raw;
        }

        // 更新 spacers
        int topSize = firstVisible * estimatedItemSize_;
        int bottomSize = (itemCount_ - 1 - lastVisible) * estimatedItemSize_;
        if (bottomSize < 0) bottomSize = 0;

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
    std::function<std::unique_ptr<QWidget>(int)> factory_;
    Qt::Orientation orientation_ = Qt::Vertical;
    int estimatedItemSize_ = 48;

    QWidget* content_ = nullptr;
    QBoxLayout* layout_ = nullptr;
    QSpacerItem* topSpacer_ = nullptr;
    QSpacerItem* bottomSpacer_ = nullptr;

    std::unordered_map<int, QWidget*> visibleItems_;
};

} // namespace

std::unique_ptr<QWidget> LazyColumn(
    int itemCount,
    std::function<std::unique_ptr<QWidget>(int)> factory,
    Modifier modifier
) {
    auto widget = std::make_unique<LazyListWidget>(itemCount, std::move(factory), Qt::Vertical);
    applyModifier(widget.get(), modifier);
    return widget;
}

std::unique_ptr<QWidget> LazyRow(
    int itemCount,
    std::function<std::unique_ptr<QWidget>(int)> factory,
    Modifier modifier
) {
    auto widget = std::make_unique<LazyListWidget>(itemCount, std::move(factory), Qt::Horizontal);
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
