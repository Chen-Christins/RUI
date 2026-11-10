// flow.hpp
#pragma once

#include <functional>
#include <memory>
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
        if (collector) {
            collectors_.push_back(std::move(collector));
        }
        if (hasValue_) {
            auto value = lastValue_;
            for (const auto& c : collectors_) {
                if (c) {
                    c(value);
                }
            }
        }
    }

    // Internal: emit a value to all collectors.
    void emitValue(const T& value)
    {
        lastValue_ = value;
        hasValue_ = true;
        for (const auto& collector : collectors_) {
            if (collector) {
                collector(value);
            }
        }
    }

private:
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
        detail::ReadTracker::begin(tracker.get());
        T value = (*blockPtr)();
        detail::ReadTracker::end();
        flow->emitValue(value);
    });

    detail::ReadTracker::begin(tracker.get());
    T value = (*blockPtr)();
    detail::ReadTracker::end();
    flow->emitValue(value);
    return flow;
}

} // namespace rui
