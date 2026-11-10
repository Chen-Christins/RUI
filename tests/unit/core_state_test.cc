#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

#include <QObject>

#include "core/composable.hpp"
#include "core/derived_state.hpp"
#include "core/flow.hpp"
#include "core/observable_state.hpp"

namespace {

void check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void mutableStateNotifiesOnlyWhenValueChanges()
{
    auto state = rui::mutableStateOf(1);
    int notifications = 0;
    int lastValue = 0;
    state->observe([&](const int& value) {
        ++notifications;
        lastValue = value;
    });

    check(state->get() == 1, "MutableState returns its initial value");
    state->set(1);
    check(notifications == 0, "MutableState ignores equal values");

    state->set(2);
    check(notifications == 1, "MutableState notifies on change");
    check(lastValue == 2, "MutableState notification contains the new value");
    check(state->get() == 2, "MutableState stores the new value");
}

void observerHandleCancelsNotifications()
{
    auto state = rui::mutableStateOf(0);
    int notifications = 0;
    auto handle = state->observe([&](const int&) {
        ++notifications;
    });

    state->set(1);
    check(notifications == 1, "Active observer receives notifications");
    handle.cancel();
    state->set(2);
    check(notifications == 1, "Cancelled observer stops receiving notifications");
}

void ownerBoundObserverStopsAfterOwnerDestruction()
{
    auto state = rui::mutableStateOf(0);
    int notifications = 0;
    {
        QObject owner;
        state->observe(&owner, [&](const int&) {
            ++notifications;
        });
        state->set(1);
        check(notifications == 1, "Owner-bound observer receives notifications while alive");
    }
    state->set(2);
    check(notifications == 1, "Owner-bound observer stops after owner destruction");
}

void composableContextInvalidatesAndRemembers()
{
    auto context = std::make_shared<rui::ComposableContext>();
    int recompositions = 0;
    context->setRecomposer([&]() {
        ++recompositions;
    });

    auto state = rui::mutableStateOf(0);
    context->markComposed();
    int& remembered = context->remember(10);
    context->reset();
    int& rememberedAgain = context->remember(20);
    check(&remembered == &rememberedAgain, "remember preserves the slot value");
    check(rememberedAgain == 10, "remember does not replace an existing value");

    context->observe(state);
    state->set(1);
    check(recompositions == 1, "Observed state triggers recomposition");
    check(context->needsRecompose(), "Context is marked for recomposition");

    context->markComposed();
    state->set(2);
    check(recompositions == 2, "Context can be invalidated again after composition");
}

void concurrentStateWritesInvalidateContextOnce()
{
    auto context = std::make_shared<rui::ComposableContext>();
    std::atomic<int> recompositions = 0;
    context->setRecomposer([&]() {
        ++recompositions;
    });

    auto state = rui::mutableStateOf(0);
    context->observe(state);

    std::thread first([&]() {
        state->set(1);
    });
    std::thread second([&]() {
        state->set(2);
    });
    first.join();
    second.join();

    check(recompositions == 1, "Concurrent state writes schedule one pending recomposition");
    check(context->needsRecompose(), "Concurrent state writes mark the context safely");
}

void contextResetDoesNotAccumulateObservers()
{
    auto context = std::make_shared<rui::ComposableContext>();
    int recompositions = 0;
    context->setRecomposer([&]() {
        ++recompositions;
    });
    auto state = rui::mutableStateOf(0);

    context->observe(state);
    context->markComposed();
    context->reset();
    context->observe(state);
    context->markComposed();

    state->set(1);
    check(recompositions == 1, "Reset cancels observers from the previous composition");
}

void concurrentObserveAndResetIsSafe()
{
    auto context = std::make_shared<rui::ComposableContext>();
    auto state = rui::mutableStateOf(0);
    std::atomic<int> callbacks = 0;

    std::thread observerThread([&]() {
        for (int i = 0; i < 1000; ++i) {
            context->observe(state);
        }
    });
    std::thread resetThread([&]() {
        for (int i = 0; i < 1000; ++i) {
            context->reset();
        }
    });
    observerThread.join();
    resetThread.join();

    context->setRecomposer([&]() {
        ++callbacks;
    });
    state->set(1);
    check(callbacks <= 1, "Concurrent observe and reset leaves no duplicate active callbacks");
}

void keyedRememberKeepsValuesAcrossReset()
{
    rui::ComposableContext context;

    int& first = context.remember(std::string("counter"), 7);
    context.reset();
    int& second = context.remember(std::string("counter"), 99);

    check(&first == &second, "Keyed remember preserves the keyed value");
    check(second == 7, "Keyed remember does not replace the existing value");
}

void derivedStateTracksDependencies()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    auto source = rui::mutableStateOf(2);
    int computations = 0;
    auto derived = rui::derivedStateOf<int>([&]() {
        ++computations;
        return source->get() * 2;
    });

    check(computations == 1, "DerivedState computes its initial value");
    check(derived->get() == 4, "DerivedState exposes its computed value");

    source->set(2);
    check(computations == 1, "DerivedState ignores unchanged dependencies");

    source->set(3);
    check(computations == 2, "DerivedState recomputes when a dependency changes");
    check(derived->get() == 6, "DerivedState publishes the updated value");
    rui::ComposableContext::setCurrent(nullptr);
}

