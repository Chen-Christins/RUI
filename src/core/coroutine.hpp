// coroutine.hpp
#pragma once

#include <QTimer>

#include <coroutine>
#include <exception>
#include <memory>
#include <utility>

namespace rui {

// 协程取消令牌：所有挂起点共享同一令牌，cancel() 后不再恢复。
class CancellationToken {
public:
    void cancel() { cancelled_ = true; }
    bool cancelled() const { return cancelled_; }

private:
    bool cancelled_ = false;
};

// Job —— 可取消的 fire-and-forget 协程，运行在主线程事件循环上。
class Job {
public:
    struct promise_type;

    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::shared_ptr<CancellationToken> token;
        std::exception_ptr exception_;

        Job get_return_object() { return Job(Handle::from_promise(*this)); }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception_ = std::current_exception();
        }
    };

    Job() = default;

    explicit Job(Handle handle) : handle_(handle) {}

    ~Job() { destroy(); }

    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    Job(Job&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

    Job& operator=(Job&& other) noexcept {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    bool valid() const { return handle_ != nullptr; }
    bool done() const { return !handle_ || handle_.done(); }

    Handle handle() const { return handle_; }

    // 启动协程：设置令牌并执行到首个挂起点。
    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
        if (handle_ && handle_.done() && handle_.promise().exception_) {
            try {
                std::rethrow_exception(handle_.promise().exception_);
            } catch (const std::exception& e) {
                // 协程异常仅记录，不向调用方传播。
            } catch (...) {
            }
        }
    }

    // 显式销毁协程帧。
    void destroy() {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

private:
    Handle handle_ = nullptr;
};

// delay —— 主线程事件循环上的可取消延时挂起点。
class Delay {
public:
    explicit Delay(int ms) : ms_(ms) {}

    bool await_ready() const noexcept { return ms_ <= 0; }

    void await_suspend(std::coroutine_handle<> h) {
        auto handle = Job::Handle::from_address(h.address());
        auto token = handle.promise().token;

        auto* timer = new QTimer();
        timer->setSingleShot(true);
        timer->setInterval(ms_);
        QObject::connect(timer, &QTimer::timeout, [handle, token, timer]() {
            timer->deleteLater();
            if (token && token->cancelled()) {
                return;
            }
            handle.resume();
        });
        timer->start();
    }

    void await_resume() const noexcept {}

private:
    int ms_;
};

inline Delay delay(int ms) {
    return Delay(ms);
}

} // namespace rui
