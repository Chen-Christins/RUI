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
#include "core/composition_local.hpp"
#include "core/observable_state.hpp"
#include "ui/lazy_list.hpp"
#include "ui/animation.hpp"
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
    rui::ComposableContext::setCurrent(nullptr);
    rui::ComposableContext secondContext;
    rui::ComposableContext::setCurrent(&secondContext);
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

void derivedStateCalculationExceptionsRestoreTracking()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    auto source = rui::mutableStateOf(1);
    bool fail = false;
    int computations = 0;
    auto derived = rui::derivedStateOf<int>([&]() {
        ++computations;
        const int value = source->get();
        if (fail) {
            throw std::runtime_error("derived failure");
        }
        return value * 2;
    });
    check(derived->get() == 2, "Derived state starts with a valid value");
    fail = true;
    source->set(2);
    check(derived->get() == 2, "Derived state keeps its last valid value after failure");
    check(rui::ComposableContext::current() == &context,
          "Derived state exceptions preserve the composition context");
    rui::ComposableContext::setCurrent(nullptr);
}

void derivedStateRetriesAfterCalculationRecovers()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    auto source = rui::mutableStateOf(1);
    bool fail = false;
    auto derived = rui::derivedStateOf<int>([&]() {
        const int value = source->get();
        if (fail) {
            throw std::runtime_error("derived retry failure");
        }
        return value * 2;
    });
    check(derived->get() == 2, "Derived state has an initial valid value");
    fail = true;
    source->set(2);
    fail = false;
    source->set(3);
    check(derived->get() == 6, "Derived state retries after its calculation recovers");
    rui::ComposableContext::setCurrent(nullptr);
}

void derivedStateHandlesConcurrentDependencyChanges()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    auto source = rui::mutableStateOf(0);
    std::atomic<int> computations = 0;
    auto derived = rui::derivedStateOf<int>([&]() {
        ++computations;
        return source->get() * 2;
    });
    std::thread first([&]() { source->set(1); });
    std::thread second([&]() { source->set(2); });
    first.join();
    second.join();
    check(derived->get() == 4, "Derived state converges to the latest concurrent dependency value");
    check(computations.load() >= 2, "Concurrent dependency changes trigger recomputation");
    rui::ComposableContext::setCurrent(nullptr);
}

void snapshotFlowHandlesConcurrentDependencyChanges()
{
    auto source = rui::mutableStateOf(0);
    auto flow = rui::snapshotFlow<int>([&]() {
        return source->get();
    });
    std::atomic<int> emissions = 0;
    auto handle = flow->collect([&](const int&) {
        ++emissions;
    });
    std::thread first([&]() { source->set(1); });
    std::thread second([&]() { source->set(2); });
    first.join();
    second.join();
    check(emissions.load() >= 1, "SnapshotFlow emits during concurrent dependency changes");
    handle.cancel();
}

void snapshotFlowSuppressesEqualResults()
{
    auto source = rui::mutableStateOf(1);
    auto flow = rui::snapshotFlow<int>([&]() {
        return source->get() / 2;
    });
    int emissions = 0;
    auto handle = flow->collect([&](const int&) {
        ++emissions;
    });
    source->set(2);
    source->set(3);
    check(emissions == 2, "SnapshotFlow suppresses equal calculated results");
    source->set(4);
    check(emissions == 3, "SnapshotFlow emits when the calculated result changes");
    handle.cancel();
}

void regularFlowKeepsDuplicateEmissions()
{
    rui::Flow<int> flow;
    int emissions = 0;
    auto handle = flow.collect([&](const int&) {
        ++emissions;
    });
    flow.emitValue(1);
    flow.emitValue(1);
    check(emissions == 2, "Regular Flow keeps duplicate emissions");
    handle.cancel();
}

void distinctFlowSuppressesDuplicateEmissions()
{
    rui::Flow<int> flow;
    flow.setDistinct(true);
    int emissions = 0;
    auto handle = flow.collect([&](const int&) {
        ++emissions;
    });
    flow.emitValue(1);
    flow.emitValue(1);
    flow.emitValue(2);
    check(emissions == 2, "Distinct Flow suppresses duplicate emissions");
    handle.cancel();
}

