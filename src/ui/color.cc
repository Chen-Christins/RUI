// color.cc
#include "color.hpp"

namespace rui {

Color::Color(const QColor& c)
    : r_(static_cast<uint8_t>(c.red()))
    , g_(static_cast<uint8_t>(c.green()))
    , b_(static_cast<uint8_t>(c.blue()))
    , a_(static_cast<uint8_t>(c.alpha()))
{
}

QColor Color::toQColor() const
{
    return QColor(r_, g_, b_, a_);
}

Color Color::fromHex(const std::string& hex)
{
    QString h = QString::fromStdString(hex).remove('#').remove(' ');
    if (h.length() == 3) {
        h = QString("%1%1%2%2%3%3").arg(h[0], h[1], h[2]);
    } else if (h.length() == 4) {
        h = QString("%1%1%2%2%3%3%4%4").arg(h[0], h[1], h[2], h[3]);
    }
    bool ok;
    uint32_t val = h.toUInt(&ok, 16);
    if (!ok) {
        return Color();
    }
    if (h.length() == 8) {
        return Color((val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
    }
    return Color((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
}

} // namespace rui
