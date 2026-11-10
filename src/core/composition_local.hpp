// composition_local.hpp
#pragma once

#include <QGuiApplication>
#include <QScreen>
#include <cmath>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "composable.hpp"

namespace rui {

// Density —— dp/sp 与像素之间的换算密度。
struct Density {
    float density = 1.0f;   // dp -> px 倍率
    float fontScale = 1.0f; // sp 字体缩放

    int dpToPx(int dp) const { return static_cast<int>(std::lround(dp * density)); }
    int pxToDp(int px) const { return static_cast<int>(std::lround(px / density)); }
    int spToPx(int sp) const { return static_cast<int>(std::lround(sp * density * fontScale)); }
};

// Configuration —— 屏幕 / 设备配置。
struct Configuration {
    int screenWidth = 0;
    int screenHeight = 0;
    bool portrait = true;
};

namespace detail {

struct ProvidedValue {
    const void* key = nullptr;
    std::shared_ptr<void> value;
};

inline std::vector<ProvidedValue>& providedStack()
{
    static thread_local std::vector<ProvidedValue> stack;
    return stack;
}

template <typename T>
const T* findProvided(const void* key)
{
    auto& stack = providedStack();
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        if (it->key == key) {
            return static_cast<const T*>(it->value.get());
        }
    }
    return nullptr;
}

template <typename T>
class Provider {
public:
    Provider(const void* key, T value)
        : key_(key)
        , value_(std::make_shared<T>(std::move(value)))
    {
        providedStack().push_back(ProvidedValue{key_, value_});
    }

    ~Provider()
    {
        auto& stack = providedStack();
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            if (it->key == key_ && it->value.get() == value_.get()) {
                stack.erase(std::next(it).base());
                break;
            }
        }
    }

    Provider(const Provider&) = delete;
    Provider& operator=(const Provider&) = delete;

private:
    const void* key_;
    std::shared_ptr<T> value_;
};

} // namespace detail

template <typename T>
class CompositionLocal {
public:
    using DefaultProvider = std::function<T()>;

    CompositionLocal() = default;
    explicit CompositionLocal(DefaultProvider provider)
        : default_(std::move(provider))
    {
    }

    T current() const
    {
        if (const T* v = detail::findProvided<T>(this)) {
            return *v;
        }
        if (default_) {
            return default_();
        }
        return T{};
    }

    detail::Provider<T> provides(T value) const { return detail::Provider<T>(this, std::move(value)); }

private:
    DefaultProvider default_;
};

// 无默认值。
template <typename T>
CompositionLocal<T> staticCompositionLocalOf()
{
    return CompositionLocal<T>();
}

// 带默认值。
template <typename T>
CompositionLocal<T> compositionLocalOf(typename CompositionLocal<T>::DefaultProvider provider)
{
    return CompositionLocal<T>(std::move(provider));
}

// CompositionLocalProvider —— 在 content 作用域内提供 local 的值。
template <typename T, typename Content>
auto CompositionLocalProvider(CompositionLocal<T> local, T value, Content&& content)
{
    auto guard = local.provides(std::move(value));
    return content();
}

// 默认的 LocalDensity / LocalConfiguration。
inline CompositionLocal<Density> LocalDensity{[]() -> Density {
    Density d;
    if (auto* screen = QGuiApplication::primaryScreen()) {
        d.density = static_cast<float>(screen->devicePixelRatio());
    }
    return d;
}};

inline CompositionLocal<Configuration> LocalConfiguration{[]() -> Configuration {
    Configuration c;
    if (auto* screen = QGuiApplication::primaryScreen()) {
        c.screenWidth = screen->size().width();
        c.screenHeight = screen->size().height();
    }
    return c;
}};

} // namespace rui
