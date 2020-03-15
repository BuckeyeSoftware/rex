#ifndef RX_CORE_CONCURRENCY_MSVC_ATOMIC_H
#define RX_CORE_CONCURRENCY_MSVC_ATOMIC_H
#if defined(RX_COMPILER_MSVC)

#include <atomic>

#include "rx/core/traits/remove_const.h"

namespace rx::concurrency::detail {
    template <typename T>
    using atomic_base = std::atomic<T>;

    inline std::memory_order to_memory_order(const memory_order order) {
        switch (order) {
        case memory_order::k_relaxed:
            return std::memory_order::memory_order_relaxed;

        case memory_order::k_consume:
            return std::memory_order::memory_order_consume;

        case memory_order::k_acquire:
            return std::memory_order::memory_order_acquire;

        case memory_order::k_release:
            return std::memory_order::memory_order_release;

        case memory_order::k_acq_rel:
            return std::memory_order::memory_order_acq_rel;

        case memory_order::k_seq_cst:
            return std::memory_order::memory_order_seq_cst;

        default:
            return std::memory_order::memory_order_relaxed;
        }
    }

    inline void atomic_thread_fence(memory_order _order) {}

    inline void atomic_signal_fence(memory_order _order) {}

    template <typename T>
    inline void atomic_init(volatile atomic_base<T>* base_, T _value) {
        base_->store(_value);
    }

    template <typename T>
    inline void atomic_init(atomic_base<T>* base_, T _value) {
        base_->store(_value);
    }

    template <typename T>
    inline void atomic_store(volatile atomic_base<T>* base_, T _value, memory_order _order) {
        base_->store(_value, to_memory_order(_order));
    }

    template <typename T>
    inline void atomic_store(atomic_base<T>* base_, T _value, memory_order _order) {
        base_->store(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_load(const volatile atomic_base<T>* base_, memory_order _order) {
        return base_->load(to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_load(const atomic_base<T>* base_, memory_order _order) {
        return base_->load(to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_exchange(volatile atomic_base<T>* base_, T _value, memory_order _order) {
        return base_->exchange(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_exchange(atomic_base<T>* base_, T _value, memory_order _order) {
        return base_->exchange(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(volatile atomic_base<T>* base_, T* _expected, T _value, memory_order _order) {
        return base_->compare_exchange_strong(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(atomic_base<T>* base_, T* _expected, T _value, memory_order _order) {
        return base_->compare_exchange_strong(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(volatile atomic_base<T>* base_, T* _expected, T _value, memory_order _order) {
        return base_->compare_exchange_weak(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(atomic_base<T>* base_, T* _expected, T _value, memory_order _order) {
        return base_->compare_exchange_weak(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_add(volatile atomic_base<T>* base_, T _delta, memory_order _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_add(atomic_base<T>* base_, T _delta, memory_order _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_add(volatile atomic_base<T>* base_, rx_ptrdiff _delta, memory_order _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_add(atomic_base<T>* base_, rx_ptrdiff _delta, memory_order _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_sub(volatile atomic_base<T>* base_, T _delta, memory_order _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_sub(atomic_base<T>* base_, T _delta, memory_order _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_sub(volatile atomic_base<T>* base_, rx_ptrdiff _delta, memory_order _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_sub(atomic_base<T>* base_, rx_ptrdiff _delta, memory_order _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_and(volatile atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_and(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_and(atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_and(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_or(volatile atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_or(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_or(atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_or(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_xor(volatile atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_xor(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_xor(atomic_base<T>* base_, T _pattern, memory_order _order) {
        return base_->fetch_xor(_pattern, to_memory_order(_order));
    }

} // namespace rx::concurrency::detail

#endif // defined(RX_COMPILER_MSVC)
#endif // RX_CORE_CONCURRENCY_MSVC_ATOMIC_H