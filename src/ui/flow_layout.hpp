// flow_layout.hpp
#pragma once

#include <QLayout>
#include <QWidget>

#include <vector>

namespace rui {

// FlowLayout - arranges items in rows (horizontal) or columns (vertical), wrapping when the
// available space in the main direction is exhausted.
class FlowLayout : public QLayout {
public:
    explicit FlowLayout(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

private:
    int doLayout(const QRect& rect, bool testOnly) const;
    int spacingX() const;
    int spacingY() const;

    Qt::Orientation orientation_;
    std::vector<QLayoutItem*> items_;
};

// FlowWidget - QWidget wrapper that reports height-for-width so that wrapping
// flow layouts are sized correctly inside box layouts.
class FlowWidget : public QWidget {
public:
    explicit FlowWidget(Qt::Orientation orientation, QWidget* parent = nullptr);

    FlowLayout* flowLayout() const { return layout_; }

    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

private:
    Qt::Orientation orientation_;
    FlowLayout* layout_ = nullptr;
};

} // namespace rui