void nestedDerivedStateRestoresDependencyTracking()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    auto source = rui::mutableStateOf(1);
    int innerComputations = 0;
    int outerComputations = 0;
    auto inner = rui::derivedStateOf<int>([&]() {
        ++innerComputations;
        return source->get() + 1;
    });
    auto outer = rui::derivedStateOf<int>([&]() {
        ++outerComputations;
        return inner->get() * 2;
    });

    check(innerComputations == 1, "Nested derived state computes its inner value");
    check(outerComputations == 1, "Nested derived state computes its outer value");
    check(outer->get() == 4, "Nested derived state returns the composed value");

    source->set(3);
    check(innerComputations == 2, "Inner derived state tracks its source");
    check(outerComputations == 2, "Outer derived state tracks the inner value");
    check(outer->get() == 8, "Nested derived state updates after the source changes");
    rui::ComposableContext::setCurrent(nullptr);
}

void snapshotFlowReplaysOnlyToNewCollectors()
{
    auto source = rui::mutableStateOf(1);
    auto flow = rui::snapshotFlow<int>([&]() {
        return source->get() * 2;
    });
    int firstCollectorCalls = 0;
    int secondCollectorCalls = 0;
    int firstLastValue = 0;
    int secondLastValue = 0;

    flow->collect([&](const int& value) {
        ++firstCollectorCalls;
        firstLastValue = value;
    });
    check(firstCollectorCalls == 1, "First snapshotFlow collector receives the initial value");
    check(firstLastValue == 2, "First snapshotFlow collector receives the computed value");

    flow->collect([&](const int& value) {
        ++secondCollectorCalls;
        secondLastValue = value;
    });
    check(firstCollectorCalls == 1, "Adding a collector does not replay to existing collectors");
    check(secondCollectorCalls == 1, "Second snapshotFlow collector receives one replay");
    check(secondLastValue == 2, "Second snapshotFlow collector receives the latest value");

    source->set(3);
    check(firstCollectorCalls == 2, "First collector receives the changed value once");
    check(secondCollectorCalls == 2, "Second collector receives the changed value once");
    check(firstLastValue == 6 && secondLastValue == 6,
          "All snapshotFlow collectors receive the updated value");
}

void snapshotFlowStopsTrackingWithoutCollectors()
{
    auto source = rui::mutableStateOf(1);
    int computations = 0;
    auto flow = rui::snapshotFlow<int>([&]() {
        ++computations;
        return source->get();
    });
    check(computations == 0, "SnapshotFlow stays lazy without collectors");

    auto handle = flow->collect([](const int&) {});
    check(computations == 1, "SnapshotFlow starts tracking on first collector");
    handle.cancel();
    source->set(2);
    check(computations == 1, "SnapshotFlow stops tracking after the last collector cancels");
}

void flowHandleCanOutliveFlow()
{
    rui::Flow<int>::CollectorHandle handle;
    {
        auto flow = std::make_unique<rui::Flow<int>>();
        handle = flow->collect([](const int&) {});
    }
    handle.cancel();
    check(true, "A collector handle can safely outlive its flow");
}

void flowCollectorCanRegisterDuringEmission()
{
    rui::Flow<int> flow;
    int firstCalls = 0;
    int secondCalls = 0;
    int secondLastValue = 0;
    flow.collect([&](const int& value) {
        ++firstCalls;
        if (value == 1) {
            flow.collect([&](const int& replayed) {
                ++secondCalls;
                secondLastValue = replayed;
            });
        }
    });

    flow.emitValue(1);
    check(firstCalls == 1, "Original collector receives the emitted value once");
    check(secondCalls == 1, "Reentrant collector receives one replay");
    check(secondLastValue == 1, "Reentrant collector receives the current value");

    flow.emitValue(2);
    check(firstCalls == 2, "Original collector receives the next value once");
    check(secondCalls == 2, "Reentrant collector receives the next value once");
    check(secondLastValue == 2, "Reentrant collector receives the updated value");
}

void flowCollectorHandleCancelsSubscription()
{
    rui::Flow<int> flow;
    int calls = 0;
    auto handle = flow.collect([&](const int&) {
        ++calls;
    });

    flow.emitValue(1);
    check(calls == 1, "Active flow collector receives emissions");
    handle.cancel();
    flow.emitValue(2);
    check(calls == 1, "Cancelled flow collector stops receiving emissions");
}

void flowCollectorCanBeCancelledMoreThanOnce()
{
    rui::Flow<int> flow;
    int calls = 0;
    auto handle = flow.collect([&](const int&) {
        ++calls;
    });

    handle.cancel();
    handle.cancel();
    flow.emitValue(1);
    check(calls == 0, "Repeated cancellation remains safe and idempotent");

    auto replacement = flow.collect([&](const int&) {
        ++calls;
    });
    check(calls == 1, "Replacement collector receives the latest replay once");
    flow.emitValue(2);
    check(calls == 2, "New collectors remain active after cancelled entries are pruned");
    replacement.cancel();
}

} // namespace

int main()
{
    mutableStateNotifiesOnlyWhenValueChanges();
    observerHandleCancelsNotifications();
    ownerBoundObserverStopsAfterOwnerDestruction();
    composableContextInvalidatesAndRemembers();
    concurrentStateWritesInvalidateContextOnce();
    contextResetDoesNotAccumulateObservers();
    concurrentObserveAndResetIsSafe();
    keyedRememberKeepsValuesAcrossReset();
    derivedStateTracksDependencies();
    nestedDerivedStateRestoresDependencyTracking();
    snapshotFlowReplaysOnlyToNewCollectors();
    snapshotFlowStopsTrackingWithoutCollectors();
    flowHandleCanOutliveFlow();
    flowCollectorCanRegisterDuringEmission();
    flowCollectorHandleCancelsSubscription();
    flowCollectorCanBeCancelledMoreThanOnce();
    std::cout << "RUI core state tests passed\n";
    return EXIT_SUCCESS;
}
