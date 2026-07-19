/**
 * @file singleton.hpp
 * @brief 单例模式工具
 * @author Christins (chen.christins@qq.com)
 * @date 2026-07-19
 * @copyright Apache 2.0
 */
#pragma once

#include <memory>

namespace rui {

/**
 * @brief 单例模式工具，返回 T*
 * @tparam T 
 */
template <typename T>
class Singleton {
public:
    static T* Instance() {
        static T v;
        return &v;
    }
};

/**
 * @brief 单例模式工具，返回 std::shared_ptr<T>
 * @tparam T 
 */
template <typename T>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

} // namespace rui