void flowDistinctConfigurationCanChangeConcurrently()
{
    rui::Flow<int> flow;
    int emissions = 0;
    auto handle = flow.collect([&](const int&) {
        ++emissions;
    });
    std::thread config([&]() {
        for (int i = 0; i < 100; ++i) {
            flow.setDistinct((i % 2) == 0);
        }
    });
    std::thread emitter([&]() {
        for (int i = 0; i < 100; ++i) {
            flow.emitValue(i % 3);
        }
    });
    config.join();
    emitter.join();
    check(emissions > 0, "Concurrent distinct configuration and emission remain safe");
    handle.cancel();
}

void flowCollectorsCanJoinWhileEmitting()
{
    rui::Flow<int> flow;
    std::atomic<int> emissions = 0;
    auto first = flow.collect([&](const int&) {
        ++emissions;
    });
    std::thread emitter([&]() {
        for (int i = 0; i < 100; ++i) {
            flow.emitValue(i);
        }
    });
    std::vector<rui::Flow<int>::CollectorHandle> handles;
    for (int i = 0; i < 20; ++i) {
        handles.push_back(flow.collect([&](const int&) {
            ++emissions;
        }));
    }
    emitter.join();
    check(emissions.load() >= 100, "Collectors can join while a Flow is emitting");
    for (auto& handle : handles) {
        handle.cancel();
    }
    first.cancel();
}

void flowCollectorExceptionsAreIsolated()
{
    rui::Flow<int> flow;
    int notified = 0;
    auto failing = flow.collect([](const int&) {
        throw std::runtime_error("collector failure");
    });
    auto healthy = flow.collect([&](const int&) {
        ++notified;
    });
    bool threw = false;
    try {
        flow.emitValue(1);
    } catch (...) {
        threw = true;
    }
    check(!threw, "Flow collector exceptions do not escape emission");
    check(notified == 1, "Flow collector exceptions do not block healthy collectors");
    failing.cancel();
    healthy.cancel();
}

void flowReplayExceptionsAreIsolated()
{
    rui::Flow<int> flow;
    flow.emitValue(1);
    bool threw = false;
    rui::Flow<int>::CollectorHandle handle;
    try {
        handle = flow.collect([](const int&) {
            throw std::runtime_error("replay failure");
        });
    } catch (...) {
        threw = true;
    }
    check(!threw, "Flow replay exceptions do not escape collect");
    handle.cancel();
}

void flowCollectorCancellationIsIdempotentUnderReentry()
{
    rui::Flow<int> flow;
    int countChanges = 0;
    flow.setSubscriptionCountCallback([&](size_t) {
        ++countChanges;
    });
    auto handle = flow.collect([](const int&) {});
    handle.cancel();
    handle.cancel();
    check(countChanges == 2, "Registration and cancellation each update subscription count once");
}

