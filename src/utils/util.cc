#include "util.hpp"

#include "../ui/component_type.hpp"
#include "../ui/dimension.hpp"

namespace rui::utils {

void setComponentType(QWidget* widget, const char* type) {
    widget->setProperty(kComponentTypeProperty, type);
}

QString widgetQtType(QWidget* widget) {
    return QString::fromUtf8(widget->metaObject()->className());
}

QString widgetComponentType(QWidget* widget) {
    QVariant componentType = widget->property(kComponentTypeProperty);
    if (componentType.isValid() && !componentType.toString().isEmpty()) {
        return componentType.toString();
    }
    return widgetQtType(widget).remove("rui::");
}

QJsonObject colorToJson(const QColor& c) {
    QJsonObject o;
    o["r"] = c.red();
    o["g"] = c.green();
    o["b"] = c.blue();
    o["a"] = c.alpha();
    return o;
}

QJsonObject dpDimension(int value, const QWidget* context) {
    QJsonObject obj;
    obj["value"] = value;
    obj["unit"] = "dp";
    obj["px"] = dp2px(value, context);
    obj["dpr"] = devicePixelRatio(context);
    return obj;
}

QJsonObject pxDimension(int value, const QWidget* context) {
    QJsonObject obj;
    obj["value"] = value;
    obj["unit"] = "px";
    obj["dp"] = px2dp(value, context);
    obj["dpr"] = devicePixelRatio(context);
    return obj;
}

} // namespace rui::utils