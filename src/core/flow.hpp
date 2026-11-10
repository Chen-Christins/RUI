// flow.hpp
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "observable_state.hpp"

namespace rui {

// Flow - minimal flow abstraction: register collectors, invoke them on each emission (main-thread, synchronous).
// The last emitted value is replayed to a collector when it registers.
template <typename T>
class Flow {
public:
    using Collector = std::function<void(const T&)>;

    void collect(Collector collector)
    {
        if (!collector) {
            return;
        }

        auto newCollector = std::move(collector);
        std::optional<T> replayValue;
        {
            std::lock_guard lock(mutex_);
            collectors_.push_back(newCollector);
            if (hasValue_) {
                replayValue = lastValue_;
            }
        }
        if (replayValue) {
            newCollector(*replayValue);
        }
    }

    // Internal: emit a value to all collectors.
    void emitValue(const T& value)
    {
        std::vector<Collector> collectors;
        {
            std::lock_guard lock(mutex_);
            lastValue_ = value;
            hasValue_ = true;
            collectors = collectors_;
        }
        for (const auto& collector : collectors) {
            if (collector) {
                collector(value);
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::vector<Collector> collectors_;
    T lastValue_{};
    bool hasValue_ = false;
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
