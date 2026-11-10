// composable.cc
#include "composable.hpp"

namespace rui {

void ComposableContext::invalidate()
{
    if (needRecompose_) {
        return;
    }
    needRecompose_ = true;
    if (recomposer_) {
        recomposer_();
    }
}

bool ComposableContext::needsRecompose() const
{
    return needRecompose_;
}

void ComposableContext::markComposed()
{
    needRecompose_ = false;
}

void ComposableContext::setRecomposer(std::function<void()> recomposer)
{
    recomposer_ = std::move(recomposer);
}

void ComposableContext::reset()
{
    needRecompose_ = false;
    ++generation_;
    rememberSlot_ = 0;
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
