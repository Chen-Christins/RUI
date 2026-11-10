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
#include "ui/lazy_list.hpp"
#include "ui/theme.hpp"

struct CollidingKey {
    int value = 0;

    bool operator==(const CollidingKey& other) const { return value == other.value; }
};

namespace std {
template <>
struct hash<CollidingKey> {
    size_t operator()(const CollidingKey&) const { return 1; }
};
} // namespace std

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

void keyedRememberChecksKeyIdentity()
{
    rui::ComposableContext context;
    int& first = context.remember(CollidingKey{1}, 7);
    int& second = context.remember(CollidingKey{2}, 9);

    check(&first != &second, "Different keyed values do not share storage");
    check(first == 7 && second == 9, "Different keyed values retain independent state");

    context.reset();
    int& firstAgain = context.remember(CollidingKey{1}, 100);
    int& secondAgain = context.remember(CollidingKey{2}, 200);
    check(&firstAgain == &first && &secondAgain == &second,
          "Equal keys restore their original remembered state");
}

void rememberSaveableSavesAndRestoresState()
{
    rui::ComposableContext source;
    rui::ComposableContext::setCurrent(&source);
    int& count = rui::rememberSaveable(7);
    count = 42;
    auto saved = source.saveState();
    rui::ComposableContext::setCurrent(nullptr);

    rui::ComposableContext restored;
    restored.restoreState(std::move(saved));
    rui::ComposableContext::setCurrent(&restored);
    int& restoredCount = rui::rememberSaveable(0);
    check(restoredCount == 42, "rememberSaveable restores default-saveable state");
    rui::ComposableContext::setCurrent(nullptr);
}

void rememberSaveableSupportsCustomSaver()
{
    rui::ComposableContext source;
    rui::ComposableContext::setCurrent(&source);
    auto saver = rui::Saver<std::string, int>{
        [](const std::string& value) { return static_cast<int>(value.size()); },
        [](const int& value) { return std::string(static_cast<size_t>(value), 'x'); }
    };
    rui::rememberSaveable(std::string("hello"), saver);
    auto saved = source.saveState();
    rui::ComposableContext::setCurrent(nullptr);

    rui::ComposableContext restored;
    restored.restoreState(std::move(saved));
    rui::ComposableContext::setCurrent(&restored);
    std::string& restoredValue = rui::rememberSaveable(std::string{}, saver);
    check(restoredValue == "xxxxx", "rememberSaveable uses the custom saver");
    rui::ComposableContext::setCurrent(nullptr);
}

void keyedRememberSaveableRestoresByKey()
{
    rui::ComposableContext source;
    rui::ComposableContext::setCurrent(&source);
    int& first = rui::rememberSaveable(std::string("first"), 1);
    int& second = rui::rememberSaveable(std::string("second"), 2);
    first = 11;
    second = 22;
    auto saved = source.saveState();
    rui::ComposableContext::setCurrent(nullptr);

    rui::ComposableContext restored;
    restored.restoreState(std::move(saved));
    rui::ComposableContext::setCurrent(&restored);
    int& secondRestored = rui::rememberSaveable(std::string("second"), 0);
    int& firstRestored = rui::rememberSaveable(std::string("first"), 0);
    check(secondRestored == 22 && firstRestored == 11,
          "Keyed rememberSaveable restores state independent of declaration order");
    rui::ComposableContext::setCurrent(nullptr);
}

