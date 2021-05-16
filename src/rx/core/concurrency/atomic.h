#ifndef RX_CORE_CONCURRENCY_ATOMIC_H
#define RX_CORE_CONCURRENCY_ATOMIC_H
#include "rx/core/config.h" // RX_COMPILER_*
#include "rx/core/types.h"
#include "rx/core/markers.h"

#include "rx/core/concepts/integral.h"
#include "rx/core/concepts/pointer.h"
#include "rx/core/concepts/boolean.h"

/// \file rx/core/concurrency/atomic.h

namespace Rx::Concurrency {

/// Specifies how memory accesses, including regular, non-atomic accesses are to
/// be ordered around an atomic operation.
///
/// The default behavior provided by \ref Atomic is full sequenial consistency.
enum class MemoryOrder : Uint8 {
  /// A relaxed operation: there are no synchronization or ordering constraints
  /// imposed on other reads or writes, only this operation's atomicity is
  /// guaranteed.
  ///
  /// All atomic operations tagged with this order are not synchronization
  /// operations; they do not impose an order among concurrent memory accesses.
  /// They only guarantee atomicity and modification order consistency.
  ///
  /// Typical use of relaxed memory ordering is incrementing counters, such as
  /// reference counters, since this only requires atomicity, but no ordering
  /// or synchronization.
  RELAXED,

  /// A load operation with this memory order performs a _consume operation_ on
  /// the affected memory location: no reads or writes in the current thread
  /// dependent on the value currently loaded can be reordered before this load.
  /// Writes to data-dependent variables in other threads that release the same
  /// atomic variable are visible in the current thread.
  CONSUME,

  /// A load operation with this memory order performs the _acquire operation_
  /// on the affected memory location: no reads or writes in the current thread
  /// can be reordered before this load. All writes in other threads that
  /// release the same atomic variable are visible in the current thread.
  ACQUIRE,

  /// A store operation with this memory order performs the _release operation_
  /// no reads or writes in the current thread can be reordered after this
  /// store. All writes in the current thread are visible in other threads that
  /// acquire the same atomic variable and writes that carry a dependency into
  /// the atomic variable become visible in other threads that consume the same
  /// atomic variable.
  RELEASE,

  /// A read-modify-write operation with this memory order is both an
  /// _acquire operation_ and a _release operation_. No memory reads or writes
  /// in the current thread can be reordered before or after this store. All
  /// writes in other threads that release the same atomic variable are visible
  /// before the modification and the modification is visible in other threads
  /// that acquire the same atomic variable.
  ACQ_REL,

  /// A load operation with this memory order performs an _acquire operation_,
  /// a store performs a _release operation_, and a read-modify-write performs
  /// both an _acquire operation_ and a _release operation_, plus a single
  /// total order exists in which all threads observe all modifications in the
  /// same order.
  SEQ_CST
};

} // namespace Rx::Concurrency

#if defined(RX_COMPILER_GCC)
#include "rx/core/concurrency/gcc/atomic.h"
#elif defined(RX_COMPILER_CLANG)
#include "rx/core/concurrency/clang/atomic.h"
#else
// Use <atomic> as fallback,
#include "rx/core/concurrency/std/atomic.h"
#endif

/// Concurrent facilities and primitives.
namespace Rx::Concurrency {
#if !defined(RX_DOCUMENT)
namespace _ {
template<typename T>
struct AtomicValue : AtomicBase<T> {
  AtomicValue() = default;

  constexpr explicit AtomicValue(T _value)
    : AtomicBase<T>{_value}
  {
  }
};
} // namespace _
#endif

/// \brief Atomic value of any type.
/// \warning Might not be lock-free. Use Atomic<T> for lock-free gurantees.
template<typename T>
struct AtomicValue {
  // RX_MARK_NO_COPY(AtomicValue);

  /// Constructs an atomic value.
  AtomicValue() = default;

  /// Initialize with value.
  /// \param _value The value to initialize with.
  /// \warning The initialization is not atomic.
  constexpr AtomicValue(T _value) : m_value{_value} {}

