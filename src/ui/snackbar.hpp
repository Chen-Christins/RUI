// snackbar.hpp
#pragma once

#include <memory>
#include <string>

#include "component.hpp"
#include "modifier.hpp"
#include "../core/observable_state.hpp"

namespace rui {

enum class SnackbarDuration { Short, Long, Indefinite };

class SnackbarHostState {
public:
    SnackbarHostState();

    void showSnackbar(const std::string& message, SnackbarDuration duration = SnackbarDuration::Short);

    std::shared_ptr<MutableState<std::string>> messageState() const { return message_; }
    std::shared_ptr<MutableState<int>> generationState() const { return generation_; }
    SnackbarDuration duration() const { return duration_; }

private:
    std::shared_ptr<MutableState<std::string>> message_;
    std::shared_ptr<MutableState<int>> generation_;
    SnackbarDuration duration_ = SnackbarDuration::Short;
};

Component SnackbarHost(
    std::shared_ptr<SnackbarHostState> hostState,
    Modifier modifier = Modifier{}
);

} // namespace rui
