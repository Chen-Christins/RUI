// box.cc
#include "box.hpp"

#include <QResizeEvent>

#include <algorithm>
#include <vector>

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

    QVariant ox = child->property(kOffsetXProperty);
    QVariant oy = child->property(kOffsetYProperty);
    if (ox.isValid()) {
        x += ox.toInt();
    }
    if (oy.isValid()) {
        y += oy.toInt();
    }

    child->setGeometry(x, y, w, h);
}

void applyZOrder(QWidget* box) {
    auto children = box->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    std::vector<QWidget*> sorted(children.begin(), children.end());
    std::stable_sort(sorted.begin(), sorted.end(), [](QWidget* a, QWidget* b) {
        int za = a->property(kZIndexProperty).isValid() ? a->property(kZIndexProperty).toInt() : 0;
        int zb = b->property(kZIndexProperty).isValid() ? b->property(kZIndexProperty).toInt() : 0;
        return za < zb;
    });
    for (auto* w : sorted) {
        w->raise();
    }
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
