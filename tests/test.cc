// main.cpp
#include "platform_adapter.hpp"
#include "text.hpp"
#include "button.hpp"
#include "column.hpp"
#include "color.hpp"

#include <iostream>

using namespace rui;

// 应用主界面
Composable std::unique_ptr<QWidget> App() {
    auto counter    = mutableStateOf("counter", 0);
    auto textState  = mutableStateOf("title", std::string("Hello, RUI!"));
    auto counterText = mutableStateOf("countText", std::string("Count: 0"));

    // 更新计数器的同时更新文本
    auto onIncrement = [counter, counterText]() {
        counter->set(counter->get() + 1);
        counterText->set("Count: " + std::to_string(counter->get()));
        std::cout << "Counter: " << counter->get() << std::endl;
    };

    return Column(
        Text(textState,
             Modifier().foreground(Color::blue()).background(Color::green()).fontSize(16)),
        Button(
            onIncrement,
            Text(mutableStateOf(std::string("Click me")), Modifier().foreground(Color::red())),
            Modifier().background(Color::blue())
                      .padding(4, 12, 4, 12)
        ),
        Text(counterText,
             Modifier().foreground(Color::grey600()).background(Color::orange()))
    );
}

int main(int argc, char* argv[]) {
    auto platform = PlatformAdapter::create();
    platform->initialize(argc, argv);

    platform->setWindowSize(300, 300);
    platform->setWindowTitle("RUI Counter Demo");
    platform->enableDevTools();

    platform->run([]() {
        // 在 run() 回调中构建 UI，此时 currentLayout 已就绪
        auto widgetTree = App();
        auto* layout = PlatformAdapter::currentLayout();
        if (layout) {
            layout->addWidget(widgetTree.release());
        }
    });

    return 0;
}
