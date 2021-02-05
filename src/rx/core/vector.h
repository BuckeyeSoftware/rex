#ifndef RX_CORE_VECTOR_H
#define RX_CORE_VECTOR_H
#include "rx/core/array.h"
#include "rx/core/optional.h"

#include "rx/core/concepts/trivially_copyable.h"
#include "rx/core/concepts/trivially_destructible.h"

#include "rx/core/traits/invoke_result.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/restrict.h"

#include "rx/core/memory/system_allocator.h"

namespace Rx {

namespace detail {
  RX_API void copy(void *RX_HINT_RESTRICT dst_, const void* RX_HINT_RESTRICT _src, Size _size);
}

// 32-bit: 16 bytes
// 64-bit: 32 bytes
template<typename T>
struct Vector {
  template<typename U, Size E>
  using Initializers = Array<U[E]>;

  constexpr Vector();
  constexpr Vector(Memory::Allocator& _allocator);
  constexpr Vector(Memory::View _view);

  // Construct a vector from an array of initializers. This is similar to
  // how initializer_list works in C++11 except it requires no compiler proxy
  // and is actually faster since the initializer Type can be moved.
  template<typename U, Size E>
  Vector(Memory::Allocator& _allocator, Initializers<U, E>&& _initializers);
  template<typename U, Size E>
  Vector(Initializers<U, E>&& _initializers);

  Vector(Memory::Allocator& _allocator, Size _size);
  Vector(Memory::Allocator& _allocator, const Vector& _other);
  Vector(Size _size);
  Vector(const Vector& _other);
  Vector(Vector&& other_);

  ~Vector();

  Vector& operator=(const Vector& _other);
  Vector& operator=(Vector&& other_);

  T& operator[](Size _index);
  const T& operator[](Size _index) const;

  // resize to |size| with |value| for new objects
  bool resize(Size _size, const T& _value = {});

  // reserve |size| elements
  [[nodiscard]] bool reserve(Size _size);

  [[nodiscard]] bool append(const Vector& _other);

  void clear();

  Optional<Size> find(const T& _value) const;

  template<typename F>
  Optional<Size> find_if(F&& _compare) const;

  // append |data| by copy
  bool push_back(const T& _data);
  // append |data| by move
  bool push_back(T&& data_);

  void pop_back();

  // append new |T| construct with |args|
  template<typename... Ts>
  bool emplace_back(Ts&&... _args);

  Size size() const;
  Size capacity() const;

  bool in_range(Size _index) const;
  bool is_empty() const;

  template<typename F>
  bool each_fwd(F&& _func);
  template<typename F>
  bool each_fwd(F&& _func) const;

  void erase(Size _from, Size _to);

  // first or last element
  const T& first() const;
  T& first();
  const T& last() const;
  T& last();

  const T* data() const;
  T* data();

  constexpr Memory::Allocator& allocator() const;

  Memory::View disown();

private:
  // NOTE(dweiler): This does not adjust |m_size|, it only adjusts |m_capacity|.
  bool grow_or_shrink_to(Size _size);

  Memory::Allocator* m_allocator;
  T* m_data;
  Size m_size;
  Size m_capacity;
};

template<typename T>
constexpr Vector<T>::Vector()
  : Vector{Memory::SystemAllocator::instance()}
{
}

template<typename T>
constexpr Vector<T>::Vector(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{0}
  , m_capacity{0}
{
}

template<typename T>
constexpr Vector<T>::Vector(Memory::View _view)
  : m_allocator{_view.owner}
  , m_data{reinterpret_cast<T*>(_view.data)}
  , m_size{_view.size / sizeof(T)}
  , m_capacity{_view.capacity / sizeof(T)}
{
}

template<typename T>
template<typename U, Size E>
Vector<T>::Vector(Memory::Allocator& _allocator, Initializers<U, E>&& _initializers)
  : Vector{_allocator}
{
  grow_or_shrink_to(E);
  if constexpr (Concepts::TriviallyCopyable<T>) {
    detail::copy(m_data, _initializers.data(), sizeof(T) * E);
  } else for (Size i = 0; i < E; i++) {
    Utility::construct<T>(m_data + i, Utility::move(_initializers[i]));
  }
  m_size = E;
}

template<typename T>
template<typename U, Size E>
Vector<T>::Vector(Initializers<U, E>&& _initializers)
  : Vector{Memory::SystemAllocator::instance(), Utility::move(_initializers)}
{
}

template<typename T>
Vector<T>::Vector(Memory::Allocator& _allocator, Size _size)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{_size}
  , m_capacity{_size}
{
  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), m_size));
  RX_ASSERT(m_data, "out of memory");

  // TODO(dweiler): is_trivial trait so we can memset this.
  for (Size i = 0; i < m_size; i++) {
    Utility::construct<T>(m_data + i);
  }
}

