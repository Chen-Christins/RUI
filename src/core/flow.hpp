// flow.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <algorithm>
#include <atomic>
#include <vector>

#include "observable_state.hpp"

namespace rui {

// Flow - minimal flow abstraction: register collectors, invoke them on each emission (main-thread, synchronous).
// The last emitted value is replayed to a collector when it registers.
template <typename T>
class Flow : public std::enable_shared_from_this<Flow<T>> {
private:
    struct CollectorSubscription {
        std::atomic_bool active = true;
        std::function<void()> onCancel;
    };

    struct SubscriptionState {
        std::atomic_size_t count = 0;
        std::mutex mutex;
        std::function<void(size_t)> callback;
    };

public:
    using Collector = std::function<void(const T&)>;

    class CollectorHandle {
    public:
        CollectorHandle() = default;
        CollectorHandle(std::weak_ptr<CollectorSubscription> subscription)
            : subscription_(std::move(subscription))
        {
        }

        CollectorHandle(const CollectorHandle&) = delete;
        CollectorHandle& operator=(const CollectorHandle&) = delete;
        CollectorHandle(CollectorHandle&& other) noexcept
            : subscription_(std::move(other.subscription_))
        {
        }
        CollectorHandle& operator=(CollectorHandle&& other) noexcept
        {
            if (this != &other) {
                cancel();
                subscription_ = std::move(other.subscription_);
            }
            return *this;
        }

        void cancel()
        {
            if (auto subscription = subscription_.lock()) {
                subscription->active.store(false, std::memory_order_release);
                if (subscription->onCancel) {
                    auto callback = std::move(subscription->onCancel);
                    callback();
                }
            }
            subscription_.reset();
        }

    private:
        std::weak_ptr<CollectorSubscription> subscription_;
    };

public:

    void setSubscriptionCountCallback(std::function<void(size_t)> callback)
    {
        std::lock_guard lock(subscriptionState_->mutex);
        subscriptionState_->callback = std::move(callback);
    }

    void setDistinct(bool distinct)
    {
        std::lock_guard lock(mutex_);
        distinct_ = distinct;
    }

    CollectorHandle collect(Collector collector)
    {
        if (!collector) {
            return {};
        }

        auto newCollector = std::move(collector);
        auto subscription = std::make_shared<CollectorSubscription>();
        std::optional<T> replayValue;
        std::function<void(size_t)> countCallback;
        size_t collectorCount = 0;
        {
            std::lock_guard lock(mutex_);
            pruneCancelledLocked();
            collectors_.push_back(CollectorEntry{subscription, newCollector});
            {
                std::lock_guard stateLock(subscriptionState_->mutex);
                countCallback = subscriptionState_->callback;
            }
            collectorCount = subscriptionState_->count.fetch_add(1, std::memory_order_acq_rel) + 1;
            if (hasValue_) {
                replayValue = lastValue_;
            }
        }
        if (replayValue) {
            newCollector(*replayValue);
        }
        auto subscriptionState = subscriptionState_;
        subscription->onCancel = [subscriptionState]() {
            const auto previous = subscriptionState->count.fetch_sub(1, std::memory_order_acq_rel);
            const auto count = previous > 0 ? previous - 1 : 0;
            std::function<void(size_t)> callback;
            {
                std::lock_guard lock(subscriptionState->mutex);
                callback = subscriptionState->callback;
            }
            if (callback) {
                callback(count);
            }
        };
        if (countCallback) {
            countCallback(collectorCount);
        }
        return CollectorHandle(subscription);
    }

    // Internal: emit a value to all collectors.
    void emitValue(const T& value)
    {
        std::vector<CollectorEntry> collectors;
        bool distinct = false;
        {
            std::lock_guard lock(mutex_);
            pruneCancelledLocked();
            distinct = distinct_;
            if (distinct && hasValue_ && lastValue_ == value) {
                return;
            }
            lastValue_ = value;
            hasValue_ = true;
            collectors = collectors_;
        }
        for (const auto& entry : collectors) {
            if (!entry.subscription->active.load(std::memory_order_acquire)) {
                continue;
            }
            if (entry.collector) {
                try {
                    entry.collector(value);
                } catch (...) {
                    // One collector must not break the remaining collectors.
                }
            }
        }
    }

private:
    struct CollectorEntry {
        std::shared_ptr<CollectorSubscription> subscription;
        Collector collector;
    };

    mutable std::mutex mutex_;
    std::vector<CollectorEntry> collectors_;
    T lastValue_{};
    bool hasValue_ = false;
    bool distinct_ = false;
    std::shared_ptr<SubscriptionState> subscriptionState_ = std::make_shared<SubscriptionState>();

    void pruneCancelledLocked()
    {
        collectors_.erase(
            std::remove_if(collectors_.begin(), collectors_.end(), [&](const CollectorEntry& entry) {
                return !entry.subscription->active.load(std::memory_order_acquire);
            }),
            collectors_.end());
    }
};

// snapshotFlow - turn state reads into a Flow: re-evaluates and emits whenever a state read inside block changes.
template <typename T>
std::shared_ptr<Flow<T>> snapshotFlow(std::function<T()> block)
{
    auto flow = std::make_shared<Flow<T>>();
    std::weak_ptr<Flow<T>> weakFlow = flow;
    auto blockPtr = std::make_shared<std::function<T()>>(std::move(block));
    auto tracker = std::make_shared<detail::ReadTracker>();
    auto active = std::make_shared<std::atomic_bool>(false);
    flow->setDistinct(true);

    auto evaluate = [tracker, blockPtr, weakFlow](bool shouldEmit) {
        detail::ReadTracker nextTracker;
        nextTracker.setOnInvalidate(tracker->invalidationCallback());
        T value;
        try {
            detail::ReadTrackingScope trackingScope(&nextTracker);
            value = (*blockPtr)();
        } catch (...) {
            nextTracker.clearSubscriptions();
            throw;
        }
        if (shouldEmit) {
            if (auto flow = weakFlow.lock()) {
                flow->emitValue(value);
            }
        }
        tracker->replaceSubscriptions(nextTracker);
    };

    flow->setSubscriptionCountCallback([tracker, blockPtr, weakFlow, active, evaluate](size_t count) {
        if (count == 0) {
            active->store(false, std::memory_order_release);
            tracker->clearSubscriptions();
            return;
        }
        if (active->exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        try {
            evaluate(true);
        } catch (...) {
            active->store(false, std::memory_order_release);
        }
    });

    tracker->setOnInvalidate([tracker, blockPtr, weakFlow, active, evaluate]() {
        if (!active->load(std::memory_order_acquire)) {
            return;
        }
        try {
            evaluate(true);
        } catch (...) {
            // Keep the previous dependency subscriptions and value; a later change can retry.
        }
    });
    return flow;
}

} // namespace rui