  /// @{
  /// \brief Atomically replaces the value of the atomic object with a
  /// non-atomic argument.
  ///
  /// Atomically replaces the current value with \p _value. Memory is affected
  /// according to the value of \p _order.
  ///
  /// \warning \p _order Must be one of:
  ///   * MemoryOrder::RELAXED
  ///   * MemoryOrder::RELEASE
  ///   * MemoryOrder::SEQ_CST
  ///
  /// \param _value The value to store into the atomic variable.
  /// \param _order %Memory order constraints to enforce.
  void store(T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    atomic_store(&m_value, _value, _order);
  }

  void store(T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    atomic_store(&m_value, _value, _order);
  }
  /// @}

  /// \brief Atomically obtains the value of the atomic object.
  ///
  /// Atomically loads and returns the current value of the atomic variable.
  /// Memory is affected according to the value of \o _order.
  ///
  /// \warning \p _order Must be one of:
  ///  * MemoryOrder::RELAXED
  ///  * MemoryOrder::CONSUME
  ///  * MemoryOrder::ACQUIRE
  ///  * MemoryOrder::SEQ_CST
  ///
  /// \param _order %Memory order constraints to enforce.
  /// \returns The current value of the atomic variable.
  /// @{
  T load(MemoryOrder _order = MemoryOrder::SEQ_CST) const volatile {
    return atomic_load(&m_value, _order);
  }

  T load(MemoryOrder _order = MemoryOrder::SEQ_CST) const {
    return atomic_load(&m_value, _order);
  }
  /// @}

  /// \brief Loads a value from an atomic object.
  ///
  /// Atomically loads and returns the current value of the atomic variable.
  /// Equivalent to load().
  ///
  /// \returns The current value of the atomic variable.
  /// @{
  operator T() const volatile {
    return load();
  }

  operator T() const {
    return load();
  }
  /// @}

  /// \brief Atomically replaces the value of the atomic object and obtains the
  /// value held previously.
  ///
  /// Atomically replaces the underlying value with \p _value. The operation is
  /// read-modify-write operation. Memory is affected according to the value of
  /// \p _order.
  ///
  /// \param _value The value to assign.
  /// \param _order %Memory order constraints to enforce.
  /// \returns The value of the atomic variable before the call.
  /// @{
  T exchange(T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_exchange(&m_value, _value, _order);
  }

  T exchange(T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_exchange(&m_value, _value, _order);
  }
  /// @}

  /// @{
  /// \brief Atomically compares the value of the atomic object with non-atomic
  /// argument and performs atomic exchange if equal or atomic load if not.
  ///
  /// Atomically compares value representation of \c *this with that of
  /// \p expected_, and if those are bitwise-equal, replaces the former with
  /// \p _value (performs read-modify-write operation.) Otherwise, loads the
  /// actual value stored in \c *this into \p expected_ (load operation.)
  ///
  /// The memory models for the read-modify-write and load operations are
  /// \p _success and \p _failure respectively. In the (3-4) and (7-8) versions
  /// \p _order is used for both read-modify-write and load operations, except
  /// that MemoryOrder::ACQUIRE and MemoryOrder::RELAXED are used for the
  /// load operation if \p _order is MemoryOrder::ACQ_REL, or
  /// MemoryOrder::RELEASE respectively.
  ///
  /// \param expected_ Reference to the value expected to be found in the atomic
  /// object. Gets stored with the actual value of \c *this if the comparison
  /// fails.
  /// \param _value The value to store in the atomic object if it is as
  /// expected.
  /// \param _success The memory synchronization ordering for the
  /// read-modify-write operation if the comparison succeeds. All values are
  /// permitted.
  /// \param _failure The memory synchronization ordering for the load operation
  /// if the comparison fails. Cannot be MemoryOrder::RELEASE or
  /// MemoryOrder::ACQ_REL.
  /// \param _order The memory synchronization ordering for both operations.
  /// \returns If the underlying atomic value was sucessfully changed, \c true.
  /// Otherwise, \c false.
  ///
  /// \note The comparison and copying are bitwise. No constructor, assignment
  /// operator, or comparison operator are used.
  ///
  /// \note The weak forms (1-4) of the functions are allowed to fail
  /// spuriously, that is, act as if `*this != expected_` even if they are
  /// equal.
  bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _success,
    MemoryOrder _failure) volatile
  {
    return atomic_compare_exchange_weak(&m_value, expected_, _value, _success, _failure);
  }

  bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _success,
    MemoryOrder _failure)
  {
    return atomic_compare_exchange_weak(&m_value, expected_, _value, _success, _failure);
  }

  bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_compare_exchange_weak(&m_value, expected_, _value, _order, _order);
  }

  bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_compare_exchange_weak(&m_value, expected_, _value, _order, _order);
  }

  bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _success,
    MemoryOrder _failure) volatile
  {
    return atomic_compare_exchange_strong(&m_value, expected_, _value, _success, _failure);
  }

  bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _success,
    MemoryOrder _failure)
  {
    return atomic_compare_exchange_strong(&m_value, expected_, _value, _success, _failure);
  }

  bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _order) volatile {
    return atomic_compare_exchange_strong(&m_value, expected_, _value, _order, _order);
  }

  bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _order) {
    return atomic_compare_exchange_strong(&m_value, expected_, _value, _order, _order);
  }
  /// @}

protected:
  mutable _::AtomicValue<T> m_value;
};

/// Lock-free atomic type.
template<typename T>
  requires (Concepts::Integral<T> || Concepts::Pointer<T>)
struct Atomic;

/// Specialization for bool.
template<>
struct Atomic<bool> : AtomicValue<bool> {
  /// Default constructor is trivial.
  ///
  /// No initialization takes place other than zero initialization of static and
  /// thread-local objects.
  Atomic() = default;

  /// \brief Initializes the underlying value with \p _value.
  /// \note The initialization is not atomic.
  constexpr Atomic(bool _value) : Type{_value} {}

  /// @{
  /// Stores boolean value into atomic object.
  bool operator=(bool _value) volatile {
    Type::store(_value);
    return _value;
  }

  bool operator=(bool _value) {
    Type::store(_value);
    return _value;
  }
  /// @}

private:
  using Type = AtomicValue<bool>;
};

