#include "spacer.hpp"

#include <QSizePolicy>

#include "../utils/util.hpp"
#include "component_type.hpp"

namespace rui {

std::unique_ptr<QWidget> Spacer(Modifier modifier) {
    auto spacer = std::make_unique<QWidget>();
    utils::setComponentType(spacer.get(), kUISpacerProperty);

    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setAttribute(Qt::WA_TransparentForMouseEvents);

    applyModifier(spacer.get(), modifier);

    return spacer;
}

} // namespace rui