template<typename T>
Vector<T>::Vector(Memory::Allocator& _allocator, const Vector& _other)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{_other.m_size}
  , m_capacity{_other.m_capacity}
{
  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), _other.m_capacity));
  RX_ASSERT(m_data, "out of memory");

  if (m_size) {
    if constexpr (Concepts::TriviallyCopyable<T>) {
      detail::copy(m_data, _other.m_data, _other.m_size * sizeof *m_data);
    } else for (Size i = 0; i < m_size; i++) {
      Utility::construct<T>(m_data + i, _other.m_data[i]);
    }
  }
}

template<typename T>
Vector<T>::Vector(Size _size)
  : Vector{Memory::SystemAllocator::instance(), _size}
{
}

template<typename T>
Vector<T>::Vector(const Vector& _other)
  : Vector{*_other.m_allocator, _other}
{
}

template<typename T>
Vector<T>::Vector(Vector&& other_)
  : m_allocator{other_.m_allocator}
  , m_data{Utility::exchange(other_.m_data, nullptr)}
  , m_size{Utility::exchange(other_.m_size, 0)}
  , m_capacity{Utility::exchange(other_.m_capacity, 0)}
{
}

template<typename T>
Vector<T>::~Vector() {
  clear();
  allocator().deallocate(m_data);
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector& _other) {
  RX_ASSERT(&_other != this, "self assignment");

  clear();
  allocator().deallocate(m_data);

  m_size = _other.m_size;
  m_capacity = _other.m_capacity;

  m_data = reinterpret_cast<T*>(allocator().allocate(sizeof(T), _other.m_capacity));
  RX_ASSERT(m_data, "out of memory");

  if (m_size) {
    if constexpr (Concepts::TriviallyCopyable<T>) {
      detail::copy(m_data, _other.m_data, _other.m_size * sizeof *m_data);
    } else for (Size i = 0; i < m_size; i++) {
      Utility::construct<T>(m_data + i, _other.m_data[i]);
    }
  }

  return *this;
}

template<typename T>
Vector<T>& Vector<T>::operator=(Vector&& other_) {
  RX_ASSERT(&other_ != this, "self assignment");

  clear();
  allocator().deallocate(m_data);

  m_allocator = other_.m_allocator;
  m_data = Utility::exchange(other_.m_data, nullptr);
  m_size = Utility::exchange(other_.m_size, 0);
  m_capacity = Utility::exchange(other_.m_capacity, 0);

  return *this;
}

template<typename T>
T& Vector<T>::operator[](Size _index) {
  RX_ASSERT(m_data && in_range(_index), "out of bounds (%zu >= %zu)", _index, m_size);
  return m_data[_index];
}

template<typename T>
const T& Vector<T>::operator[](Size _index) const {
  RX_ASSERT(m_data && in_range(_index), "out of bounds (%zu >= %zu)", _index, m_size);
  return m_data[_index];
}

template<typename T>
bool Vector<T>::grow_or_shrink_to(Size _size) {
  if (!reserve(_size)) {
    return false;
  }

  if constexpr (!Concepts::TriviallyDestructible<T>) {
    for (Size i = m_size; i > _size; --i) {
      Utility::destruct<T>(m_data + (i - 1));
    }
  }

  return true;
}

template<typename T>
bool Vector<T>::resize(Size _size, const T& _value) {
  if (!grow_or_shrink_to(_size)) {
    return false;
  }

  // Copy construct the objects.
  for (Size i{m_size}; i < _size; i++) {
    if constexpr (Concepts::TriviallyCopyable<T>) {
      m_data[i] = _value;
    } else {
      Utility::construct<T>(m_data + i, _value);
    }
  }

  m_size = _size;
  return true;
}

template<typename T>
bool Vector<T>::reserve(Size _size) {
  if (_size <= m_capacity) {
    return true;
  }

  // Always resize capacity with the Golden ratio.
  Size capacity = m_capacity;
  while (capacity < _size) {
    capacity = ((capacity + 1) * 3) / 2;
  }

  T* resize = nullptr;
  if constexpr (Concepts::TriviallyCopyable<T>) {
    resize = reinterpret_cast<T*>(allocator().reallocate(m_data, capacity * sizeof *m_data));
  } else {
    resize = reinterpret_cast<T*>(allocator().allocate(sizeof(T), capacity));
  }

  if (RX_HINT_UNLIKELY(!resize)) {
    return false;
  }

  if (m_data) {
    if constexpr (!Concepts::TriviallyCopyable<T>) {
      for (Size i = 0; i < m_size; i++) {
        Utility::construct<T>(resize + i, Utility::move(*(m_data + i)));
        Utility::destruct<T>(m_data + i);
      }
      allocator().deallocate(m_data);
    }
  }

  m_data = resize;
  m_capacity = capacity;

  return true;
}

template<typename T>
bool Vector<T>::append(const Vector& _other) {
  const auto new_size = m_size + _other.m_size;

  // Slight optimization for trivially copyable |T|.
  if constexpr (Concepts::TriviallyCopyable<T>) {
    const auto old_size = m_size;
    if (!resize(new_size)) {
      return false;
    }
    // Use fast copy.
    detail::copy(m_data + old_size, _other.m_data, sizeof(T) * _other.m_size);
  } else {
    if (!reserve(new_size)) {
      return false;
    }

    // Copy construct the objects.
    for (Size i = 0; i < _other.m_size; i++) {
      Utility::construct<T>(m_data + m_size + i, _other[i]);
    }

    m_size = new_size;
  }

  return true;
}

