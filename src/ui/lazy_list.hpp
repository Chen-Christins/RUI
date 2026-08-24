#pragma once

#include <functional>
#include <memory>

#include "component.hpp"
#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

// LazyListState —— 惰性列表的滚动状态，暴露首个可见项索引，并提供 item 级导航。
class LazyListState {
public:
    LazyListState();
    ~LazyListState();

    // 首个可见 item 索引（可观察状态，可订阅 / 供 DevTools 展示）。
    std::shared_ptr<MutableState<int>> firstVisibleIndex();

    // 按 index 滚动到指定项；offsetPx 为额外主轴向偏移（像素，可为负）。
    void scrollToItem(int index, int offsetPx = 0);

    // 动画滚动到指定项。
    void animateScrollToItem(int index, int offsetPx = 0);

    // 由 LazyListWidget 注册：把 item 级导航翻译为像素级滚动请求。
    using ScrollRequest = std::function<void(int index, int offsetPx, bool animate)>;
    void setScrollRequest(ScrollRequest request);
    void setFirstVisibleIndex(int index);

private:
    std::shared_ptr<MutableState<int>> firstVisibleIndex_;
    ScrollRequest scrollRequest_;
};

// key 返回每个 item 的稳定标识，用于复用 item 以保留其内部状态。
// 默认 nullptr，退化为主轴向 index 即 key。
Component LazyColumn(
    int itemCount,
    std::function<Component(int index)> factory,
    Modifier modifier = Modifier{},
    std::function<int(int index)> key = nullptr,
    std::shared_ptr<LazyListState> state = nullptr
);

Component LazyRow(
    int itemCount,
    std::function<Component(int index)> factory,
    Modifier modifier = Modifier{},
    std::function<int(int index)> key = nullptr,
    std::shared_ptr<LazyListState> state = nullptr
);

} // namespace rui
