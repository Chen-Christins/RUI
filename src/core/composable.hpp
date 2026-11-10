// composable.hpp
#pragma once

#include <functional>
#include <any>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "observable_state.hpp"

namespace rui {

class ComposableContext;

template <typename T, typename Saved = T>
struct Saver {
    std::function<Saved(const T&)> save;
    std::function<T(const Saved&)> restore;
};

template <typename T>
Saver<T, T> identitySaver()
{
    return Saver<T, T>{
        [](const T& value) { return value; },
        [](const T& value) { return value; }
    };
}

class ComposableContext : public std::enable_shared_from_this<ComposableContext> {
public:
    struct SavedValue {
        std::type_index keyType = typeid(void);
        std::shared_ptr<void> key;
        std::function<bool(const void*)> matches;
        std::any value;
    };
    using SavedState = std::vector<SavedValue>;

    void invalidate();

    bool needsRecompose() const;

    void markComposed();

    template <typename T>
    void observe(std::shared_ptr<MutableState<T>> state);

    template <typename T>
    T& remember(T&& initialValue);

    template <typename K, typename T>
    T& remember(const K& key, T&& initialValue);

    template <typename T, typename Saved = T>
    T& rememberSaveable(T initialValue, Saver<T, Saved> saver = identitySaver<T>());

    template <typename K, typename T, typename Saved = T>
    T& rememberSaveable(const K& key, T initialValue, Saver<T, Saved> saver = identitySaver<T>());

    SavedState saveState() const;
    void restoreState(SavedState state);

    void setRecomposer(std::function<void()> recomposer);

    void reset();

    uint64_t generation() const;

    static ComposableContext* current();

    static void setCurrent(ComposableContext* ctx);

private:
    struct KeyedEntry {
        size_t key = 0;
        std::type_index keyType = typeid(void);
        std::shared_ptr<void> keyValue;
        std::function<bool(const void*)> equals;
        std::shared_ptr<void> value;
        std::type_index valueType = typeid(void);
    };

    struct SaveableEntry {
        size_t keyHash = 0;
        std::type_index keyType = typeid(void);
        std::shared_ptr<void> keyValue;
        std::function<bool(const void*)> equals;
        std::shared_ptr<void> value;
        std::function<std::any()> save;
        std::type_index valueType = typeid(void);
    };

    struct RememberedEntry {
        std::shared_ptr<void> value;
        std::type_index valueType = typeid(void);
    };

