// coroutine_scope.hpp
#pragma once

#include <memory>
#include <vector>

#include "coroutine.hpp"

namespace rui {

// CoroutineScope —— 拥有并取消一组协程任务，作用域析构时自动取消。
class CoroutineScope {
public:
    CoroutineScope();
    ~CoroutineScope();

    CoroutineScope(const CoroutineScope&) = delete;
    CoroutineScope& operator=(const CoroutineScope&) = delete;

    void launch(Job job);
    void cancel();

private:
    std::shared_ptr<CancellationToken> token_;
    std::vector<Job> jobs_;
};

} // namespace rui