/// Specialization for integral types.
template<Concepts::Integral T>
struct Atomic<T>
  : AtomicValue<T>
{
  Atomic() = default;
  constexpr Atomic(T _value) : Type{_value} {}

  /// @{
  /// Stores integer value into atomic object.
  T operator=(T _value) volatile {
    Type::store(_value);
    return _value;
  }

  T operator=(T _value) {
    Type::store(_value);
    return _value;
  }
  /// @}

  /// @{
  /// \brief Atomically adds the argument to the value stored in the atomic
  /// object and obtains the value held previously.
  ///
  /// Atomically replaces the current value with the result of artihmetic
  /// addition of the value and \p _delta. The operation is read-modify-write
  /// operation. Memory is affected according to the value of \p _order.
  ///
  /// \param _delta The other argument of artihmetic addition.
  /// \param _order %Memory order constraints to enforce.
  /// \returns The value immediately preceding the effects of this function in
  /// the modification order of \p *this.
  T fetch_add(T _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_fetch_add(&this->m_value, _delta, _order);
  }

  T fetch_add(T _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_fetch_add(&this->m_value, _delta, _order);
  }
  /// @}

  T fetch_sub(T _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T fetch_sub(T _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T fetch_and(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_fetch_and(&this->m_value, _pattern, _order);
  }

  T fetch_and(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_fetch_sub(&this->m_value, _pattern, _order);
  }

  T fetch_or(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_fetch_or(&this->m_value, _pattern, _order);
  }

  T fetch_or(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_fetch_or(&this->m_value, _pattern, _order);
  }

  T fetch_xor(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return atomic_fetch_xor(&this->m_value, _pattern, _order);
  }

  T fetch_xor(T _pattern, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return atomic_fetch_xor(&this->m_value, _pattern, _order);
  }

  // ++, --
  T operator++(int) volatile {
    return fetch_add(T{1});
  }

  T operator++(int) {
    return fetch_add(T{1});
  }

  T operator--(int) volatile {
    return fetch_sub(T{1});
  }

  T operator--(int) {
    return fetch_sub(T{1});
  }

  T operator++() volatile {
    return fetch_add(T{1}) + T{1};
  }

  T operator++() {
    return fetch_add(T{1}) + T{1};
  }

  T operator--() volatile {
    return fetch_sub(T{1}) - T{1};
  }

  T operator--() {
    return fetch_sub(T{1}) - T{1};
  }

  T operator+=(T _delta) volatile {
    return fetch_add(_delta) + _delta;
  }

  T operator+=(T _delta) {
    return fetch_add(_delta) + _delta;
  }

  T operator-=(T _delta) volatile {
    return fetch_sub(_delta) - _delta;
  }

  T operator-=(T _delta) {
    return fetch_sub(_delta) - _delta;
  }

  T operator&=(T _pattern) volatile {
    return fetch_and(_pattern) & _pattern;
  }

  T operator&=(T _pattern) {
    return fetch_and(_pattern) & _pattern;
  }

  T operator|=(T _pattern) volatile {
    return fetch_or(_pattern) | _pattern;
  }

  T operator|=(T _pattern) {
    return fetch_or(_pattern) | _pattern;
  }

  T operator^=(T _pattern) volatile {
    return fetch_xor(_pattern) ^ _pattern;
  }

  T operator^=(T _pattern) {
    return fetch_xor(_pattern) ^ _pattern;
  }

private:
  using Type = AtomicValue<T>;
};

/// Specialization for pointer types.
template<Concepts::Pointer T>
struct Atomic<T*>
  : AtomicValue<T*>
{
  Atomic() = default;
  constexpr Atomic(T* _value) : Type{_value} {}

  T* operator=(T* _value) volatile {
    Type::store(_value);
    return _value;
  }

  T* operator=(T* _value) {
    Type::store(_value);
    return _value;
  }

  T* fetch_add(PtrDiff _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return _::atomic_fetch_add(&this->m_value, _delta, _order);
  }

  T* fetch_add(PtrDiff _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return _::atomic_fetch_add(&this->m_value, _delta, _order);
  }

  T* fetch_sub(PtrDiff _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return _::atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T* fetch_sub(PtrDiff _delta, MemoryOrder _order = MemoryOrder::SEQ_CST) {
    return _::atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T* operator++(int) volatile {
    return fetch_add(1);
  }

  T* operator++(int) {
    return fetch_add(1);
  }

  T* operator--(int) volatile {
    return fetch_sub(1);
  }

  T* operator--(int) {
    return fetch_sub(1);
  }

  T* operator++() volatile {
    return fetch_add(1) + 1;
  }

  T* operator++() {
    return fetch_add(1) + 1;
  }

  T* operator--() volatile {
    return fetch_sub(1) - 1;
  }

  T* operator--() {
    return fetch_sub(1) - 1;
  }

  T* operator+=(PtrDiff _delta) volatile {
    return fetch_add(_delta) + _delta;
  }

  T* operator+=(PtrDiff _delta) {
    return fetch_add(_delta) + _delta;
  }

  T* operator-=(PtrDiff _delta) volatile {
    return fetch_sub(_delta) - _delta;
  }

  T* operator-=(PtrDiff _delta) {
    return fetch_sub(_delta) - _delta;
  }

private:
  using Type = AtomicValue<T*>;
};

/// Lock-free atomic boolean type.
struct AtomicFlag {
  RX_MARK_NO_COPY(AtomicFlag);

  AtomicFlag() = default;
  constexpr AtomicFlag(bool _value) : m_value{_value} {}

  /// Atomically sets the flag to `true` and obtains its previous value.
  /// \param _order The memory synchronization order for this operation.
  bool test_and_set(MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    return _::atomic_exchange(&m_value, true, _order);
  }

  /// Atomically sets the flag to `true` and obtains its previous value.
  bool test_and_set(MemoryOrder _order = MemoryOrder::SEQ_CST)  {
    return _::atomic_exchange(&m_value, true, _order);
  }

  /// Atomically returns the value of the flag.
  void clear(MemoryOrder _order = MemoryOrder::SEQ_CST) volatile {
    _::atomic_store(&m_value, false, _order);
  }

  /// Atomically sets flag to `false`
  void clear(MemoryOrder _order = MemoryOrder::SEQ_CST) {
    _::atomic_store(&m_value, false, _order);
  }

private:
  _::AtomicBase<bool> m_value;
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_ATOMIC_H