    mutable std::mutex mutex_;
    bool needRecompose_ = false;
    uint64_t generation_ = 0;
    size_t rememberSlot_ = 0;
    std::function<void()> recomposer_;
    std::vector<RememberedEntry> remembered_;
    std::vector<KeyedEntry> keyed_;
    std::vector<SaveableEntry> saveables_;
    SavedState pendingSavedState_;
    std::vector<ObserverHandle> observations_;
    inline static thread_local ComposableContext* currentContext_ = nullptr;
};

inline ComposableContext* requireCurrentComposableContext()
{
    auto* context = ComposableContext::current();
    if (!context) {
        throw std::logic_error("remember requires an active ComposableContext");
    }
    return context;
}

template <typename T>
void ComposableContext::observe(std::shared_ptr<MutableState<T>> state)
{
    auto weak = std::weak_ptr<ComposableContext>(shared_from_this());
    auto gen = generation();
    auto handle = state->observe([weak, gen](const T&) {
        if (const auto ctx = weak.lock()) {
            if (ctx->generation() == gen) {
                ctx->invalidate();
            }
        }
    });
    bool stale = false;
    {
        std::lock_guard lock(mutex_);
        if (generation_ != gen) {
            stale = true;
        } else {
            observations_.push_back(std::move(handle));
        }
    }
    if (stale) {
        handle.cancel();
    }
}

template <typename T>
T& ComposableContext::remember(T&& initialValue)
{
    std::lock_guard lock(mutex_);
    if (rememberSlot_ < remembered_.size()) {
        const auto& entry = remembered_[rememberSlot_];
        if (entry.valueType != std::type_index(typeid(std::decay_t<T>))) {
            throw std::logic_error("remember slot type changed during recomposition");
        }
        rememberSlot_++;
        return *static_cast<std::decay_t<T>*>(entry.value.get());
    }
    auto value = std::make_shared<std::decay_t<T>>(std::forward<T>(initialValue));
    remembered_.push_back(RememberedEntry{value, std::type_index(typeid(std::decay_t<T>))});
    rememberSlot_++;
    return *static_cast<std::decay_t<T>*>(value.get());
}

template <typename K, typename T>
T& ComposableContext::remember(const K& key, T&& initialValue)
{
    std::lock_guard lock(mutex_);
    using Storage = std::decay_t<T>;
    using KeyStorage = std::decay_t<K>;
    const size_t hash = std::hash<KeyStorage>{}(key);
    for (const auto& entry : keyed_) {
        if (entry.key == hash && entry.keyType == std::type_index(typeid(KeyStorage)) &&
            entry.equals && entry.equals(&key)) {
            if (entry.valueType != std::type_index(typeid(Storage))) {
                throw std::logic_error("keyed remember value type changed");
            }
            return *static_cast<Storage*>(entry.value.get());
        }
    }
    auto keyValue = std::make_shared<KeyStorage>(key);
    auto value = std::make_shared<Storage>(std::forward<T>(initialValue));
    keyed_.push_back(KeyedEntry{
        hash,
        std::type_index(typeid(KeyStorage)),
        keyValue,
        [keyValue](const void* candidate) {
            return *keyValue == *static_cast<const KeyStorage*>(candidate);
        },
        value
        , std::type_index(typeid(Storage))
    });
    return *static_cast<Storage*>(value.get());
}

template <typename T, typename Saved>
T& ComposableContext::rememberSaveable(T initialValue, Saver<T, Saved> saver)
{
    std::lock_guard lock(mutex_);
    using Storage = std::decay_t<T>;
    using SavedStorage = std::decay_t<Saved>;
    const size_t slot = rememberSlot_++;
    if (slot < saveables_.size()) {
        if (saveables_[slot].valueType != std::type_index(typeid(Storage))) {
            throw std::logic_error("rememberSaveable value type changed");
        }
        return *static_cast<Storage*>(saveables_[slot].value.get());
    }

    auto value = std::make_shared<Storage>(std::move(initialValue));
    for (const auto& pending : pendingSavedState_) {
        if (pending.keyType == std::type_index(typeid(size_t)) && pending.matches &&
            pending.matches(&slot) && saver.restore) {
            if (const auto* saved = std::any_cast<SavedStorage>(&pending.value)) {
                *value = saver.restore(*saved);
            }
            break;
        }
    }
    saveables_.push_back(SaveableEntry{
        slot,
        std::type_index(typeid(size_t)),
        std::make_shared<size_t>(slot),
        [slot](const void* candidate) {
            return slot == *static_cast<const size_t*>(candidate);
        },
        value,
        [value, save = std::move(saver.save)]() -> std::any {
            return save ? std::any(save(*value)) : std::any();
        },
        std::type_index(typeid(Storage))
    });
    return *value;
}

template <typename K, typename T, typename Saved>
T& ComposableContext::rememberSaveable(const K& key, T initialValue, Saver<T, Saved> saver)
{
    std::lock_guard lock(mutex_);
    using Storage = std::decay_t<T>;
    using KeyStorage = std::decay_t<K>;
    using SavedStorage = std::decay_t<Saved>;
    const size_t hash = std::hash<KeyStorage>{}(key);
    for (auto& entry : saveables_) {
        if (entry.keyHash == hash && entry.keyType == std::type_index(typeid(KeyStorage)) &&
            entry.equals && entry.equals(&key)) {
            if (entry.valueType != std::type_index(typeid(Storage))) {
                throw std::logic_error("keyed rememberSaveable value type changed");
            }
            return *static_cast<Storage*>(entry.value.get());
        }
    }

    auto keyValue = std::make_shared<KeyStorage>(key);
    auto value = std::make_shared<Storage>(std::move(initialValue));
    for (const auto& pending : pendingSavedState_) {
        if (pending.keyType == std::type_index(typeid(KeyStorage)) && pending.matches &&
            pending.matches(&key) && saver.restore) {
            if (const auto* saved = std::any_cast<SavedStorage>(&pending.value)) {
                *value = saver.restore(*saved);
            }
            break;
        }
    }
    saveables_.push_back(SaveableEntry{
        hash,
        std::type_index(typeid(KeyStorage)),
        keyValue,
        [keyValue](const void* candidate) {
            return *keyValue == *static_cast<const KeyStorage*>(candidate);
        },
        value,
        [value, save = std::move(saver.save)]() -> std::any {
            return save ? std::any(save(*value)) : std::any();
        },
        std::type_index(typeid(Storage))
    });
    return *value;
}

template <typename T>
T& remember(T&& initialValue)
{
    return requireCurrentComposableContext()->remember<T>(std::forward<T>(initialValue));
}

template <typename K, typename T>
T& remember(const K& key, T&& initialValue)
{
    return requireCurrentComposableContext()->remember(key, std::forward<T>(initialValue));
}

template <typename T, typename Saved = T>
T& rememberSaveable(T initialValue, Saver<T, Saved> saver = identitySaver<T>())
{
    return requireCurrentComposableContext()->rememberSaveable(std::move(initialValue), std::move(saver));
}

template <typename K, typename T, typename Saved = T>
T& rememberSaveable(const K& key, T initialValue, Saver<T, Saved> saver = identitySaver<T>())
{
    return requireCurrentComposableContext()->rememberSaveable(key, std::move(initialValue), std::move(saver));
}

template <typename T>
void observe(std::shared_ptr<MutableState<T>> state)
{
    if (auto* ctx = ComposableContext::current()) {
        ctx->observe(state);
    }
}

} // namespace rui
