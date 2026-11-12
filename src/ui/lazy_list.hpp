#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../core/observable_state.hpp"
#include "../core/composable.hpp"
#include "../core/compose_widget.hpp"
#include "component.hpp"
#include "modifier.hpp"

namespace rui {

class LazyListScope {
public:
    using ItemFactory = std::function<Component()>;
    using IndexedItemFactory = std::function<Component(int index)>;
    using KeyFactory = std::function<int(int index)>;

    void item(ItemFactory factory, int key = -1);

    void items(int count, IndexedItemFactory factory, KeyFactory key = nullptr);

    template <typename T, typename Content>
    void items(const std::vector<T>& values, Content content, KeyFactory key = nullptr)
    {
        for (int index = 0; index < static_cast<int>(values.size()); ++index) {
            const T& value = values[static_cast<size_t>(index)];
            const int itemKey = key ? key(index) : index;
            item([value, index, content]() mutable {
                return content(value);
            }, itemKey);
        }
    }

    template <typename T, typename Content>
    void itemsIndexed(const std::vector<T>& values, Content content, KeyFactory key = nullptr)
    {
        for (int index = 0; index < static_cast<int>(values.size()); ++index) {
            const T& value = values[static_cast<size_t>(index)];
            const int itemKey = key ? key(index) : index;
            item([value, index, content]() mutable {
                return content(index, value);
            }, itemKey);
        }
    }

    int itemCount() const { return static_cast<int>(items_.size()); }
    int keyAt(int index) const;
    Component build(int index) const;

private:
    struct Entry {
        ItemFactory factory;
        int key = 0;
    };

    std::vector<Entry> items_;
};

template <typename T>
using LazyListItems = std::shared_ptr<MutableState<std::vector<T>>>;

template <typename T>
LazyListItems<T> lazyListItems(std::vector<T> initial)
{
    return mutableStateOf(std::move(initial));
}

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
    std::shared_ptr<LazyListState> state = nullptr);

Component LazyRow(
    int itemCount,
    std::function<Component(int index)> factory,
    Modifier modifier = Modifier{},
    std::function<int(int index)> key = nullptr,
    std::shared_ptr<LazyListState> state = nullptr);

Component LazyColumn(
    std::function<void(LazyListScope&)> content,
    Modifier modifier = Modifier{},
    std::shared_ptr<LazyListState> state = nullptr);

Component LazyRow(
    std::function<void(LazyListScope&)> content,
    Modifier modifier = Modifier{},
    std::shared_ptr<LazyListState> state = nullptr);

template <typename T, typename Content>
Component LazyColumn(
    LazyListItems<T> values,
    Content content,
    Modifier modifier = Modifier{},
    std::shared_ptr<LazyListState> state = nullptr,
    LazyListScope::KeyFactory key = nullptr)
{
    return compose([values, content, modifier, state, key]() mutable {
        if (values) {
            observe(values);
        }
        const auto snapshot = values ? values->get() : std::vector<T>{};
        return LazyColumn(
            [snapshot, content, key](LazyListScope& scope) mutable {
                scope.items(snapshot, content, key);
            },
            modifier,
            state
        );
    });
}

template <typename T, typename Content>
Component LazyRow(
    LazyListItems<T> values,
    Content content,
    Modifier modifier = Modifier{},
    std::shared_ptr<LazyListState> state = nullptr,
    LazyListScope::KeyFactory key = nullptr)
{
    return compose([values, content, modifier, state, key]() mutable {
        if (values) {
            observe(values);
        }
        const auto snapshot = values ? values->get() : std::vector<T>{};
        return LazyRow(
            [snapshot, content, key](LazyListScope& scope) mutable {
                scope.items(snapshot, content, key);
            },
            modifier,
            state
        );
    });
}

} // namespace rui
