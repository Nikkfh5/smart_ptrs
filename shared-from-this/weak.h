#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
private:
    template <class U>
    friend class SharedPtr;
    T* ptr_ = nullptr;
    ControlBlockBase* block_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : ptr_(nullptr), block_(nullptr){};

    WeakPtr(const WeakPtr& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->ref_cnt_weak++;
        }
    };
    WeakPtr(WeakPtr&& other) {
        ptr_ = std::move(other.ptr_);
        block_ = std::move(other.block_);
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    };

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_ != nullptr) {
            block_->ref_cnt_weak++;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        WeakPtr tmp(other);
        Swap(tmp);
        return *this;
    };
    WeakPtr& operator=(WeakPtr&& other) {
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

    ~WeakPtr() {
        if (block_ && --block_->ref_cnt_weak == 0 && block_->ref_cnt_shared == 0) {
            delete block_;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ && --block_->ref_cnt_weak == 0 && block_->ref_cnt_shared == 0) {
            delete block_;
        }
        ptr_ = nullptr;
        block_ = nullptr;
    };
    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        return block_ ? block_->ref_cnt_shared : 0;
    }
    bool Expired() const {
        return UseCount() == 0;
    };
    SharedPtr<T> Lock() const {
        if (!block_ || block_->ref_cnt_shared == 0) {
            return SharedPtr<T>();
        }
        return SharedPtr<T>(*this);
    }
};
