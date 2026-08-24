#include "lazy_list.hpp"

#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QSizePolicy>
#include <QPointer>
#include <QShowEvent>
#include <QResizeEvent>
#include <QEasingCurve>

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

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

// 主轴向两端预加载的像素缓冲，保证 item 在进入视口前完成测量与布局。
constexpr int kScrollBufferPx = 240;

// Fenwick 树（树状数组）：维护每个 item 的主轴向尺寸，支持前缀和 / 点更新，
// 从而以 O(log n) 得到任意 index 的偏移量并反向定位像素位置所属的 item。
class Fenwick {
public:
    Fenwick() = default;

    explicit Fenwick(const std::vector<int>& values) {
        build(values);
    }

    void build(const std::vector<int>& values) {
        n_ = static_cast<int>(values.size());
        tree_.assign(n_ + 1, 0);
        for (int i = 0; i < n_; i++) {
            tree_[i + 1] += values[i];
        }
        for (int i = 1; i <= n_; i++) {
            int p = i + (i & -i);
            if (p <= n_) {
                tree_[p] += tree_[i];
            }
        }
    }

    void add(int index, long long delta) {
        for (int k = index + 1; k <= n_; k += k & -k) {
            tree_[k] += delta;
        }
    }

    long long prefixSum(int count) const {
        long long s = 0;
        for (int k = count; k > 0; k -= k & -k) {
            s += tree_[k];
        }
        return s;
    }

    long long total() const {
        return prefixSum(n_);
    }

    // 返回满足 prefixSum(k) <= v 的最大 k（0..n），即偏移量不超过 v 的最靠后的 item 索引。
    int findLastPrefixLeq(long long v) const {
        int idx = 0;
        int bit = highestPowerOfTwo();
        for (; bit > 0; bit >>= 1) {
            int next = idx + bit;
            if (next <= n_ && tree_[next] <= v) {
                v -= tree_[next];
                idx = next;
            }
        }
        return idx;
    }

private:
    int highestPowerOfTwo() const {
        int bit = 1;
        while (bit <= n_) {
            bit <<= 1;
        }
        return bit >> 1;
    }

    int n_ = 0;
    std::vector<long long> tree_;
};

class LazyListWidget : public QScrollArea {
public:
    LazyListWidget(
        int itemCount,
        std::function<Component(int)> factory,
        Qt::Orientation orientation,
        std::function<int(int)> key,
        std::shared_ptr<LazyListState> state
    )
        : QScrollArea(nullptr)
        , itemCount_(itemCount)
        , factory_(std::move(factory))
        , orientation_(orientation)
        , key_(std::move(key))
        , state_(std::move(state))
    {
        setWidgetResizable(false);
        setFrameShape(QFrame::NoFrame);

        utils::setComponentType(this, orientation == Qt::Vertical ? kUILazyColumnProperty : kUILazyRowProperty);

        if (orientation == Qt::Vertical) {
            estimatedItemSize_ = 48;
        } else {
            estimatedItemSize_ = 100;
        }

        extent_.assign(itemCount_, estimatedItemSize_);
        measured_.assign(itemCount_, 0);
        fenwick_.build(extent_);

        content_ = new QWidget();
        setWidget(content_);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        initOverlayScrollBar();

        if (state_) {
            QPointer<QWidget> self = this;
            state_->setScrollRequest([self](int index, int offsetPx, bool animate) {
                auto* w = static_cast<LazyListWidget*>(self.data());
                if (w) {
                    w->requestScrollToItem(index, offsetPx, animate);
                }
            });
        }

        if (orientation == Qt::Vertical) {
            connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                relayout();
                syncOverlayScrollBar();
                showOverlayScrollBar();
            });
            connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this]() {
                syncOverlayScrollBar();
            });
        } else {
            connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                relayout();
                syncOverlayScrollBar();
                showOverlayScrollBar();
            });
            connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() {
                syncOverlayScrollBar();
            });
        }
    }

    ~LazyListWidget() override {
        if (state_) {
            state_->setScrollRequest(nullptr);
        }
    }

