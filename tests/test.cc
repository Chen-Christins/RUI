// main.cpp
#include "core/platform_adapter.hpp"
#include "core/nav_controller.hpp"
#include "core/composable.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "ui/column.hpp"
#include "ui/color.hpp"
#include "ui/row.hpp"
#include "ui/spacer.hpp"
#include "ui/box.hpp"

#include "utils/logger.hpp"

using namespace rui;

static auto logger = LOG_NAME("App");

// 独立设置窗口
std::unique_ptr<QWidget> SettingsWindow() {
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
            Modifier().background(Color::green(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(14_dp, 6_dp)
        ),
        Text(statusState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
                       .align(Alignment::Center))
    );
}

// 详情界面
std::unique_ptr<QWidget> DetailScreen(std::shared_ptr<NavController> nav) {
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
            Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(16_dp, 8_dp)
        ),
        Text(canPopState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
                       .align(Alignment::Center))
    );
}

// 主页界面
std::unique_ptr<QWidget> HomeScreen(std::shared_ptr<NavController> nav) {
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
            Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(16_dp, 8_dp)
        ),
        // 按钮：Replace 模式 - 替换当前界面
        Button(
            [nav]() {
                nav->replace(DetailScreen(nav));
            },
            mutableStateOf(std::string("Replace Detail (替换当前)")),
            Modifier().background(Color::darkBlue(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(16_dp, 8_dp)
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
            Modifier().background(Color::lightBlue(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(16_dp, 8_dp)
        ),
        // 原有计数器演示
        Button(
            onIncrement,
            Text(mutableStateOf(std::string("Click me")), Modifier().foreground(Color::red())),
            Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                      .padding(12_dp, 4_dp)
        ),
        Row(
            Spacer(Modifier().height(8_dp)
                              .fillMaxWidth()
                              .weight(1)
                              .background(Color::green(), RoundedCornerShape(4_dp))
                              .clip(RoundedCornerShape(4_dp))),
            Spacer(Modifier().width(8_dp)),
            Spacer(Modifier().height(8_dp)
                              .fillMaxWidth()
                              .weight(2)
                              .background(Color::orange(), RoundedCornerShape(4_dp))
                              .clip(RoundedCornerShape(4_dp)))
        ),
        Text(mutableStateOf(std::string("Text clickable")),
             Modifier().padding(8_dp)
                       .background(Color::grey200(), RoundedCornerShape(6_dp))
                       .clickable(onIncrement)),
        Text(counterText,
             Modifier().foreground(Color::grey600())
                       .align(Alignment::Center)),
        // Box 演示：层叠布局
        Box(
            Modifier().fillMaxWidth().height(80_dp).background(Color::grey100(), RoundedCornerShape(6_dp)),
            Text(mutableStateOf(std::string("Behind")),
                 Modifier().fontSize(24_dp)
                           .align(Alignment::Center)
                           .foreground(Color::grey300())),
            Text(mutableStateOf(std::string("Front")),
                 Modifier().fontSize(16_dp)
                           .align(Alignment::BottomEnd)
                           .foreground(Color::blue())
                           .padding(8_dp))
        ),
        // compose() 演示：自动重组的组件
        // 每次 counter 变化时，整个区块会重建，remember 的值在重组间保持
        compose([counter]() -> std::unique_ptr<QWidget> {
            observe(counter);
            auto& recomposeCount = remember(-1);
            recomposeCount++;

            return Column(
                Text(mutableStateOf(std::string(
                    "Recompose #" + std::to_string(recomposeCount))),
                     Modifier().foreground(Color::grey500())
                               .fontSize(11_dp)
                               .align(Alignment::Center)),
                Text(mutableStateOf(std::string(
                    "Counter = " + std::to_string(counter->get()))),
                     Modifier().foreground(Color::grey500())
                               .fontSize(11_dp)
                               .align(Alignment::Center))
            );
        })
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
