#ifndef RX_CORE_MEMORY_VMA_H
#define RX_CORE_MEMORY_VMA_H
#include "rx/core/types.h"
#include "rx/core/assert.h"

#include "rx/core/utility/exchange.h"

#include "rx/core/concepts/no_copy.h"

namespace rx::memory {

struct vma
  : concepts::no_copy
{
  constexpr vma();
  constexpr vma(rx_byte* _base, rx_size _page_size, rx_size _page_count);
  vma(vma&& vma_);

  ~vma();
  vma& operator=(vma&& vma_);

  struct range {
    rx_size offset;
    rx_size count;
  };

  [[nodiscard]] bool allocate(rx_size _page_size, rx_size _page_count);
  [[nodiscard]] bool commit(range _range, bool _read, bool _write);
  [[nodiscard]] bool uncommit(range _range);

  rx_byte* base() const;
  rx_byte* page(rx_size _index) const;

  rx_size page_count() const;
  rx_size page_size() const;

  bool is_valid() const;
  bool in_range(range _range) const;

  rx_byte* release();

private:
  void deallocate();

  rx_byte* m_base;

  rx_size m_page_size;
  rx_size m_page_count;
};

inline constexpr vma::vma()
  : vma{nullptr, 0, 0}
{
}

inline constexpr vma::vma(rx_byte* _base, rx_size _page_size, rx_size _page_count)
  : m_base{_base}
  , m_page_size{_page_size}
  , m_page_count{_page_count}
{
}

inline vma::vma(vma&& vma_)
  : m_base{utility::exchange(vma_.m_base, nullptr)}
  , m_page_size{utility::exchange(vma_.m_page_size, 0)}
  , m_page_count{utility::exchange(vma_.m_page_count, 0)}
{
}

inline vma::~vma() {
  deallocate();
}

inline vma& vma::operator=(vma&& vma_) {
  deallocate();

  m_base = utility::exchange(vma_.m_base, nullptr);
  m_page_size = utility::exchange(vma_.m_page_size, 0);
  m_page_count = utility::exchange(vma_.m_page_count, 0);

  return *this;
}

inline rx_byte* vma::base() const {
  RX_ASSERT(is_valid(), "unallocated");
  return m_base;
}

inline rx_byte* vma::page(rx_size _page) const {
  return base() + m_page_size * _page;
}

inline rx_size vma::page_count() const {
  return m_page_count;
}

inline rx_size vma::page_size() const {
  return m_page_size;
}

inline bool vma::is_valid() const {
  return m_base != nullptr;
}

inline bool vma::in_range(range _range) const {
  return _range.offset + _range.count <= m_page_count;
}

inline rx_byte* vma::release() {
  m_page_size = 0;
  m_page_count = 0;
  return utility::exchange(m_base, nullptr);
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_VMA_H
