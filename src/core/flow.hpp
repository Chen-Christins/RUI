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
class Flow {
private:
    struct CollectorSubscription {
        std::atomic_bool active = true;
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
            }
            subscription_.reset();
        }

    private:
        std::weak_ptr<CollectorSubscription> subscription_;
    };

public:

    CollectorHandle collect(Collector collector)
    {
        if (!collector) {
            return {};
        }

        auto newCollector = std::move(collector);
        auto subscription = std::make_shared<CollectorSubscription>();
        std::optional<T> replayValue;
        {
            std::lock_guard lock(mutex_);
            pruneCancelledLocked();
            collectors_.push_back(CollectorEntry{subscription, newCollector});
            if (hasValue_) {
                replayValue = lastValue_;
            }
        }
        if (replayValue) {
            newCollector(*replayValue);
        }
        return CollectorHandle(subscription);
    }

    // Internal: emit a value to all collectors.
    void emitValue(const T& value)
    {
        std::vector<CollectorEntry> collectors;
        {
            std::lock_guard lock(mutex_);
            pruneCancelledLocked();
            lastValue_ = value;
            hasValue_ = true;
            collectors = collectors_;
        }
        for (const auto& entry : collectors) {
            if (!entry.subscription->active.load(std::memory_order_acquire)) {
                continue;
            }
            if (entry.collector) {
                entry.collector(value);
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
    auto blockPtr = std::make_shared<std::function<T()>>(std::move(block));
    auto tracker = std::make_shared<detail::ReadTracker>();

    tracker->setOnInvalidate([tracker, blockPtr, flow]() {
        tracker->clearSubscriptions();
        detail::ReadTrackingScope trackingScope(tracker.get());
        T value = (*blockPtr)();
        flow->emitValue(value);
    });

    detail::ReadTrackingScope trackingScope(tracker.get());
    T value = (*blockPtr)();
    flow->emitValue(value);
    return flow;
}

} // namespace rui
