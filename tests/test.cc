// main.cpp
#include "platform_adapter.hpp"
#include "text.hpp"
#include "button.hpp"
#include "column.hpp"

#include <iostream>

using namespace rui;

// 应用主界面
Composable std::unique_ptr<QWidget> App() {
    auto counter = mutableStateOf(0);
    auto textState = mutableStateOf(std::string("Hello, RUI!"));

    // 用于显示的计数器文本
    auto counterText = mutableStateOf(std::string("Count: 0"));

    // 更新计数器的同时更新文本
    auto onIncrement = [counter, counterText]() {
        counter->set(counter->get() + 1);
        counterText->set("Count: " + std::to_string(counter->get()));
        std::cout << "Counter: " << counter->get() << std::endl;
    };

    return Column(
        Text(textState),
        Button(
            onIncrement,
            Text(mutableStateOf(std::string("Click me")))
        ),
        Text(counterText)
    );
}

int main(int argc, char* argv[]) {
    auto platform = PlatformAdapter::create();
    platform->initialize(argc, argv);

    platform->setWindowSize(300, 300);

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
