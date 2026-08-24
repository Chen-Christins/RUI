// text.cc
#include "text.hpp"

#include <QLabel>
#include <QSizePolicy>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

Component Text(
    std::shared_ptr<MutableState<std::string>> text,
    Modifier modifier
) {
    auto label = std::make_unique<QLabel>();
    utils::setComponentType(label.get(), kUITextProperty);

    // 不随布局拉伸，紧贴内容大小
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 初始显示
    label->setText(QString::fromStdString(text->get()));

    // 订阅状态变化，实时更新文本
    text->observe(label.get(), [rawLabel = label.get()](const std::string& newText) {
        rawLabel->setText(QString::fromStdString(newText));
    });

    applyModifier(label.get(), modifier);

    return label;
}

} // namespace rui
