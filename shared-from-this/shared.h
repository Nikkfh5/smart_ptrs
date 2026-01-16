#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <memory>
#include <optional>
#include <type_traits>

struct ESFTBase;
template <typename T>
class EnableSharedFromThis;
struct ControlBlockBase {
    size_t ref_cnt_shared = 1;
    size_t ref_cnt_weak = 1;
    virtual ~ControlBlockBase() noexcept = default;
    virtual void DestroyShared() noexcept = 0;
};

template <class Y, class Deleter = std::default_delete<Y>>
struct ControlBlockPtr : ControlBlockBase {
    Y* ptr_;
    Deleter deleter_;
    explicit ControlBlockPtr(Y* ptr, Deleter deleter = Deleter{})
        : ptr_(ptr), deleter_(std::move(deleter)) {
    }
    void DestroyShared() noexcept override {
        if (ptr_) {
            deleter_(ptr_);
            ptr_ = nullptr;
        }
    }
};
template <class Y>
struct ControlBlockInplace : ControlBlockBase {
    std::optional<Y> obj;

    template <class... Args>
    explicit ControlBlockInplace(Args&&... a) : obj(std::in_place, std::forward<Args>(a)...) {
    }
    void DestroyShared() noexcept override {
        obj.reset();
    }
};
struct ESFTBase {
    virtual void Bind(ControlBlockBase* cb, void* owner) const noexcept = 0;
    virtual ~ESFTBase() = default;
};
// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
private:
    template <class U>
    friend class EnableSharedFromThis;
    template <class U>
    friend class SharedPtr;
    template <class U>
    friend class WeakPtr;
    template <class U, class... A>
    friend SharedPtr<U> MakeShared(A&&...);
    template <class U>
    void MaybeInitWeakThis(U* ptr) {
        if constexpr (std::is_convertible_v<U*, ESFTBase*>) {
            static_cast<ESFTBase*>(ptr)->Bind(block_, ptr);
        }
    }
    T* ptr_ = nullptr;
    ControlBlockBase* block_ = nullptr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), block_(nullptr){};
    SharedPtr(std::nullptr_t) : ptr_(nullptr), block_(nullptr){};
    explicit SharedPtr(T* ptr) {
        if (ptr == nullptr) {
            ptr_ = nullptr;
            block_ = nullptr;
        } else {
            block_ = new ControlBlockPtr<T>(ptr);
            ptr_ = ptr;
            MaybeInitWeakThis(ptr);
        }
    };

    SharedPtr(const SharedPtr& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->ref_cnt_shared++;
        }
    };
    SharedPtr(SharedPtr&& other) {
        ptr_ = std::move(other.ptr_);
        block_ = std::move(other.block_);
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    };
    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) noexcept {
        ptr_ = std::move(other.ptr_);
        block_ = std::move(other.block_);
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }
    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        if (!ptr) {
            ptr_ = nullptr;
            block_ = nullptr;
        } else {
            block_ = new ControlBlockPtr<Y>(ptr);
            ptr_ = static_cast<T*>(ptr);
            MaybeInitWeakThis(ptr);
        }
    }
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->ref_cnt_shared++;
        }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        ptr_ = ptr;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->ref_cnt_shared++;
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (!other.block_ || other.block_->ref_cnt_shared == 0) {
            throw BadWeakPtr();
        }
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->ref_cnt_shared++;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr tmp(other);
        Swap(tmp);
        return *this;
    };
    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_ && --block_->ref_cnt_shared == 0) {
            block_->DestroyShared();
            if (--block_->ref_cnt_weak == 0) {
                delete block_;
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ && --block_->ref_cnt_shared == 0) {
            block_->DestroyShared();
            if (--block_->ref_cnt_weak == 0) {
                delete block_;
            }
        }
        ptr_ = nullptr;
        block_ = nullptr;
    };
    template <typename Y>
    void Reset(Y* ptr) {
        if (block_ && --block_->ref_cnt_shared == 0) {
            block_->DestroyShared();
            if (--block_->ref_cnt_weak == 0) {
                delete block_;
            }
        }
        block_ = nullptr;
        ptr_ = nullptr;
        if (ptr) {
            block_ = new ControlBlockPtr<Y>(ptr);
            ptr_ = ptr;
            MaybeInitWeakThis(ptr);
        }
    }

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    };
    T& operator*() const {
        return *ptr_;
    };
    T* operator->() const {
        return ptr_;
    };
    size_t UseCount() const {
        return block_ ? block_->ref_cnt_shared : 0;
    };
    explicit operator bool() const {
        return ptr_ != nullptr;
    };
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
};

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto* block = new ControlBlockInplace<T>(std::forward<Args>(args)...);
    SharedPtr<T> sp;
    sp.block_ = block;
    sp.ptr_ = &*block->obj;
    sp.MaybeInitWeakThis(sp.ptr_);
    return sp;
};

// Look for usage examples in tests
#include "weak.h"
template <typename T>
class EnableSharedFromThis : public ESFTBase {
private:
    template <class U>
    friend class SharedPtr;
    mutable WeakPtr<T> weak_this_;
    void Bind(ControlBlockBase* cb, void* owner) const noexcept override {
        if (!cb) {
            return;
        }
        ++cb->ref_cnt_shared;
        SharedPtr<T> now;
        now.block_ = cb;
        now.ptr_ = static_cast<T*>(owner);

        weak_this_ = WeakPtr<T>(now);
    }

public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    };
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_this_);
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    };
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    };
};
