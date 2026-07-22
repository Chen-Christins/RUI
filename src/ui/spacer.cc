#include "spacer.hpp"

#include <QSizePolicy>

#include "../utils/util.hpp"

namespace rui {

Composable std::unique_ptr<QWidget> Spacer(Modifier modifier) {
    auto spacer = std::make_unique<QWidget>();
    utils::setComponentType(spacer.get(), "Spacer");

    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setAttribute(Qt::WA_TransparentForMouseEvents);

    applyModifier(spacer.get(), modifier);

    return spacer;
}

} // namespace rui
