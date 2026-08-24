// coroutine_scope.cc
#include "coroutine_scope.hpp"

namespace rui {

CoroutineScope::CoroutineScope() : token_(std::make_shared<CancellationToken>()) {}

CoroutineScope::~CoroutineScope() {
    cancel();
}

void CoroutineScope::launch(Job job) {
    if (!job.valid()) {
        return;
    }
    job.handle().promise().token = token_;
    job.resume();
    jobs_.push_back(std::move(job));
}

void CoroutineScope::cancel() {
    token_->cancel();
    for (auto& job : jobs_) {
        job.destroy();
    }
    jobs_.clear();
    token_ = std::make_shared<CancellationToken>();
}

} // namespace rui
