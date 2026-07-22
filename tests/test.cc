// main.cpp
#include "core/platform_adapter.hpp"
#include "core/nav_controller.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "ui/column.hpp"
#include "ui/color.hpp"
#include "ui/spacer.hpp"

#include "utils/logger.hpp"

using namespace rui;

static auto logger = LOG_NAME("App");

// 独立设置窗口
Composable std::unique_ptr<QWidget> SettingsWindow() {
    auto titleState = mutableStateOf(std::string("Settings"));
    auto statusState = mutableStateOf(std::string(""));

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(18_dp)
                       .align(Alignment::Center)),
        Spacer(Modifier().size(0_dp, 12_dp).background(Color::orange())),
        Button(
            [statusState]() {
                statusState->set("Saved!");
            },
            mutableStateOf(std::string("Save Settings")),
            Modifier().background(Color::green())
                      .foreground(Color::white())
                      .padding(6_dp, 14_dp, 6_dp, 14_dp)
        ),
        Text(statusState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
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
                       .fontSize(20_dp)
                       .align(Alignment::Center)),
        Button(
            [nav]() {
                nav->pop();
            },
            mutableStateOf(std::string("Go Back")),
            Modifier().background(Color::blue())
                      .foreground(Color::white())
                      .padding(8_dp, 16_dp, 8_dp, 16_dp)
        ),
        Text(canPopState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
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
                       .fontSize(20_dp)
                       .align(Alignment::Center)),
        // 按钮：Push 模式 - 保留当前界面
        Button(
            [nav]() {
                nav->push(DetailScreen(nav));
            },
            mutableStateOf(std::string("Push Detail (保留当前)")),
            Modifier().background(Color::blue())
                      .foreground(Color::white())
                      .padding(8_dp, 16_dp, 8_dp, 16_dp)
        ),
        // 按钮：Replace 模式 - 替换当前界面
        Button(
            [nav]() {
                nav->replace(DetailScreen(nav));
            },
            mutableStateOf(std::string("Replace Detail (替换当前)")),
            Modifier().background(Color::darkBlue())
                      .foreground(Color::white())
                      .padding(8_dp, 16_dp, 8_dp, 16_dp)
        ),
        // 按钮：打开独立窗口
        Button(
            [nav]() {
                nav->openWindow(SettingsWindow(),
                                WindowConfig{
                                    .title = "Settings",
                                    .key = "settings",
                                    .width = 300_dp,
                                    .height = 200_dp,
                                    .minWidth = 200_dp,
                                    .minHeight = 150_dp
                                }
                );
            },
            mutableStateOf(std::string("Open Settings Window")),
            Modifier().background(Color::lightBlue())
                      .foreground(Color::white())
                      .padding(8_dp, 16_dp, 8_dp, 16_dp)
        ),
        // 原有计数器演示
        Button(
            onIncrement,
            Text(mutableStateOf(std::string("Click me")), Modifier().foreground(Color::red())),
            Modifier().background(Color::blue())
                      .padding(4_dp, 12_dp, 4_dp, 12_dp)
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
        .width = 350_dp,
        .height = 400_dp,
        .minWidth = 300_dp,
        .minHeight = 300_dp
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
