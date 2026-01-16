#pragma once

#include "compressed_pair.h"
#include <utility>
#include <algorithm>
#include <cstddef>  // std::nullptr_t
#include <type_traits>

struct Slug {
    template <class U>
    void operator()(U* p) const {
        delete p;
    }
};

// Primary template
template <typename T, typename Deleter = Slug>
class UniquePtr {
private:
    CompressedPair<T*, Deleter> p_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : p_(ptr, Deleter()){};
    UniquePtr(T* ptr, Deleter deleter) : p_(ptr, std::move(deleter)){};

    UniquePtr(UniquePtr&& other) noexcept : p_(other.Release(), std::move(other.GetDeleter())) {
    }
    template <class U, class D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept
        : p_(other.Release(), std::move(other.GetDeleter())) {
    }

    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    UniquePtr& operator=(UniquePtr& other) = delete;
    UniquePtr& operator=(UniquePtr&& other) noexcept {

        if (this == &other) {
            return *this;
        }
        Reset();
        p_.GetSecond() = std::move(other.GetDeleter());
        p_.GetFirst() = other.Release();
        return *this;
    };
    template <class U, class D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        Reset();
        p_.GetSecond() = std::move(other.GetDeleter());
        p_.GetFirst() = other.Release();
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (p_.GetFirst()) {
            p_.GetSecond()(p_.GetFirst());
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        return std::exchange(p_.GetFirst(), nullptr);
    };
    void Reset(T* ptr = nullptr) {
        if (ptr == p_.GetFirst()) {
            return;
        }
        UniquePtr<T, Deleter> tmp(p_.GetFirst(), std::move(p_.GetSecond()));
        p_.GetFirst() = ptr;
        p_.GetSecond() = std::move(tmp.GetDeleter());
    };
    void Swap(UniquePtr& other) {
        std::swap(p_.GetFirst(), other.p_.GetFirst());
        std::swap(p_.GetSecond(), other.p_.GetSecond());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return p_.GetFirst();
    };
    Deleter& GetDeleter() {
        return p_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return p_.GetSecond();
    };
    explicit operator bool() const {
        return p_.GetFirst() != nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *(p_.GetFirst());
    };
    T* operator->() const {
        return p_.GetFirst();
    };
};
template <typename Deleter>
class UniquePtr<void, Deleter> {
private:
    CompressedPair<void*, Deleter> p_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(void* ptr = nullptr) : p_(ptr, Deleter()){};
    UniquePtr(void* ptr, Deleter deleter) : p_(ptr, std::move(deleter)){};
    UniquePtr(UniquePtr&& other) noexcept : p_(other.Release(), std::move(other.GetDeleter())) {
    }

    template <class U, class D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept
        : p_(other.Release(), std::move(other.GetDeleter())) {
    }

    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    UniquePtr& operator=(UniquePtr& other) = delete;
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        p_.GetSecond() = std::move(other.GetDeleter());
        p_.GetFirst() = other.Release();
        return *this;
    };
    template <class U, class D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        if (this != &other) {
            Reset();
            p_.GetFirst() = other.Release();
            p_.GetSecond() = std::move(other.GetDeleter());
        }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (p_.GetFirst()) {
            p_.GetSecond()(p_.GetFirst());
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void* Release() {
        return std::exchange(p_.GetFirst(), nullptr);
    };
    void Reset(void* ptr = nullptr) {
        if (ptr == p_.GetFirst()) {
            return;
        }
        UniquePtr<void, Deleter> tmp(p_.GetFirst(), std::move(p_.GetSecond()));
        p_.GetFirst() = ptr;
        p_.GetSecond() = std::move(tmp.GetDeleter());
    };
    void Swap(UniquePtr& other) {
        std::swap(p_.GetFirst(), other.Get());
        std::swap(p_.GetSecond(), other.GetDeleter());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    void* Get() const {
        return p_.GetFirst();
    };
    Deleter& GetDeleter() {
        return p_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return p_.GetSecond();
    };
    explicit operator bool() const {
        return p_.GetFirst() != nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
private:
    CompressedPair<T*, Deleter> p_;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : p_(ptr, Deleter()){};
    UniquePtr(T* ptr, Deleter deleter) : p_(ptr, std::move(deleter)){};

    UniquePtr(UniquePtr&& other) noexcept : p_(other.Release(), std::move(other.GetDeleter())) {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        p_.GetFirst() = other.Release();
        p_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    };
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (p_.GetFirst()) {
            if (std::is_same_v<Deleter, Slug>) {
                delete[] p_.GetFirst();
            } else {
                p_.GetSecond()(p_.GetFirst());
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        return std::exchange(p_.GetFirst(), nullptr);
    };
    void Reset(T* ptr = nullptr) {
        if (ptr == p_.GetFirst()) {
            return;
        }
        UniquePtr<T[], Deleter> tmp(p_.GetFirst(), std::move(p_.GetSecond()));
        p_.GetFirst() = ptr;
        p_.GetSecond() = std::move(tmp.GetDeleter());
    };
    void Swap(UniquePtr& other) {
        std::swap(p_.GetFirst(), other.Get());
        std::swap(p_.GetSecond(), other.GetDeleter());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return p_.GetFirst();
    };
    Deleter& GetDeleter() {
        return p_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return p_.GetSecond();
    };
    explicit operator bool() const {
        return p_.GetFirst() != nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators
    T& operator[](size_t i) const {
        return p_.GetFirst()[i];
    }
};