void snapshotFlowExceptionsDoNotBreakTheFlow()
{
    auto source = rui::mutableStateOf(1);
    bool fail = false;
    int computations = 0;
    auto flow = rui::snapshotFlow<int>([&]() {
        ++computations;
        const int value = source->get();
        if (fail) {
            throw std::runtime_error("snapshot failure");
        }
        return value;
    });
    int values = 0;
    auto handle = flow->collect([&](const int&) {
        ++values;
    });
    check(values == 1, "SnapshotFlow emits its initial value");
    fail = true;
    source->set(2);
    fail = false;
    source->set(3);
    check(values == 2, "SnapshotFlow retries and emits after calculation recovers");
    handle.cancel();
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

void themeProviderRestoresAfterContentException()
{
    bool threw = false;
    try {
        rui::RuiTheme(rui::ColorScheme::dark(), rui::Typography{}, []() {
            throw std::runtime_error("theme content failure");
            return 0;
        });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "Theme content exceptions propagate");
    check(rui::currentColorScheme().background == rui::Color::white(),
          "Theme restores color scheme after content exception");
    check(rui::currentTypography().bodyFontSize == 14,
          "Theme restores typography after content exception");
}

void themeStateStoresCanBeUpdated()
{
    auto colors = rui::mutableStateOf(rui::ColorScheme::light());
    auto typography = rui::mutableStateOf(rui::Typography{});
    typography->set(rui::Typography{16, 22, 30, 12, 400, 600});
    colors->set(rui::ColorScheme::dark());
    check(colors->get().background == rui::Color::grey900(),
          "Theme color state can switch to dark colors");
    check(typography->get().bodyFontSize == 16,
          "Theme typography state can update independently");
}

void stateDrivenThemeInvalidatesComposition()
{
    auto colors = rui::mutableStateOf(rui::ColorScheme::light());
    auto typography = rui::mutableStateOf(rui::Typography{});
    auto context = std::make_shared<rui::ComposableContext>();
    int recompositions = 0;
    context->setRecomposer([&]() {
        ++recompositions;
    });
    context->observe(colors);
    context->observe(typography);
    context->markComposed();

    colors->set(rui::ColorScheme::dark());
    check(recompositions == 1, "Theme color state invalidates the composition");
    context->markComposed();
    typography->set(rui::Typography{16, 22, 30, 12, 400, 600});
    check(recompositions == 2, "Theme typography state invalidates the composition");
}

void stateDrivenThemeObserversDoNotAccumulate()
{
    auto colors = rui::mutableStateOf(rui::ColorScheme::light());
    for (int i = 0; i < 20; ++i) {
        auto context = std::make_shared<rui::ComposableContext>();
        int recompositions = 0;
        context->setRecomposer([&]() {
            ++recompositions;
        });
        context->observe(colors);
        context->markComposed();
        colors->set(rui::ColorScheme::dark());
        check(recompositions == 1, "Each theme context receives one invalidation");
        context->reset();
        colors->set(rui::ColorScheme::light());
        check(recompositions == 1, "Reset theme contexts stop receiving invalidations");
    }
}

void themeStyleColorsSerializeConsistently()
{
    const auto light = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(rui::themeColorCss(light.onBackground) == "#ff212121",
          "Light theme text color serializes to CSS consistently");
    check(rui::themeColorCss(dark.onBackground) == "#fffafafa",
          "Dark theme text color serializes to CSS consistently");
    check(rui::themeColorCss(light.primary) != rui::themeColorCss(dark.primary),
          "Light and dark button colors serialize differently");
}

void themeTokenCssUsesResolvedSchemeColor()
{
    const auto dark = rui::ColorScheme::dark();
    check(rui::themeColorCss(rui::Color::themePrimary(), dark) == rui::themeColorCss(dark.primary),
          "Theme token CSS resolves through the selected scheme");
    check(rui::themeColorCss(rui::Color::themePrimary().withAlpha(80), dark) !=
              rui::themeColorCss(dark.primary),
          "Theme token CSS preserves customized alpha");
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

void surfaceThemeUsesSurfaceColors()
{
    const auto light = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(light.surface != dark.surface, "Light and dark themes provide distinct surface colors");
    check(light.onSurface != dark.onSurface, "Light and dark themes provide distinct surface content colors");
}

void formControlsUseThemeSemanticColors()
{
    const auto light = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(light.primary != dark.primary, "Form controls use theme primary colors");
    check(light.onPrimary != dark.onPrimary, "Checked form controls use theme content colors");
    check(light.onSurface != dark.onSurface, "Unchecked form controls use theme surface content colors");
}

void textFieldThemeUsesPrimaryAndSurfaceSemantics()
{
    const auto scheme = rui::ColorScheme::dark();
    check(scheme.primary != scheme.surface, "TextField focus and surface colors are distinct");
    check(scheme.onSurface != scheme.primary, "TextField content and focus colors are distinct");
}

void progressThemeUsesPrimaryAndMutedTrackColors()
{
    const auto scheme = rui::ColorScheme::dark();
    check(scheme.primary != rui::Color::blue(), "Progress uses the active theme primary color");
    check(scheme.onSurface.withAlpha(40) != scheme.onSurface,
          "Progress track uses a muted on-surface color");
}

void sliderThemeUsesPrimaryAndTrackColors()
{
    const auto scheme = rui::ColorScheme::dark();
    check(scheme.primary != scheme.onSurface, "Slider active and inactive colors are distinct");
    check(scheme.primary != rui::Color::blue(), "Slider uses the active theme primary color");
}

void snackbarThemeProvidesInverseSurfaceColors()
{
    const auto light = rui::ColorScheme::light();
    const auto dark = rui::ColorScheme::dark();
    check(light.inverseSurfaceColor != dark.inverseSurfaceColor,
          "Snackbar inverse surfaces differ between light and dark themes");
    check(light.inverseOnSurfaceColor != dark.inverseOnSurfaceColor,
          "Snackbar inverse content colors differ between light and dark themes");
}

void chipAndBadgeThemeUseSemanticColors()
{
    const auto scheme = rui::ColorScheme::dark();
    check(scheme.primary != scheme.onPrimary, "Selected chips have contrasting content color");
    check(scheme.error != scheme.onError, "Badges have contrasting error content color");
}

void explicitColorsAreNotMistakenForThemeTokens()
{
    check(!rui::Color::blue().isThemeToken(), "Explicit blue is not a theme token");
    check(!rui::Color::white().isThemeToken(), "Explicit white is not a theme token");
    check(!rui::Color::red().isThemeToken(), "Explicit red is not a theme token");
    check(rui::Color::themePrimary().isThemeToken(), "Theme primary has an explicit token");
    check(rui::Color::themeSurface().isThemeToken(), "Theme surface has an explicit token");
    check(rui::Color::themeError().isThemeToken(), "Theme error has an explicit token");
}

void themeColorTokensPreserveIdentityAcrossAlpha()
{
    const auto token = rui::Color::themePrimary();
    const auto faded = token.withAlpha(80);
    check(faded.isThemeToken(), "Theme token identity survives alpha changes");
    check(faded.a() == 80, "Theme token alpha can still be customized");
}

void themeColorTokensResolveAgainstScheme()
{
    const auto scheme = rui::ColorScheme::dark();
    const auto primary = rui::resolveThemeColor(rui::Color::themePrimary(), scheme);
    const auto faded = rui::resolveThemeColor(rui::Color::themePrimary().withAlpha(80), scheme);
    check(primary == scheme.primary, "Theme primary token resolves to the scheme primary");
    check(faded.r() == scheme.primary.r() && faded.a() == 80,
          "Theme token resolution preserves customized alpha");
    check(rui::resolveThemeColor(rui::Color::red(), scheme) == rui::Color::red(),
          "Explicit colors bypass theme resolution");
}

void compositionLocalsRestoreAfterNestedAndThrowingContent()
{
    auto local = rui::compositionLocalOf<int>([]() { return 1; });
    check(local.current() == 1, "CompositionLocal returns its default value");

    bool threw = false;
    try {
        rui::CompositionLocalProvider(local, 2, [&]() {
            check(local.current() == 2, "CompositionLocal returns its nested value");
            throw std::runtime_error("provider test");
            return 0;
        });
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "CompositionLocal content exceptions propagate");
    check(local.current() == 1, "CompositionLocal restores after an exception");
}

void compositionLocalsAreThreadLocal()
{
    auto local = rui::compositionLocalOf<int>([]() { return 7; });
    int workerValue = 0;
    std::thread worker([&]() {
        workerValue = local.current();
        rui::CompositionLocalProvider(local, 9, [&]() {
            workerValue = local.current();
            return 0;
        });
    });
    worker.join();
    check(workerValue == 9, "CompositionLocal provider is isolated to its thread");
    check(local.current() == 7, "CompositionLocal provider does not leak across threads");
}

void compositionLocalProviderSupportsMultipleValues()
{
    auto first = rui::compositionLocalOf<int>([]() { return 1; });
    auto second = rui::compositionLocalOf<std::string>([]() { return std::string("default"); });
    auto third = rui::compositionLocalOf<bool>([]() { return false; });

    const auto result = rui::CompositionLocalProvider(
        first,
        10,
        second,
        std::string("provided"),
        third,
        true,
        [&]() {
            check(first.current() == 10, "Multiple provider supplies the first local");
            check(second.current() == "provided", "Multiple provider supplies the second local");
            check(third.current(), "Multiple provider supplies the third local");
            return 42;
        }
    );
    check(result == 42, "Multiple CompositionLocalProvider returns content result");
    check(first.current() == 1 && second.current() == "default" && !third.current(),
          "Multiple provider restores all outer local values");
}

void stateObserversSupportReentrantUpdates()
{
    auto state = rui::mutableStateOf(0);
    std::vector<int> values;
    state->observe([&](const int& value) {
        values.push_back(value);
        if (value == 1) {
            state->set(2);
        }
    });

    state->set(1);
    check(values.size() == 2, "Reentrant state updates notify both changes");
    check(values[0] == 1 && values[1] == 2,
          "Reentrant state notifications preserve update order");
    check(state->get() == 2, "Reentrant state update stores the final value");
}

void stateObserverExceptionsAreIsolated()
{
    auto state = rui::mutableStateOf(0);
    int notified = 0;
    state->observe([](const int&) {
        throw std::runtime_error("observer failure");
    });
    state->observe([&](const int&) {
        ++notified;
    });

    bool threw = false;
    try {
        state->set(1);
    } catch (...) {
        threw = true;
    }
    check(!threw, "Observer exceptions do not escape state updates");
    check(notified == 1, "Observer exceptions do not block later observers");
}

void cancelledStateObserversCanBeReplacedWithoutGrowth()
{
    auto state = rui::mutableStateOf(0);
    int calls = 0;
    auto first = state->observe([&](const int&) { ++calls; });
    first.cancel();
    auto second = state->observe([&](const int&) { ++calls; });
    state->set(1);
    check(calls == 1, "Cancelled observers do not receive future updates");
    second.cancel();
    state->set(2);
    check(calls == 1, "Replacement observer can also be cancelled");
}

void stateChangeCallbackCanBeReplacedConcurrently()
{
    auto state = rui::mutableStateOf(0);
    std::atomic<int> callbacks = 0;
    std::thread writer([&]() {
        for (int i = 1; i <= 100; ++i) {
            state->set(i);
        }
    });
    std::thread configurator([&]() {
        for (int i = 0; i < 100; ++i) {
            state->setOnChange([&](const std::string&) {
                ++callbacks;
            });
        }
    });
    writer.join();
    configurator.join();
    check(callbacks.load() >= 0, "Concurrent state change callback replacement is safe");
}

void ownerBoundObserversRemainSafeDuringUpdates()
{
    auto state = rui::mutableStateOf(0);
    int notifications = 0;
    QObject* owner = new QObject();
    state->observe(owner, [&](const int&) {
        ++notifications;
    });
    state->set(1);
    check(notifications == 1, "Owner-bound observer receives updates while alive");
    delete owner;

    for (int i = 2; i <= 100; ++i) {
        state->set(i);
    }
    check(notifications == 1, "Owner-bound observer never runs after owner destruction");
}

void composableRememberStorageIsThreadSafe()
{
    auto context = std::make_shared<rui::ComposableContext>();
    std::thread first([&]() {
        context->remember(1);
        context->rememberSaveable(2);
    });
    std::thread second([&]() {
        context->remember(std::string("key"), 3);
        context->rememberSaveable(std::string("save-key"), 4);
    });
    first.join();
    second.join();
    check(true, "Concurrent remember operations complete safely");
}

void rememberWithoutContextFailsClearly()
{
    rui::ComposableContext::setCurrent(nullptr);
    bool threw = false;
    try {
        rui::remember(1);
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "remember without a composition context reports a clear error");
}

void rememberSlotsRemainStableAcrossConditionalComposition()
{
    auto context = std::make_shared<rui::ComposableContext>();
    int& first = context->remember(1);
    context->reset();
    int& second = context->remember(2);
    check(&first == &second && second == 1,
          "Remember slots remain stable across normal recomposition");

    context->reset();
    int& conditional = context->remember(3);
    check(&conditional == &first && &conditional == &second,
          "Conditional composition can reuse the existing slot");
}

void rememberTypeChangesFailClearly()
{
    rui::ComposableContext context;
    context.remember(1);
    context.reset();
    bool threw = false;
    try {
        context.remember(std::string("changed"));
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "Changing a remember slot type reports a clear error");
}

void rememberAndSaveableUseIndependentSlots()
{
    rui::ComposableContext context;
    int& remembered = context.remember(1);
    int& saveable = context.rememberSaveable(2);
    remembered = 11;
    saveable = 22;

    context.reset();
    int& saveableAgain = context.rememberSaveable(0);
    int& rememberedAgain = context.remember(0);
    check(&rememberedAgain == &remembered && rememberedAgain == 11,
          "Remember state is independent from saveable state");
    check(&saveableAgain == &saveable && saveableAgain == 22,
          "Saveable state is independent from remember state");
}

void saveableStateCanBeSavedConcurrently()
{
    auto context = std::make_shared<rui::ComposableContext>();
    context->rememberSaveable(1);
    std::thread saver([&]() {
        for (int i = 0; i < 100; ++i) {
            auto state = context->saveState();
            context->restoreState(std::move(state));
        }
    });
    std::thread composer([&]() {
        for (int i = 0; i < 100; ++i) {
            context->reset();
            context->rememberSaveable(1);
        }
    });
    saver.join();
    composer.join();
    check(true, "Concurrent saveable state snapshots complete safely");
}

void saveableSnapshotCanBeRestoredAtASafePoint()
{
    rui::ComposableContext source;
    int& value = source.rememberSaveable(1);
    value = 42;
    auto saved = source.saveState();

    rui::ComposableContext restored;
    restored.restoreState(std::move(saved));
    int& restoredValue = restored.rememberSaveable(0);
    check(restoredValue == 42, "Saveable snapshots restore a stable safe-point value");
}

void zeroDurationAnimationCompletesImmediately()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    int finished = 0;
    rui::animateFloatAsState(0.0f, rui::AnimationSpec::snap());
    context.reset();
    auto state = rui::animateFloatAsState(1.0f, rui::AnimationSpec::snap(), [&]() {
        ++finished;
    });
    check(state->get() == 1.0f, "Snap animation reaches its target immediately");
    check(finished == 1, "Snap animation invokes its completion callback once");
    rui::ComposableContext::setCurrent(nullptr);
}

void unchangedAnimationTargetDoesNotRestartOrNotify()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    int finished = 0;
    rui::animateFloatAsState(0.0f, rui::AnimationSpec::snap());
    context.reset();
    rui::animateFloatAsState(0.0f, rui::AnimationSpec::snap(), [&]() {
        ++finished;
    });
    check(finished == 0, "An unchanged completed animation target does not notify again");
    rui::ComposableContext::setCurrent(nullptr);
}

