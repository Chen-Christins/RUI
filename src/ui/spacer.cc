#include "spacer.hpp"

#include <QSizePolicy>

namespace rui {

Composable std::unique_ptr<QWidget> Spacer(Modifier modifier) {
    auto spacer = std::make_unique<QWidget>();
    detail::setComponentType(spacer.get(), "Spacer");

    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setAttribute(Qt::WA_TransparentForMouseEvents);

    applyModifier(spacer.get(), modifier);

    return spacer;
}

} // namespace rui
