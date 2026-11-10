// flow.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <vector>

#include "observable_state.hpp"

namespace rui {

// Flow - minimal flow abstraction: register collectors, invoke them on each emission (main-thread, synchronous).
// The last emitted value is replayed to a collector when it registers.
template <typename T>
class Flow {
private:
    struct CollectorControl {
        std::mutex mutex;
        std::unordered_set<size_t> cancelled;
    };

public:
    using Collector = std::function<void(const T&)>;

    class CollectorHandle {
    public:
        CollectorHandle() = default;
        CollectorHandle(std::weak_ptr<CollectorControl> control, size_t id)
            : control_(std::move(control)), id_(id)
        {
        }

        CollectorHandle(const CollectorHandle&) = delete;
        CollectorHandle& operator=(const CollectorHandle&) = delete;
        CollectorHandle(CollectorHandle&& other) noexcept
            : control_(std::move(other.control_)), id_(other.id_)
        {
            other.id_ = 0;
        }
        CollectorHandle& operator=(CollectorHandle&& other) noexcept
        {
            if (this != &other) {
                cancel();
                control_ = std::move(other.control_);
                id_ = other.id_;
                other.id_ = 0;
            }
            return *this;
        }

        void cancel()
        {
            if (auto control = control_.lock()) {
                std::lock_guard lock(control->mutex);
                control->cancelled.insert(id_);
            }
            id_ = 0;
        }

    private:
        std::weak_ptr<CollectorControl> control_;
        size_t id_ = 0;
    };

public:

    CollectorHandle collect(Collector collector)
    {
        if (!collector) {
            return {};
        }

        auto newCollector = std::move(collector);
        std::optional<T> replayValue;
        size_t id = 0;
        {
            std::lock_guard lock(mutex_);
            pruneCancelledLocked();
            id = nextCollectorId_++;
            collectors_.push_back(CollectorEntry{id, newCollector});
            if (hasValue_) {
                replayValue = lastValue_;
            }
        }
        if (replayValue) {
            newCollector(*replayValue);
        }
        return CollectorHandle(collectorControl_, id);
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
            bool cancelled = false;
            {
                std::lock_guard lock(collectorControl_->mutex);
                cancelled = collectorControl_->cancelled.contains(entry.id);
            }
            if (cancelled) {
                continue;
            }
            if (entry.collector) {
                entry.collector(value);
            }
        }
    }

private:
    struct CollectorEntry {
        size_t id = 0;
        Collector collector;
    };

    mutable std::mutex mutex_;
    std::shared_ptr<CollectorControl> collectorControl_ = std::make_shared<CollectorControl>();
    std::vector<CollectorEntry> collectors_;
    size_t nextCollectorId_ = 1;
    T lastValue_{};
    bool hasValue_ = false;

    void pruneCancelledLocked()
    {
        std::lock_guard controlLock(collectorControl_->mutex);
        collectors_.erase(
            std::remove_if(collectors_.begin(), collectors_.end(), [&](const CollectorEntry& entry) {
                return collectorControl_->cancelled.contains(entry.id);
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
