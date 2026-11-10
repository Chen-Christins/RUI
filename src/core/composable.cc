// composable.cc
#include "composable.hpp"

namespace rui {

void ComposableContext::invalidate()
{
    std::function<void()> recomposer;
    {
        std::lock_guard lock(mutex_);
        if (needRecompose_) {
            return;
        }
        needRecompose_ = true;
        recomposer = recomposer_;
    }
    if (recomposer) {
        recomposer();
    }
}

bool ComposableContext::needsRecompose() const
{
    std::lock_guard lock(mutex_);
    return needRecompose_;
}

void ComposableContext::markComposed()
{
    std::lock_guard lock(mutex_);
    needRecompose_ = false;
}

void ComposableContext::setRecomposer(std::function<void()> recomposer)
{
    std::lock_guard lock(mutex_);
    recomposer_ = std::move(recomposer);
}

void ComposableContext::reset()
{
    std::vector<ObserverHandle> observations;
    {
        std::lock_guard lock(mutex_);
        needRecompose_ = false;
        ++generation_;
        rememberSlot_ = 0;
        observations = std::move(observations_);
    }
    for (auto& observation : observations) {
        observation.cancel();
    }
}

uint64_t ComposableContext::generation() const
{
    std::lock_guard lock(mutex_);
    return generation_;
}

ComposableContext* ComposableContext::current()
{
    return currentContext_;
}

void ComposableContext::setCurrent(ComposableContext* ctx)
{
    currentContext_ = ctx;
}

} // namespace rui
