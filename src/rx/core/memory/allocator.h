#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H
#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"

#include "rx/core/markers.h"

/// \file allocator.h

/// Allocators and low-level memory routines.
namespace Rx::Memory {

/// \brief Polymorphic allocator interface.
///
/// All allocations in Rex are associated with a given allocator which must
/// implement this interface.
struct RX_API Allocator {
  RX_MARK_INTERFACE(Allocator);

  /// All allocations must return pointers aligned by this alignment.
  static inline constexpr const Size ALIGNMENT = 16;

  /// \brief Allocate memory.
  /// \param _size The size in bytes.
  /// \return On success, returns the pointer to the beginning of newly
  /// allocated memory. On failure, returns \c nullptr.
  /// \warning The pointer must be deallocated with deallocate().
  /// \warning Allocation of zero \p _size is a bug.
  virtual Byte* allocate(Size _size) = 0;

  /// \brief Reallocates the given memory.
  ///
  /// The memory must be previously allocated by allocate(), or reallocate()
  /// and not yet deallocated with a call to deallocate().
  ///
  /// The reallocation is done by either:
  ///  1. Expanding or contracting the existing area pointed to by \p _data, if
  ///     possible. The contents of the area remain unchanged up to the lesser
  ///     of the new and old sizes. If the area is expanded, the contents of the
  ///     new part of the array are undefined.
  ///  2. Allocating a new memory block of size \p _size bytes, copying the
  ///     memory area with size equal the lesser of the new and old sizes, and
  ///     deallocating the old block.
  ///
  /// If there is not enough memory, the old memory block is not deallocated and
  /// \c nullptr is returned.
  ///
  /// If \p _data is \c nullptr, the behavior is the same as calling allocate()
  /// with \p _size.
  ///
  /// \warning Reallocation of zero \p _size is a bug.
  ///
  /// \param _data Pointer to memory area to be reallocated.
  /// \param _size New size of area in bytes.
  /// \returns On success, returns the pointer to the beginning of newly
  /// allocated memory. On failure, returns \c nullptr and the original pointer
  /// \p _data remains valid and needs to be deallocated with deallocate().
  virtual Byte* reallocate(void* _data, Size _size) = 0;

  /// \brief Deallocates memory.
  ///
  /// Deallocates the space previously allocated by allocate() or reallocate().
  /// \param _data Pointer to the memory to deallocate.
  /// \note The function accepts (and does nothing with) the null pointer to
  /// reduce the amount of special-casing. Whether allocations succeeds or not,
  /// the pointer returned by an allocation can be passed to deallocate().
  virtual void deallocate(void* _data) = 0;

  /// \brief Create an object on this allocator.
  ///
  /// Allocates memory large enough for an object and constructs it in place,
  /// returning a pointer of the object type to that constructed object.
  ///
  /// \param _arguments The constructor arguments for T.
  /// \return On success, a pointer to the constructed object.
  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  /// \brief Destroy an object on this allocator.
  ///
  /// Calls the destructor of the object and passes the pointer for the object
  /// to deallocate().
  ///
  /// \param _data Opaque type to the object.
  template<typename T>
  void destroy(void* _data);

  /// \brief Allocate an array safely.
  ///
  /// This function is the same as allocate(_size * _count) except handles
  /// the case where such a multiplication would overflow by returning a null
  /// pointer, rather than an undersized allocation.
  ///
  /// \param _size The size of an element.
  /// \param _count The number of elements.
  Byte* allocate(Size _size, Size _count);

  ///
  /// \brief Reallocate an array safely.
  ///
  /// The same as reallocate() but performs _size * _count arithmetic checking
  /// for possible integer overflow.
  ///
  /// \param _data The data to reallocate.
  /// \param _size The size of each element.
  /// \param _count The number of elements.
  Byte* reallocate(void* _data, Size _size, Size _count);

  /// @{
  /// \brief Convienence functions to round a pointer or size to ALIGNMENT.
  static constexpr UintPtr round_to_alignment(UintPtr _ptr_or_size);
  template<typename T>
  static Byte* round_to_alignment(T* _ptr);
  /// @}
};

inline constexpr UintPtr Allocator::round_to_alignment(UintPtr _ptr_or_size) {
  return (_ptr_or_size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

template<typename T>
Byte* Allocator::round_to_alignment(T* _ptr) {
  return reinterpret_cast<Byte*>(round_to_alignment(reinterpret_cast<UintPtr>(_ptr)));
}

template<typename T, typename... Ts>
T* Allocator::create(Ts&&... _arguments) {
  if (Byte* data = allocate(sizeof(T))) {
    return Utility::construct<T>(data, Utility::forward<Ts>(_arguments)...);
  }
  return nullptr;
}

template<typename T>
void Allocator::destroy(void* _data) {
  if (_data) {
    Utility::destruct<T>(_data);
    deallocate(_data);
  }
}

struct View {
  Allocator* owner;
  Byte* data;
  Size size;
  Size capacity;
};

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_ALLOCATOR_H
