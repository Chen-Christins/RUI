// main.cpp
#include "platform_adapter.hpp"
#include "nav_controller.hpp"
#include "text.hpp"
#include "button.hpp"
#include "column.hpp"
#include "color.hpp"

#include "utils/logger.hpp"

using namespace rui;

static auto logger = Logger::getLogger("App");

// 独立设置窗口
Composable std::unique_ptr<QWidget> SettingsWindow() {
    auto titleState = mutableStateOf(std::string("Settings"));
    auto statusState = mutableStateOf(std::string(""));

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(18)
                       .align(Alignment::Center)),
        Button(
            [statusState]() {
                statusState->set("Saved!");
            },
            mutableStateOf(std::string("Save Settings")),
            Modifier().background(Color::green())
                      .foreground(Color::white())
                      .padding(6, 14, 6, 14)
        ),
        Text(statusState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12)
                       .align(Alignment::Center))
    );
}

// 详情界面
Composable std::unique_ptr<QWidget> DetailScreen(std::shared_ptr<NavController> nav) {
    auto titleState = mutableStateOf(std::string("Detail Screen"));
    auto canPopState = mutableStateOf(std::string(
        nav->canPop() ? "← Go Back (pop)" : "Cannot go back"
    ));

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(20)
                       .align(Alignment::Center)),
        Button(
            [nav]() {
                nav->pop();
            },
            mutableStateOf(std::string("Go Back")),
            Modifier().background(Color::blue())
                      .foreground(Color::white())
                      .padding(8, 16, 8, 16)
        ),
        Text(canPopState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12)
                       .align(Alignment::Center))
    );
}

// 主页界面
Composable std::unique_ptr<QWidget> HomeScreen(std::shared_ptr<NavController> nav) {
    auto titleState = mutableStateOf(std::string("Home Screen"));
    auto counter    = mutableStateOf("counter", 0);
    auto counterText = mutableStateOf("countText", std::string("Count: 0"));

    auto onIncrement = [counter, counterText]() {
        counter->set(counter->get() + 1);
        counterText->set("Count: " + std::to_string(counter->get()));
        INFO(logger) << "Counter: " << counter->get();
    };

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(20)
                       .align(Alignment::Center)),
        // 按钮：Push 模式 - 保留当前界面
        Button(
            [nav]() {
                nav->push(DetailScreen(nav));
            },
            mutableStateOf(std::string("Push Detail (保留当前)")),
            Modifier().background(Color::blue())
                      .foreground(Color::white())
                      .padding(8, 16, 8, 16)
        ),
        // 按钮：Replace 模式 - 替换当前界面
        Button(
            [nav]() {
                nav->replace(DetailScreen(nav));
            },
            mutableStateOf(std::string("Replace Detail (替换当前)")),
            Modifier().background(Color::darkBlue())
                      .foreground(Color::white())
                      .padding(8, 16, 8, 16)
        ),
        // 按钮：打开独立窗口
        Button(
            [nav]() {
                nav->openWindow(SettingsWindow(),
                                WindowConfig{
                                    .title = "Settings",
                                    .key = "settings",
                                    .width = 300,
                                    .height = 200,
                                    .minWidth = 200,
                                    .minHeight = 150
                                }
                );
            },
            mutableStateOf(std::string("Open Settings Window")),
            Modifier().background(Color::lightBlue())
                      .foreground(Color::white())
                      .padding(8, 16, 8, 16)
        ),
        // 原有计数器演示
        Button(
            onIncrement,
            Text(mutableStateOf(std::string("Click me")), Modifier().foreground(Color::red())),
            Modifier().background(Color::blue())
                      .padding(4, 12, 4, 12)
        ),
        Text(counterText,
             Modifier().foreground(Color::grey600())
                       .align(Alignment::Center))
    );
}

int main(int argc, char* argv[]) {
    auto platform = PlatformAdapter::create();
    platform->initialize(argc, argv);

    platform->setWindowConfig(WindowConfig{
        .title = "RUI Navigation Demo",
        .width = 350,
        .height = 400,
        .minWidth = 300,
        .minHeight = 300
    });
    platform->enableDevTools();

    platform->run([]() {
        // 创建导航控制器并设置初始界面
        auto nav = std::make_shared<NavController>();
        nav->push(HomeScreen(nav));

        auto* layout = PlatformAdapter::currentLayout();
        if (layout) {
            layout->addWidget(nav->container());
        }
    });

    return 0;
}