template<typename T>
void Vector<T>::clear() {
  if (m_size) {
    if constexpr (!Concepts::TriviallyDestructible<T>) {
      RX_ASSERT(m_data, "m_data == nullptr when m_size != 0");
      for (Size i = m_size - 1; i < m_size; i--) {
        Utility::destruct<T>(m_data + i);
      }
    }
  }
  m_size = 0;
}

template<typename T>
Optional<Size> Vector<T>::find(const T& _value) const {
  for (Size i = 0; i < m_size; i++) {
    if (m_data[i] == _value) {
      return i;
    }
  }

  return nullopt;
}

template<typename T>
template<typename F>
Optional<Size> Vector<T>::find_if(F&& _compare) const {
  for (Size i = 0; i < m_size; i++) {
    if (_compare(m_data[i])) {
      return i;
    }
  }
  return nullopt;
}

template<typename T>
bool Vector<T>::push_back(const T& _value) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Copy construct object.
  Utility::construct<T>(m_data + m_size, _value);

  m_size++;
  return true;
}

template<typename T>
bool Vector<T>::push_back(T&& value_) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Move construct object.
  Utility::construct<T>(m_data + m_size, Utility::move(value_));

  m_size++;
  return true;
}

template<typename T>
void Vector<T>::pop_back() {
  RX_ASSERT(m_size, "empty vector");
  grow_or_shrink_to(m_size - 1);
  m_size--;
}

template<typename T>
template<typename... Ts>
bool Vector<T>::emplace_back(Ts&&... _args) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // Forward construct object.
  Utility::construct<T>(m_data + m_size, Utility::forward<Ts>(_args)...);

  m_size++;
  return true;
}

template<typename T>
RX_HINT_FORCE_INLINE Size Vector<T>::size() const {
  return m_size;
}

template<typename T>
RX_HINT_FORCE_INLINE Size Vector<T>::capacity() const {
  return m_capacity;
}

template<typename T>
RX_HINT_FORCE_INLINE bool Vector<T>::is_empty() const {
  return m_size == 0;
}

template<typename T>
RX_HINT_FORCE_INLINE bool Vector<T>::in_range(Size _index) const {
  return _index < m_size;
}

template<typename T>
template<typename F>
bool Vector<T>::each_fwd(F&& _func) {
  using ReturnType = Traits::InvokeResult<F, T&>;
  for (Size i{0}; i < m_size; i++) {
    if constexpr (Traits::IS_SAME<ReturnType, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
template<typename F>
bool Vector<T>::each_fwd(F&& _func) const {
  using ReturnType = Traits::InvokeResult<F, const T&>;
  for (Size i{0}; i < m_size; i++) {
    if constexpr (Traits::IS_SAME<ReturnType, bool>) {
      if (!_func(m_data[i])) {
        return false;
      }
    } else {
      _func(m_data[i]);
    }
  }
  return true;
}

template<typename T>
void Vector<T>::erase(Size _from, Size _to) {
  const Size range{_to - _from};
  T* begin{m_data};
  T* end{m_data + m_size};
  T* first{begin + _from};
  T* last{begin + _to};

  for (T* value{last}, *dest{first}; value != end; ++value, ++dest) {
    *dest = Utility::move(*value);
  }

  if constexpr (!Concepts::TriviallyDestructible<T>) {
    for (T* value{end-range}; value < end; ++value) {
      Utility::destruct<T>(value);
    }
  }

  m_size -= range;
}

template<typename T>
RX_HINT_FORCE_INLINE const T& Vector<T>::first() const {
  RX_ASSERT(m_data, "empty vector");
  return m_data[0];
}

template<typename T>
RX_HINT_FORCE_INLINE T& Vector<T>::first() {
  RX_ASSERT(m_data, "empty vector");
  return m_data[0];
}

template<typename T>
RX_HINT_FORCE_INLINE const T& Vector<T>::last() const {
  RX_ASSERT(m_data, "empty vector");
  return m_data[m_size - 1];
}

template<typename T>
RX_HINT_FORCE_INLINE T& Vector<T>::last() {
  RX_ASSERT(m_data, "empty vector");
  return m_data[m_size - 1];
}

template<typename T>
RX_HINT_FORCE_INLINE const T* Vector<T>::data() const {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE T* Vector<T>::data() {
  return m_data;
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Vector<T>::allocator() const {
  return *m_allocator;
}

template<typename T>
Memory::View Vector<T>::disown() {
  return {
    m_allocator,
    reinterpret_cast<Byte*>(Utility::exchange(m_data, nullptr)),
    Utility::exchange(m_size, 0) * sizeof(T),
    Utility::exchange(m_capacity, 0) * sizeof(T)
  };
}

} // namespace Rx

#endif // RX_CORE_VECTOR_H