void keyedRememberSaveableHandlesHashCollisions()
{
    rui::ComposableContext source;
    rui::ComposableContext::setCurrent(&source);
    int& first = rui::rememberSaveable(CollidingKey{1}, 1);
    int& second = rui::rememberSaveable(CollidingKey{2}, 2);
    first = 11;
    second = 22;
    auto saved = source.saveState();
    rui::ComposableContext::setCurrent(nullptr);

    rui::ComposableContext restored;
    restored.restoreState(std::move(saved));
    rui::ComposableContext::setCurrent(&restored);
    int& secondRestored = rui::rememberSaveable(CollidingKey{2}, 0);
    int& firstRestored = rui::rememberSaveable(CollidingKey{1}, 0);
    check(secondRestored == 22 && firstRestored == 11,
          "Keyed saveable state survives hash collisions");
    rui::ComposableContext::setCurrent(nullptr);
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

void lazyListScopeBuildsComposeStyleItems()
{
    rui::LazyListScope scope;
    scope.item([]() { return rui::Component{}; });
    const std::vector<int> values{10, 20, 30};
    scope.items(values, [](int value) {
        return rui::Component{};
    }, [](int index) {
        return 100 + index;
    });
    scope.itemsIndexed(values, [](int index, int value) {
        return rui::Component{};
    });
    scope.items(4, [](int index) {
        return rui::Component{};
    }, [](int index) {
        return 1000 + index;
    });

    check(scope.itemCount() == 11, "LazyListScope expands item and items declarations");
    check(scope.keyAt(0) == 0, "LazyListScope assigns a stable default key to item");
    check(scope.keyAt(1) == 100 && scope.keyAt(3) == 102,
          "LazyListScope preserves custom item keys");
    check(scope.keyAt(4) == 0 && scope.keyAt(6) == 2,
          "LazyListScope assigns index keys to itemsIndexed");
    check(scope.keyAt(7) == 1000 && scope.keyAt(10) == 1003,
          "LazyListScope supports count-based items with custom keys");
    check(scope.build(-1) == nullptr && scope.build(scope.itemCount()) == nullptr,
          "LazyListScope rejects invalid item indexes");
}

void lazyListItemsProvidesObservableData()
{
    auto values = rui::lazyListItems(std::vector<int>{1, 2});
    check(values->get().size() == 2, "LazyListItems stores its initial data");
    values->set(std::vector<int>{3, 4, 5});
    check(values->get().size() == 3 && values->get()[0] == 3,
          "LazyListItems publishes updated data");
}

void vectorStateCanBeObserved()
{
    auto values = rui::mutableStateOf(std::vector<int>{1, 2});
    int notifications = 0;
    values->observe([&](const std::vector<int>& value) {
        ++notifications;
        check(value.size() == 3, "Vector state observer receives the updated vector");
    });
    values->set(std::vector<int>{3, 4, 5});
    check(notifications == 1, "Vector state supports normal observation");
}

void themeProvidesNestedColorAndTypography()
{
    check(rui::currentColorScheme().background == rui::Color::white(),
          "Theme uses the light color scheme by default");
    check(rui::currentTypography().bodyFontSize == 14,
          "Theme uses the default typography by default");

    const auto dark = rui::ColorScheme::dark();
    rui::Typography typography;
    typography.bodyFontSize = 16;
    const auto result = rui::RuiTheme(dark, typography, []() {
        const auto scheme = rui::currentColorScheme();
        const auto type = rui::currentTypography();
        check(scheme.background == rui::Color::grey900(), "Theme provides the selected color scheme");
        check(type.bodyFontSize == 16, "Theme provides the selected typography");
        return 42;
    });
    check(result == 42, "Theme returns the content result");
    check(rui::currentColorScheme().background == rui::Color::white(),
          "Theme restores the outer color scheme");
}

void themeDefaultsAreStableForComponents()
{
    const auto scheme = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(scheme.onBackground != dark.onBackground,
          "Light and dark themes provide distinct content colors");
    check(rui::Typography{}.bodyFontSize > 0,
          "Theme typography provides a usable body size");
}

void buttonThemeUsesPrimaryColors()
{
    const auto light = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(light.primary != dark.primary, "Light and dark themes provide distinct button colors");
    check(light.onPrimary != dark.onPrimary, "Light and dark themes provide distinct button content colors");
}

void dynamicLazyListStateSupportsKeysAndEmptyData()
{
    auto values = rui::lazyListItems(std::vector<int>{10, 20});
    check(values != nullptr, "Dynamic LazyList state can be created");
    values->set({});
    check(values->get().empty(), "Dynamic LazyList state supports empty data");
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
    keyedRememberChecksKeyIdentity();
    rememberSaveableSavesAndRestoresState();
    rememberSaveableSupportsCustomSaver();
    keyedRememberSaveableRestoresByKey();
    keyedRememberSaveableHandlesHashCollisions();
    derivedStateTracksDependencies();
    nestedDerivedStateRestoresDependencyTracking();
    snapshotFlowReplaysOnlyToNewCollectors();
    snapshotFlowStopsTrackingWithoutCollectors();
    flowHandleCanOutliveFlow();
    lazyListScopeBuildsComposeStyleItems();
    lazyListItemsProvidesObservableData();
    vectorStateCanBeObserved();
    themeProvidesNestedColorAndTypography();
    themeDefaultsAreStableForComponents();
    buttonThemeUsesPrimaryColors();
    dynamicLazyListStateSupportsKeysAndEmptyData();
    flowCollectorCanRegisterDuringEmission();
    flowCollectorHandleCancelsSubscription();
    flowCollectorCanBeCancelledMoreThanOnce();
    std::cout << "RUI core state tests passed\n";
    return EXIT_SUCCESS;
}