protected:
    // 报告完整虚拟内容尺寸，使滚动区域在父布局中请求足够的空间，
    // 配合 fillMaxHeight/fillMaxWidth 撑满剩余区域（与 QScrollArea 默认语义一致）。
    QSize sizeHint() const override {
        long long main = fenwick_.total();
        int cross = crossSize();
        if (cross <= 0) {
            cross = estimatedItemSize_;
        }
        if (orientation_ == Qt::Vertical) {
            return QSize(cross, static_cast<int>(main));
        }
        return QSize(static_cast<int>(main), cross);
    }

    void showEvent(QShowEvent* event) override {
        QScrollArea::showEvent(event);
        relayout();
        syncOverlayScrollBar();
    }

    void resizeEvent(QResizeEvent* event) override {
        QScrollArea::resizeEvent(event);
        resetMeasurements();
        relayout();
        positionOverlayScrollBar();
        syncOverlayScrollBar();
    }

    void wheelEvent(QWheelEvent* event) override {
        QScrollBar* bar = nativeScrollBar();
        if (!bar || bar->maximum() <= bar->minimum()) {
            QScrollArea::wheelEvent(event);
            return;
        }

        int delta = 0;
        if (orientation_ == Qt::Vertical) {
            delta = event->pixelDelta().y();
            if (delta == 0) {
                delta = event->angleDelta().y() / 120 * bar->singleStep() * 3;
            }
        } else {
            delta = event->pixelDelta().x();
            if (delta == 0) {
                int angle = event->angleDelta().x();
                if (angle == 0) {
                    angle = event->angleDelta().y();
                }
                delta = angle / 120 * bar->singleStep() * 3;
            }
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

    int scrollValue() const {
        QScrollBar* bar = nativeScrollBar();
        return bar ? bar->value() : 0;
    }

    int viewportMainSize() const {
        return orientation_ == Qt::Vertical ? viewport()->height() : viewport()->width();
    }

    int crossSize() const {
        return orientation_ == Qt::Vertical ? viewport()->width() : viewport()->height();
    }

    int keyFor(int index) const {
        return key_ ? key_(index) : index;
    }

    int measureMainExtent(QWidget* w) {
        int cross = crossSize();
        if (orientation_ == Qt::Vertical) {
            int h = 0;
            bool expand = w->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag;
            if (expand && cross > 0 && w->hasHeightForWidth()) {
                h = w->heightForWidth(cross);
            }
            if (h <= 0) {
                h = w->sizeHint().height();
            }
            h = qBound(w->minimumHeight(), h, w->maximumHeight());
            return h > 0 ? h : estimatedItemSize_;
        }
        int wd = w->sizeHint().width();
        wd = qBound(w->minimumWidth(), wd, w->maximumWidth());
        return wd > 0 ? wd : estimatedItemSize_;
    }

    void placeItem(QWidget* w, int offset, int extent) {
        int cross = crossSize();

        QVariant av = w->property(kAlignmentProperty);
        bool hasAlignment = av.isValid();
        Qt::Alignment alignment = hasAlignment
            ? toQtAlignment(static_cast<Alignment>(av.toInt()))
            : Qt::AlignLeft | Qt::AlignTop;

        if (orientation_ == Qt::Vertical) {
            bool expand = w->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag;
            if (expand || !hasAlignment) {
                // 默认与 QVBoxLayout 一致：交叉轴撑满。
                w->setGeometry(0, offset, cross, extent);
            } else {
                int width = std::min(std::max(w->sizeHint().width(), 0), cross);
                int x = 0;
                if (alignment & Qt::AlignRight) {
                    x = cross - width;
                } else if (alignment & Qt::AlignHCenter) {
                    x = (cross - width) / 2;
                }
                w->setGeometry(x, offset, std::max(width, 0), extent);
            }
        } else {
            bool expand = w->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag;
            if (expand || !hasAlignment) {
                w->setGeometry(offset, 0, extent, cross);
            } else {
                int height = std::min(std::max(w->sizeHint().height(), 0), cross);
                int y = 0;
                if (alignment & Qt::AlignBottom) {
                    y = cross - height;
                } else if (alignment & Qt::AlignVCenter) {
                    y = (cross - height) / 2;
                }
                w->setGeometry(offset, y, extent, std::max(height, 0));
            }
        }
    }

    QWidget* acquireWidget(int index) {
        int k = keyFor(index);
        auto it = pool_.find(k);
        QWidget* w = nullptr;
        if (it != pool_.end() && !it->second.empty()) {
            w = it->second.back();
            it->second.pop_back();
            if (it->second.empty()) {
                pool_.erase(it);
            }
            w->setParent(content_);
        } else {
            Component c = factory_(index);
            if (!c) {
                return nullptr;
            }
            w = c.release();
            w->setParent(content_);
        }
        w->show();
        return w;
    }

    void recycleWidget(int index, QWidget* w) {
        w->hide();
        w->setParent(nullptr);
        pool_[keyFor(index)].push_back(w);
    }

    void resetMeasurements() {
        std::fill(extent_.begin(), extent_.end(), estimatedItemSize_);
        std::fill(measured_.begin(), measured_.end(), 0);
        fenwick_.build(extent_);
    }

    void requestScrollToItem(int index, int offsetPx, bool animate) {
        if (itemCount_ <= 0) {
            return;
        }
        index = std::clamp(index, 0, itemCount_ - 1);
        long long target = fenwick_.prefixSum(index) + offsetPx;

        QScrollBar* bar = nativeScrollBar();
        int maxScroll = bar ? std::max(0, bar->maximum()) : 0;
        int clamped = static_cast<int>(std::clamp<long long>(target, 0, maxScroll));

        if (animate) {
            auto* anim = new QPropertyAnimation(bar, "value", this);
            anim->setDuration(300);
            anim->setStartValue(bar->value());
            anim->setEndValue(clamped);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            bar->setValue(clamped);
        }
    }

    void relayout() {
        if (itemCount_ <= 0) {
            content_->setFixedSize(0, 0);
            return;
        }

        int viewportMain = viewportMainSize();
        if (viewportMain <= 0) {
            return;
        }

        int p0 = scrollValue();
        int p1 = p0 + viewportMain;

        int first = fenwick_.findLastPrefixLeq(static_cast<long long>(p0) - kScrollBufferPx);
        int last = fenwick_.findLastPrefixLeq(static_cast<long long>(p1) + kScrollBufferPx);
        first = std::clamp(first, 0, itemCount_ - 1);
        last = std::clamp(last, first, itemCount_ - 1);

        for (auto it = visible_.begin(); it != visible_.end(); ) {
            int idx = it->first;
            if (idx < first || idx > last) {
                recycleWidget(idx, it->second);
                it = visible_.erase(it);
            } else {
                ++it;
            }
        }

        for (int i = first; i <= last; i++) {
            auto found = visible_.find(i);
            QWidget* w = nullptr;
            if (found != visible_.end()) {
                w = found->second;
            } else {
                w = acquireWidget(i);
                if (!w) {
                    continue;
                }
                visible_[i] = w;
            }

            if (!measured_[i]) {
                int ext = measureMainExtent(w);
                if (ext <= 0) {
                    ext = estimatedItemSize_;
                }
                long long old = extent_[i];
                extent_[i] = ext;
                fenwick_.add(i, static_cast<long long>(ext) - old);
                measured_[i] = 1;
            }

            int offset = static_cast<int>(fenwick_.prefixSum(i));
            placeItem(w, offset, extent_[i]);
        }

        int cross = crossSize();
        long long total = fenwick_.total();
        if (orientation_ == Qt::Vertical) {
            content_->setFixedSize(std::max(cross, 0), static_cast<int>(total));
        } else {
            content_->setFixedSize(static_cast<int>(total), std::max(cross, 0));
        }

        if (state_) {
            int firstVisible = fenwick_.findLastPrefixLeq(static_cast<long long>(p0));
            state_->setFirstVisibleIndex(std::clamp(firstVisible, 0, itemCount_ - 1));
        }
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

private:
    int itemCount_ = 0;
    std::function<Component(int)> factory_;
    Qt::Orientation orientation_ = Qt::Vertical;
    std::function<int(int)> key_;
    std::shared_ptr<LazyListState> state_;
    int estimatedItemSize_ = 48;

    std::vector<int> extent_;
    std::vector<char> measured_;
    Fenwick fenwick_;

    QWidget* content_ = nullptr;
    QScrollBar* overlayScrollBar_ = nullptr;
    QTimer* overlayHideTimer_ = nullptr;
    bool syncingOverlay_ = false;

    std::unordered_map<int, QWidget*> visible_;
    std::unordered_map<int, std::vector<QWidget*>> pool_;
};

} // namespace

LazyListState::LazyListState()
    : firstVisibleIndex_(mutableStateOf(0)) {}

LazyListState::~LazyListState() = default;

std::shared_ptr<MutableState<int>> LazyListState::firstVisibleIndex() {
    return firstVisibleIndex_;
}

void LazyListState::scrollToItem(int index, int offsetPx) {
    if (scrollRequest_) {
        scrollRequest_(index, offsetPx, false);
    }
}

void LazyListState::animateScrollToItem(int index, int offsetPx) {
    if (scrollRequest_) {
        scrollRequest_(index, offsetPx, true);
    }
}

void LazyListState::setScrollRequest(ScrollRequest request) {
    scrollRequest_ = std::move(request);
}

void LazyListState::setFirstVisibleIndex(int index) {
    firstVisibleIndex_->set(index);
}

Component LazyColumn(
    int itemCount,
    std::function<Component(int)> factory,
    Modifier modifier,
    std::function<int(int)> key,
    std::shared_ptr<LazyListState> state
) {
    auto widget = std::make_unique<LazyListWidget>(
        itemCount, std::move(factory), Qt::Vertical, std::move(key), std::move(state));
    applyModifier(widget.get(), modifier);
    return widget;
}

Component LazyRow(
    int itemCount,
    std::function<Component(int)> factory,
    Modifier modifier,
    std::function<int(int)> key,
    std::shared_ptr<LazyListState> state
) {
    auto widget = std::make_unique<LazyListWidget>(
        itemCount, std::move(factory), Qt::Horizontal, std::move(key), std::move(state));
    applyModifier(widget.get(), modifier);
    return widget;
}

} // namespace rui
