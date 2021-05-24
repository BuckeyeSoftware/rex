#ifndef RX_CORE_BITSET_H
#define RX_CORE_BITSET_H
#include "rx/core/optional.h"

#include "rx/core/traits/is_same.h"
#include "rx/core/traits/invoke_result.h"

#include "rx/core/memory/system_allocator.h"

#include "rx/core/utility/exchange.h"

/// \file bitset.h

namespace Rx {

/// Represents a fixed-capacity sequence of bits.
struct RX_API Bitset {
  RX_MARK_NO_COPY(Bitset);

  using WordType = Uint64;

  static constexpr const WordType BIT_ONE = 1;
  static constexpr const Size WORD_BITS = 8 * sizeof(WordType);

  /// Default construct a bitset.
  constexpr Bitset();

  /// Move construct a bitset.
  Bitset(Bitset&& bitset_);

  /// Destroy a bitset.
  ~Bitset();

  Bitset& operator=(Bitset&& bitset_);

  /// \brief Create a bitset.
  ///
  /// \param _allocator The allocator to allocate bits.
  /// \param _size The number of bits to allocate.
  /// \return On success, the BitSet. Otherwise, \c nullopt.
  static Optional<Bitset> create(Memory::Allocator& _allocator, Size _size);

  /// Clear all bits.
  void clear();

  /// Set bit.
  void set(Size _bit);

  /// Clear bit.
  void clear(Size _bit);

  /// Test if bit is set.
  bool test(Size _bit) const;

  /// Get the number of bits.
  Size size() const;

  /// Count the number of set bits.
  Size count_set_bits() const;

  /// Count the number of unset bits.
  Size count_unset_bits() const;

  /// \brief Find the index of the first set bit.
  /// \returns The index of the first set bit or \p nullopt if none found.
  Optional<Size> find_first_set() const;

  /// \brief Find the index of the first unset bit.
  /// \returns The index of the first unset bit or \p nullopt if none found.
  Optional<Size> find_first_unset() const;

  /// \brief Enumerate bitset for each set bit.
  ///
  /// Enumerates the bitset and calls an invocable with the index of every set
  /// bit.
  ///
  /// \param _function The invocable with the signature:
  /// \code{.cpp}
  ///   void function(Size _index);
  /// \endcode
  template<typename F>
  void each_set(F&& _function) const;

  /// \brief Enumerate bitset for each unset bit.
  ///
  /// Enumerates the bitset and calls an invocable with the index of every set
  /// unbit.
  ///
  /// \param _function The invocable with the signature:
  /// \code{.cpp}
  ///   void function(Size _index);
  /// \endcode
  template<typename F>
  void each_unset(F&& _function) const;

  /// The allocator used to allocate this bitset.
  constexpr Memory::Allocator& allocator() const;

private:
  static void move(Bitset* dst_, Bitset* src_);

  static Optional<Bitset> create_uninitialized(Memory::Allocator& _allocator, Size _size);

  static Size words_for_size(Size _size);

  static Size index(Size bit);
  static Size offset(Size bit);

  void release();

  Memory::Allocator* m_allocator;
  Size m_size;
  WordType* m_data;
};

inline void Bitset::move(Bitset* dst_, Bitset* src_) {
  dst_->m_allocator = src_->m_allocator;
  dst_->m_size = Utility::exchange(src_->m_size, 0);
  dst_->m_data = Utility::exchange(src_->m_data, nullptr);
}

inline constexpr Bitset::Bitset()
  : m_allocator{nullptr}
  , m_size{0}
  , m_data{0}
{
}

inline Bitset::Bitset(Bitset&& bitset_) {
  move(this, &bitset_);
}

inline Bitset& Bitset::operator=(Bitset&& bitset_) {
  if (this != &bitset_) {
    release();
    move(this, &bitset_);
  }
  return *this;
}

inline Bitset::~Bitset() {
  release();
}

inline void Bitset::set(Size _bit) {
  RX_ASSERT(_bit < m_size, "out of bounds");
  m_data[index(_bit)] |= BIT_ONE << offset(_bit);
}

inline void Bitset::clear(Size _bit) {
  RX_ASSERT(_bit < m_size, "out of bounds");
  m_data[index(_bit)] &= ~(BIT_ONE << offset(_bit));
}

inline bool Bitset::test(Size _bit) const {
  RX_ASSERT(_bit < m_size, "out of bounds");
  return !!(m_data[index(_bit)] & (BIT_ONE << offset(_bit)));
}

inline Size Bitset::size() const {
  return m_size;
}

inline Size Bitset::words_for_size(Size _size) {
  return _size / WORD_BITS + 1;
}

inline Size Bitset::index(Size _bit) {
  return _bit / WORD_BITS;
}

inline Size Bitset::offset(Size _bit) {
  return _bit % WORD_BITS;
}

template<typename F>
void Bitset::each_set(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, Size>;
  for (Size i = 0; i < m_size; i++) {
    if (test(i)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

template<typename F>
void Bitset::each_unset(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, Size>;
  for (Size i{0}; i < m_size; i++) {
    if (!test(i)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Bitset::allocator() const {
  return *m_allocator;
}

} // namespace Rx

#endif // RX_CORE_BITSET_H
