// example.cc
#include <core/nav_controller.hpp>
#include <core/platform_adapter.hpp>
#include <ui/box.hpp>
#include <ui/component.hpp>
#include <utils/logger.hpp>

#include "ui/row.hpp"
#include "ui/column.hpp"

using namespace rui;

static Logger::ptr g_logger = LOG_NAME("example");

Component HomeScreen(std::shared_ptr<NavController> nav) {
    return Box(
        Modifier().fillMaxSize().background(Color::white()),
        Row(

        )
    );
}

int main(int argc, char** argv) {
    auto platform = PlatformAdapter::create();
    platform->initialize(argc, argv);

    platform->setWindowConfig({
        .title = "RUI Example",
        .width = 900_dp,
        .height = 600_dp,
        .minWidth = 600_dp,
        .minHeight = 400_dp,
    });

    platform->run([] {
        auto nav = std::make_shared<NavController>();
        nav->push(HomeScreen(nav));

        auto layout = PlatformAdapter::currentLayout();
        if (layout) {
            layout->addWidget(nav->container());
        }
    });

    return 0;
}