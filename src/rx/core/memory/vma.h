#ifndef RX_CORE_MEMORY_VMA_H
#define RX_CORE_MEMORY_VMA_H
#include "rx/core/optional.h"

#include "rx/core/utility/exchange.h"

/// \file vma.h
namespace Rx::Memory {

/// \brief Virtual memory allocation.
struct RX_API VMA {
  RX_MARK_NO_COPY(VMA);

  VMA(VMA&& vma_);

  ~VMA();
  VMA& operator=(VMA&& vma_);

  /// \brief Page range in page units.
  struct Range {
    Size offset; ///< Page offset.
    Size count;  ///< Page count.
  };

  /// \brief Allocate virtual memory.
  /// \param _page_size The size of a page.
  /// \param _page_count The number of pages to reserve.
  /// \param _remappable If this allocation will be remapped.
  static Optional<VMA> allocate(Size _page_size, Size _page_count, bool _remappable = false);

  /// Remap an existing allocation at an offset.
  /// \param _range The range to remap.
  /// \param _read Allow reads on this region.
  /// \param _write Allow writes on this region.
  /// \warning This is only valid for VMAs initially allocated remappable.
  Optional<VMA> remap(Range _range, bool _read, bool _write);

  /// \param Commit a range of memory
  /// \param _range The range to commit.
  /// \param _read Allow reads on the comitted region.
  /// \param _write Allow writes on the comitted region.
  [[nodiscard]] bool commit(Range _range, bool _read, bool _write);

  /// \brief Uncommit a range of memory.
  /// \param _range The range to uncommit.
  [[nodiscard]] bool uncommit(Range _range);

  Byte* base() const;
  Byte* page(Size _index) const;

  Size page_count() const;
  Size page_size() const;

  bool is_valid() const;
  bool in_range(Range _range) const;

  Byte* release();

private:
  constexpr VMA();
  constexpr VMA(Byte* _base, int _shared, Size _page_size, Size _page_count);

  void deallocate();

  Byte* m_base;
  int m_shared;

  Size m_page_size;
  Size m_page_count;
};

inline constexpr VMA::VMA()
  : VMA{nullptr, -1, 0, 0}
{
}

inline constexpr VMA::VMA(Byte* _base, int _shared, Size _page_size, Size _page_count)
  : m_base{_base}
  , m_shared{_shared}
  , m_page_size{_page_size}
  , m_page_count{_page_count}
{
}

inline VMA::VMA(VMA&& vma_)
  : m_base{Utility::exchange(vma_.m_base, nullptr)}
  , m_shared{Utility::exchange(vma_.m_shared, -1)}
  , m_page_size{Utility::exchange(vma_.m_page_size, 0)}
  , m_page_count{Utility::exchange(vma_.m_page_count, 0)}
{
}

inline VMA::~VMA() {
  deallocate();
}

inline VMA& VMA::operator=(VMA&& vma_) {
  if (&vma_ != this) {
    deallocate();
    m_base = Utility::exchange(vma_.m_base, nullptr);
    m_shared = Utility::exchange(vma_.m_shared, -1);
    m_page_size = Utility::exchange(vma_.m_page_size, 0);
    m_page_count = Utility::exchange(vma_.m_page_count, 0);
  }
  return *this;
}

inline Byte* VMA::base() const {
  return m_base;
}

inline Byte* VMA::page(Size _page) const {
  return base() + m_page_size * _page;
}

inline Size VMA::page_count() const {
  return m_page_count;
}

inline Size VMA::page_size() const {
  return m_page_size;
}

inline bool VMA::is_valid() const {
  return m_base != nullptr;
}

inline bool VMA::in_range(Range _range) const {
  return _range.offset + _range.count <= m_page_count;
}

inline Byte* VMA::release() {
  m_page_size = 0;
  m_page_count = 0;
  return Utility::exchange(m_base, nullptr);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_VMA_H
