// flow_layout.cc
#include "flow_layout.hpp"

#include <QWidget>
#include <algorithm>

namespace rui {

FlowLayout::FlowLayout(Qt::Orientation orientation, QWidget* parent)
    : QLayout(parent)
    , orientation_(orientation)
{
    setContentsMargins(0, 0, 0, 0);
    setSpacing(0);
}

FlowLayout::~FlowLayout()
{
    while (count() > 0) {
        delete takeAt(0);
    }
}

void FlowLayout::addItem(QLayoutItem* item)
{
    items_.push_back(item);
}

int FlowLayout::count() const
{
    return static_cast<int>(items_.size());
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return nullptr;
    }
    return items_[static_cast<size_t>(index)];
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return nullptr;
    }
    auto it = items_.begin() + index;
    QLayoutItem* item = *it;
    items_.erase(it);
    return item;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return orientation_ == Qt::Horizontal;
}

int FlowLayout::heightForWidth(int width) const
{
    if (orientation_ != Qt::Horizontal) {
        return -1;
    }
    return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    // Natural size: all items on a single line (horizontal) or column (vertical).
    int totalMain = 0;
    int maxCross = 0;
    for (const QLayoutItem* item : items_) {
        const QSize hint = item->sizeHint();
        if (orientation_ == Qt::Horizontal) {
            totalMain += hint.width();
            maxCross = std::max(maxCross, hint.height());
        } else {
            totalMain += hint.height();
            maxCross = std::max(maxCross, hint.width());
        }
    }
    const int gap = spacingX();
    const int count = static_cast<int>(items_.size());
    if (count > 1) {
        totalMain += gap * (count - 1);
    }
    const QMargins margins = contentsMargins();
    if (orientation_ == Qt::Horizontal) {
        return QSize(totalMain + margins.left() + margins.right(), maxCross + margins.top() + margins.bottom());
    }
    return QSize(maxCross + margins.left() + margins.right(), totalMain + margins.top() + margins.bottom());
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem* item : items_) {
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

FlowWidget::FlowWidget(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
    , orientation_(orientation)
{
    layout_ = new FlowLayout(orientation, this);
    if (orientation == Qt::Horizontal) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    } else {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    }
}

bool FlowWidget::hasHeightForWidth() const
{
    return orientation_ == Qt::Horizontal;
}

int FlowWidget::heightForWidth(int width) const
{
    if (layout_) {
        return layout_->heightForWidth(width);
    }
    return -1;
}

QSize FlowWidget::sizeHint() const
{
    if (layout_) {
        return layout_->sizeHint();
    }
    return QWidget::sizeHint();
}

int FlowLayout::spacingX() const
{
    const int s = spacing();
    return s < 0 ? 0 : s;
}

int FlowLayout::spacingY() const
{
    const int s = spacing();
    return s < 0 ? 0 : s;
}

int FlowLayout::doLayout(const QRect& rect, bool testOnly) const
{
    const QMargins margins = contentsMargins();
    QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    const int hs = spacingX();
    const int vs = spacingY();
    const bool horizontal = orientation_ == Qt::Horizontal;

    int x = effective.x();
    int y = effective.y();
    int lineExtent = 0;

    for (QLayoutItem* item : items_) {
        const QSize hint = item->sizeHint();
        const bool wraps = horizontal ? (x + hint.width() > effective.right() + 1 && lineExtent > 0)
                                      : (y + hint.height() > effective.bottom() + 1 && lineExtent > 0);

        if (wraps) {
            if (horizontal) {
                x = effective.x();
                y += lineExtent + vs;
            } else {
                x += lineExtent + hs;
                y = effective.y();
            }
            lineExtent = 0;
        }

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }

        if (horizontal) {
            x += hint.width() + hs;
            lineExtent = std::max(lineExtent, hint.height());
        } else {
            y += hint.height() + vs;
            lineExtent = std::max(lineExtent, hint.width());
        }
    }

    if (horizontal) {
        return y + lineExtent - rect.y() + margins.bottom();
    }
    return x + lineExtent - rect.x() + margins.right();
}

} // namespace rui
