#pragma once

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

#include <catch.hpp>

namespace allocations_checker {
inline std::atomic<std::size_t> alloc_count{0};

struct Guard {
    std::size_t before;
    Guard() : before(alloc_count.load(std::memory_order_relaxed)) {}
    std::size_t Delta() const {
        return alloc_count.load(std::memory_order_relaxed) - before;
    }
};
}

inline void* operator new(std::size_t n) {
    allocations_checker::alloc_count.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(n)) {
        return p;
    }
    throw std::bad_alloc();
}

inline void operator delete(void* p) noexcept {
    std::free(p);
}

inline void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

#define EXPECT_ZERO_ALLOCATIONS(stmt)        \
    do {                                     \
        allocations_checker::Guard g;        \
        stmt;                                \
        REQUIRE(g.Delta() == 0);             \
    } while (false)

#define EXPECT_ONE_ALLOCATION(stmt)          \
    do {                                     \
        allocations_checker::Guard g;        \
        stmt;                                \
        REQUIRE(g.Delta() == 1);             \
    } while (false)
