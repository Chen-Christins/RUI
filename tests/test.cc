// main.cpp
#include "core/platform_adapter.hpp"
#include "core/nav_controller.hpp"
#include "core/composable.hpp"
#include "config/config.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "ui/column.hpp"
#include "ui/color.hpp"
#include "ui/row.hpp"
#include "ui/spacer.hpp"
#include "ui/box.hpp"
#include "ui/image.hpp"
#include "ui/text_field.hpp"
#include "ui/lazy_list.hpp"
#include "ui/scaffold.hpp"

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

// 图片演示界面
std::unique_ptr<QWidget> ImageDemo() {
    std::string path = Config::Instance()->workpath().toStdString();
    std::string img_path = path + "/tests/avatar.jpg";

    std::string svg_path = path + "/tests/avatar.svg";

    return Column(
        Arrangement::Center,
        Text(mutableStateOf(std::string("Image Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(20_dp)),
        // 用户卡片：头像 + 名称/角色
        Row(
            Arrangement::Center,
            Image("https://avatars.githubusercontent.com/u/121867895?v=4",
                  Modifier().size(45_dp, 45_dp)
                            .clip(RoundedCornerShape(23_dp))),
            Spacer(Modifier().width(5_dp)),
            Column(
                Arrangement::SpaceBetween,
                Text(mutableStateOf(std::string("Christins Chen")),
                     Modifier().fontSize(18_dp).foreground(Color::grey800())),
                Text(mutableStateOf(std::string("Software Engineer")),
                     Modifier().fontSize(12_dp).foreground(Color::grey500()))
            )
        ),
        Spacer(Modifier().height(16_dp)),
        // ContentScale 演示
        Row(
            Arrangement::SpaceEvenly,
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::Fit),
                Text(mutableStateOf(std::string("Fit")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::Crop),
                Text(mutableStateOf(std::string("Crop")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::FillBounds),
                Text(mutableStateOf(std::string("FillBounds")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            )
        ),
        Spacer(Modifier().height(12_dp)),
        // alpha + colorFilter 演示
        Row(
            Arrangement::SpaceEvenly,
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).alpha(0.5f)),
                Text(mutableStateOf(std::string("alpha 0.5")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp),
                      Alignment::Center, ContentScale::Crop, "",
                      ColorFilter{Color::blue()}),
                Text(mutableStateOf(std::string("Tint Blue")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            )
        ),
        Spacer(Modifier().height(12_dp)),
        // SVG 图片
        Text(mutableStateOf(std::string("SVG Image")),
             Modifier().fontSize(14_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(4_dp)),
        Image(svg_path,
              Modifier().size(80_dp, 80_dp).background(Color::grey200()))
    );
}

// TextField 演示界面
std::unique_ptr<QWidget> TextFieldDemo() {
    auto nameState = mutableStateOf(std::string("Alice"));
    auto outputState = mutableStateOf(std::string(""));

    return Column(
        Arrangement::Center,
        Text(mutableStateOf(std::string("TextField Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(16_dp)),
        Text(mutableStateOf(std::string("Filled TextField:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        TextField(
            nameState,
            Modifier().fillMaxWidth().padding(16_dp, 4_dp),
            "Enter your name",
            "Name"
        ),
        Spacer(Modifier().height(12_dp)),
        Text(mutableStateOf(std::string("Outlined TextField:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        OutlinedTextField(
            outputState,
            Modifier().fillMaxWidth().padding(16_dp, 4_dp),
            "Type something...",
            "Output"
        ),
        Spacer(Modifier().height(12_dp)),
        Text(mutableStateOf(std::string("Live preview:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        Text(nameState, Modifier().fontSize(16_dp).foreground(Color::darkBlue()))
    );
}

// LazyColumn 演示界面
std::unique_ptr<QWidget> LazyListDemo() {
    auto count = 100;
    return LazyColumn(
        count,
        [](int i) -> std::unique_ptr<QWidget> {
            return Text(
                mutableStateOf(std::string("Item #" + std::to_string(i))),
                Modifier().fillMaxWidth()
                          .padding(12_dp, 8_dp)
                          .foreground(i % 2 == 0 ? Color::grey800() : Color::grey600())
                          .background(i % 2 == 0 ? Color::grey100() : Color::white())
            );
        },
        Modifier().fillMaxWidth().fillMaxHeight()
    );
}

// Scaffold 演示界面
std::unique_ptr<QWidget> ScaffoldDemo() {
    auto titleState = mutableStateOf(std::string("Scaffold Demo"));
    auto counter = mutableStateOf("scaffold_counter", 0);
    auto counterText = mutableStateOf("scaffold_text", std::string("FAB counter: 0"));

    return Scaffold(
        Modifier{},
        TopAppBar(titleState),
        Column(
            Arrangement::Center,
            Spacer(Modifier().height(24_dp)),
            Text(mutableStateOf(std::string("Content Area")),
                 Modifier().fontSize(16_dp).foreground(Color::grey600())
                           .align(Alignment::Center)),
            Text(counterText,
                 Modifier().fontSize(14_dp).foreground(Color::grey500())
                           .align(Alignment::Center)),
            Spacer(Modifier().weight(1))
        ),
        nullptr,
        FloatingActionButton(
            [counter, counterText]() {
                counter->set(counter->get() + 1);
                counterText->set("FAB counter: " + std::to_string(counter->get()));
            },
            Text(mutableStateOf(std::string("+")),
                 Modifier().fontSize(24_dp).foreground(Color::white())
                           .align(Alignment::Center))
        )
    );
}

// 主页界面
std::unique_ptr<QWidget> HomeScreen(std::shared_ptr<NavController> nav) {
    auto counter    = mutableStateOf("counter", 0);
    auto counterText = mutableStateOf("countText", std::string("Count: 0"));

    auto onIncrement = [counter, counterText]() {
        counter->set(counter->get() + 1);
        counterText->set("Count: " + std::to_string(counter->get()));
        INFO(logger) << "Counter: " << counter->get();
    };

    struct Item {
        enum Type { Title, Button, Row, Text, Box, Compose };
        Type type;
        int navIdx = -1;
    };

    std::vector<Item> items = {
        {Item::Title},         // Title
        {Item::Button, 0},     // Push Detail
        {Item::Button, 1},     // Replace Detail
        {Item::Button, 2},     // Image Demo
        {Item::Button, 3},     // TextField Demo
        {Item::Button, 4},     // LazyColumn Demo
        {Item::Button, 7},     // Scaffold Demo
        {Item::Button, 5},     // Open Settings Window
        {Item::Button, 6},     // Click me
        {Item::Row, -1},       // Spacer Row
        {Item::Text, -1},      // Text clickable
        {Item::Text, -2},      // Counter text
        {Item::Box, -1},       // Box demo
        {Item::Compose, -1},   // Compose demo
    };

    return LazyColumn(
        static_cast<int>(items.size()),
        [nav, counter, counterText, onIncrement, items](int i) -> std::unique_ptr<QWidget> {
            switch (items[i].type) {
            case Item::Title:
                return Text(mutableStateOf(std::string("Home Screen")),
                            Modifier().foreground(Color::blue())
                                      .fontSize(20_dp)
                                      .align(Alignment::Center));
            case Item::Button: {
                switch (items[i].navIdx) {
                case 0:
                    return Button(
                        [nav]() { nav->push(DetailScreen(nav)); },
                        mutableStateOf(std::string("Push Detail (保留当前)")),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 1:
                    return Button(
                        [nav]() { nav->replace(DetailScreen(nav)); },
                        mutableStateOf(std::string("Replace Detail (替换当前)")),
                        Modifier().fillMaxWidth()
                                  .background(Color::darkBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 2:
                    return Button(
                        [nav]() { nav->push(ImageDemo()); },
                        mutableStateOf(std::string("Open Image Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::green(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 3:
                    return Button(
                        [nav]() { nav->push(TextFieldDemo()); },
                        mutableStateOf(std::string("Open TextField Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 4:
                    return Button(
                        [nav]() { nav->push(LazyListDemo()); },
                        mutableStateOf(std::string("Open LazyColumn Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::lightBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 5:
                    return Button(
                        [nav]() {
                            nav->openWindow(SettingsWindow(),
                                            WindowConfig{
                                                .title = "Settings",
                                                .key = "settings",
                                                .width = 300_dp,
                                                .height = 200_dp,
                                                .minWidth = 200_dp,
                                                .minHeight = 150_dp
                                            });
                        },
                        mutableStateOf(std::string("Open Settings Window")),
                        Modifier().fillMaxWidth()
                                  .background(Color::lightBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 7:
                    return Button(
                        [nav]() { nav->push(ScaffoldDemo()); },
                        mutableStateOf(std::string("Open Scaffold Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::darkBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 6:
                    return Button(
                        onIncrement,
                        Text(mutableStateOf(std::string("Click me")),
                             Modifier().foreground(Color::red())),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .padding(12_dp, 4_dp)
                    );
                default:
                    return nullptr;
                }
            }
            case Item::Row:
                return Row(
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
                );
            case Item::Text: {
                if (items[i].navIdx == -2) {
                    return Text(counterText,
                         Modifier().foreground(Color::grey600())
                                   .align(Alignment::Center));
                }
                return Text(mutableStateOf(std::string("Text clickable")),
                     Modifier().padding(8_dp)
                               .background(Color::grey200(), RoundedCornerShape(6_dp))
                               .clickable(onIncrement));
            }
            case Item::Box:
                return Box(
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
                );
            case Item::Compose:
                return compose([counter]() -> std::unique_ptr<QWidget> {
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
                });
            }
            return nullptr;
        },
        Modifier().fillMaxWidth().fillMaxHeight()
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
