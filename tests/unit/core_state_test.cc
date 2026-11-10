#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "core/composable.hpp"
#include "core/derived_state.hpp"
#include "core/observable_state.hpp"

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void mutableStateNotifiesOnlyWhenValueChanges() {
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

void composableContextInvalidatesAndRemembers() {
    auto context = std::make_shared<rui::ComposableContext>();
    int recompositions = 0;
    context->setRecomposer([&]() { ++recompositions; });

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

void keyedRememberKeepsValuesAcrossReset() {
    rui::ComposableContext context;

    int& first = context.remember(std::string("counter"), 7);
    context.reset();
    int& second = context.remember(std::string("counter"), 99);

    check(&first == &second, "Keyed remember preserves the keyed value");
    check(second == 7, "Keyed remember does not replace the existing value");
}

void derivedStateTracksDependencies() {
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

} // namespace

int main() {
    mutableStateNotifiesOnlyWhenValueChanges();
    composableContextInvalidatesAndRemembers();
    keyedRememberKeepsValuesAcrossReset();
    derivedStateTracksDependencies();
    std::cout << "RUI core state tests passed\n";
    return EXIT_SUCCESS;
}
