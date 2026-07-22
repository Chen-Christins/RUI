// box.cc
#include "box.hpp"

#include <QResizeEvent>

#include "modifier.hpp"

namespace rui {

void layoutBoxChild(QWidget* child) {
    auto* parent = child->parentWidget();
    if (!parent) {
        return;
    }

    auto pref = child->sizeHint();
    auto parentSize = parent->size();

    int w = pref.width();
    int h = pref.height();

    QVariant fw = child->property(kFillMaxWidthProperty);
    QVariant fh = child->property(kFillMaxHeightProperty);
    if (fw.isValid() && fw.toBool()) {
        w = parentSize.width();
    }
    if (fh.isValid() && fh.toBool()) {
        h = parentSize.height();
    }

    QVariant av = child->property(kAlignmentProperty);
    Alignment alignment = Alignment::TopStart;
    if (av.isValid()) {
        alignment = static_cast<Alignment>(av.toInt());
    }

    int x = 0, y = 0;
    auto qtAlign = toQtAlignment(alignment);
    if (qtAlign & Qt::AlignHCenter) {
        x = (parentSize.width() - w) / 2;
    } else if (qtAlign & Qt::AlignRight) {
        x = parentSize.width() - w;
    }

    if (qtAlign & Qt::AlignVCenter) {
        y = (parentSize.height() - h) / 2;
    } else if (qtAlign & Qt::AlignBottom) {
        y = parentSize.height() - h;
    }

    child->setGeometry(x, y, w, h);
}

QSize BoxWidget::sizeHint() const {
    QSize maxSize;
    const auto children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* child : children) {
        auto hint = child->sizeHint();
        maxSize = maxSize.expandedTo(hint);
    }
    return maxSize;
}

void BoxWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const auto children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* child : children) {
        layoutBoxChild(child);
    }
}

} // namespace rui
