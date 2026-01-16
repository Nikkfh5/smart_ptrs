#pragma once

#include <type_traits>
#include <utility>

template <class T, std::size_t Index, bool EBO = (std::is_empty_v<T> && !std::is_final_v<T>)>
class Slot {};
// с ебо
template <class T, std::size_t Index>
class Slot<T, Index, true> : T {
public:
    template <class U>
    Slot(U&& u) : T(std::forward<U>(u)) {
    }

    T& Get() {
        return *this;
    }
    const T& Get() const {
        return *this;
    }
};

// без ебо
template <class T, std::size_t Index>
class Slot<T, Index, false> {
public:
    Slot() = default;

    template <class U>
    Slot(U&& u) : value_(std::forward<U>(u)) {
    }

    T& Get() {
        return value_;
    }
    const T& Get() const {
        return value_;
    }

private:
    T value_{};
};

template <class First, class Second>
class CompressedPair : Slot<First, 0>, Slot<Second, 1> {
public:
    CompressedPair() = default;
    template <class Fargs, class Sargs>
    CompressedPair(Fargs&& f, Sargs&& s)
        : Slot<First, 0>(std::forward<Fargs>(f)), Slot<Second, 1>(std::forward<Sargs>(s)) {
    }

    First& GetFirst() {
        return static_cast<Slot<First, 0>&>(*this).Get();
    }
    const First& GetFirst() const {
        return static_cast<const Slot<First, 0>&>(*this).Get();
    }

    Second& GetSecond() {
        return static_cast<Slot<Second, 1>&>(*this).Get();
    }
    const Second& GetSecond() const {
        return static_cast<const Slot<Second, 1>&>(*this).Get();
    }
};