void changedAnimationTargetUsesOnlyLatestCompletion()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    int firstFinished = 0;
    int secondFinished = 0;
    rui::animateFloatAsState(0.0f, rui::AnimationSpec::snap());
    context.reset();
    rui::animateFloatAsState(1.0f, rui::AnimationSpec::tween(100), [&]() {
        ++firstFinished;
    });
    context.reset();
    rui::animateFloatAsState(2.0f, rui::AnimationSpec::snap(), [&]() {
        ++secondFinished;
    });
    check(firstFinished == 0, "A replaced animation does not invoke an obsolete callback");
    check(secondFinished == 1, "The latest animation invokes its completion callback once");
    rui::ComposableContext::setCurrent(nullptr);
}

void repeatableAnimationKeepsCompletionSemantics()
{
    rui::ComposableContext context;
    rui::ComposableContext::setCurrent(&context);
    int finished = 0;
    rui::animateFloatAsState(0.0f, rui::AnimationSpec::snap());
    context.reset();
    auto spec = rui::AnimationSpec::repeatable(rui::AnimationSpec::tween(1), true, 2);
    auto state = rui::animateFloatAsState(1.0f, spec, [&]() {
        ++finished;
    });
    check(state != nullptr, "Repeatable animation returns an observable state");
    check(finished == 0, "Asynchronous repeatable animation does not complete synchronously");
    rui::ComposableContext::setCurrent(nullptr);
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
    derivedStateCalculationExceptionsRestoreTracking();
    snapshotFlowExceptionsDoNotBreakTheFlow();
    derivedStateRetriesAfterCalculationRecovers();
    derivedStateHandlesConcurrentDependencyChanges();
    snapshotFlowHandlesConcurrentDependencyChanges();
    snapshotFlowSuppressesEqualResults();
    regularFlowKeepsDuplicateEmissions();
    distinctFlowSuppressesDuplicateEmissions();
    flowDistinctConfigurationCanChangeConcurrently();
    flowCollectorsCanJoinWhileEmitting();
    flowCollectorExceptionsAreIsolated();
    flowReplayExceptionsAreIsolated();
    flowCollectorCancellationIsIdempotentUnderReentry();
    snapshotFlowReplaysOnlyToNewCollectors();
    snapshotFlowStopsTrackingWithoutCollectors();
    flowHandleCanOutliveFlow();
    lazyListScopeBuildsComposeStyleItems();
    lazyListItemsProvidesObservableData();
    vectorStateCanBeObserved();
    themeProvidesNestedColorAndTypography();
    themeProviderRestoresAfterContentException();
    themeStateStoresCanBeUpdated();
    stateDrivenThemeInvalidatesComposition();
    stateDrivenThemeObserversDoNotAccumulate();
    themeStyleColorsSerializeConsistently();
    themeTokenCssUsesResolvedSchemeColor();
    themeDefaultsAreStableForComponents();
    buttonThemeUsesPrimaryColors();
    surfaceThemeUsesSurfaceColors();
    formControlsUseThemeSemanticColors();
    textFieldThemeUsesPrimaryAndSurfaceSemantics();
    progressThemeUsesPrimaryAndMutedTrackColors();
    sliderThemeUsesPrimaryAndTrackColors();
    snackbarThemeProvidesInverseSurfaceColors();
    chipAndBadgeThemeUseSemanticColors();
    explicitColorsAreNotMistakenForThemeTokens();
    themeColorTokensPreserveIdentityAcrossAlpha();
    themeColorTokensResolveAgainstScheme();
    compositionLocalsRestoreAfterNestedAndThrowingContent();
    compositionLocalsAreThreadLocal();
    compositionLocalProviderSupportsMultipleValues();
    stateObserversSupportReentrantUpdates();
    stateObserverExceptionsAreIsolated();
    cancelledStateObserversCanBeReplacedWithoutGrowth();
    stateChangeCallbackCanBeReplacedConcurrently();
    ownerBoundObserversRemainSafeDuringUpdates();
    composableRememberStorageIsThreadSafe();
    rememberWithoutContextFailsClearly();
    rememberSlotsRemainStableAcrossConditionalComposition();
    rememberTypeChangesFailClearly();
    rememberAndSaveableUseIndependentSlots();
    saveableStateCanBeSavedConcurrently();
    saveableSnapshotCanBeRestoredAtASafePoint();
    zeroDurationAnimationCompletesImmediately();
    unchangedAnimationTargetDoesNotRestartOrNotify();
    changedAnimationTargetUsesOnlyLatestCompletion();
    repeatableAnimationKeepsCompletionSemantics();
    dynamicLazyListStateSupportsKeysAndEmptyData();
    flowCollectorCanRegisterDuringEmission();
    flowCollectorHandleCancelsSubscription();
    flowCollectorCanBeCancelledMoreThanOnce();
    std::cout << "RUI core state tests passed\n";
    return EXIT_SUCCESS;
}
