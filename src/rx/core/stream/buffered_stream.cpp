#include <string.h>
#include "rx/core/stream/buffered_stream.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Stream {

// [BufferedStream]
BufferedStream::BufferedStream(BufferedStream&& buffered_stream_)
  : Context{Utility::move(buffered_stream_)}
  , m_context{Utility::exchange(buffered_stream_.m_context, nullptr)}
  , m_buffer{Utility::move(buffered_stream_.m_buffer)}
  , m_pages{Utility::move(buffered_stream_.m_pages)}
  , m_page_size{Utility::exchange(buffered_stream_.m_page_size, 0)}
  , m_page_count{Utility::exchange(buffered_stream_.m_page_count, 0)}
{
}

BufferedStream& BufferedStream::operator=(BufferedStream&& buffered_stream_) {
  if (this != &buffered_stream_) {
    on_flush();
    Context::operator=(Utility::move(buffered_stream_));
    m_context = Utility::exchange(buffered_stream_.m_context, nullptr);
    m_buffer = Utility::move(buffered_stream_.m_buffer);
    m_pages = Utility::move(buffered_stream_.m_pages);
    m_page_size = Utility::exchange(buffered_stream_.m_page_size, 0);
    m_page_count = Utility::exchange(buffered_stream_.m_page_count, 0);
  }
  return *this;
}

Optional<BufferedStream> BufferedStream::create(Memory::Allocator& _allocator,
  Uint16 _page_size, Uint8 _page_count)
{
  BufferedStream result{_allocator};
  if (result.resize(_page_size, _page_count)) {
    return result;
  }
  return nullopt;
}

// Flushes the page cache entry out to the underlying stream.
bool BufferedStream::flush_page(Page& page_) {
  // Only flush when the page is dirty.
  if (page_.dirty) {
    page_.dirty = 0;
    page_.hits = 0;
    const auto bytes = m_context->on_write(page_data(page_), page_.size, page_offset(page_));
    return bytes == page_.size;
  }
  return true;
}

// Finds a page in the page cache that matches the given page number,
// returning a pointer to that page, or nullptr if not found.
BufferedStream::Page* BufferedStream::find_page(Uint32 _page_no) {
  const auto index = m_pages.find_if([&](const Page& _page) {
    return _page.page_no == _page_no;
  });
  return index ? &m_pages[*index] : nullptr;
}

// Performs a page lookup on |_page_no|. If the page is in cache, it returns
// the cached page. If it's not in cache, it fills the page in cache and
// returns it. This function returns nullptr on failure.
//
// The |_allocate| parameter is the number of bytes to allocate in the stream
// for such a page. When this is zero, the data is expected to be read from
// the stream. When this is not zero, the |_allocate| bytes are allocated
// in the page cache and zeroed out, marking the page dirty immediately.
BufferedStream::Page* BufferedStream::lookup_page(Uint32 _page_no, Uint16 _allocate) {
  if (auto page = find_page(_page_no)) {
    // Since it was found in cache, hit the page.
    page->hit();

    // Possibly expand the page.
    if (_allocate > page->size) {
      // Zero out the page cache area representing the resize.
      memset(page_data(*page) + page->size, 0, _allocate - page->size);
      page->size = _allocate;
    }

    return page;
  }
  return fill_page(_page_no, _allocate);
}

// Reads in page |_page_no| and stores it in page cache by replacing the least
// recently used page in the cache. This returns a pointer to the Page itself.
BufferedStream::Page* BufferedStream::fill_page(Uint32 _page_no, Uint16 _allocate) {
  // There's three cases to consider when filling a page.
  // * The cache is empty.
  // * The cache is not full.
  // * The cache is full.
  //
  // All of these cases are handled here.
  Uint8 best_index = 0;
  Uint8 page_count = Uint8(m_pages.size());
  if (RX_HINT_UNLIKELY(page_count < m_page_count)) {
    best_index = page_count;
    // Cannot fail as |m_pages| is reserved for |m_page_count|.
    (void)m_pages.emplace_back(0_u32, 0_u16, page_count, 0_u8, 0_u8);
  } else {
    // Search for page with fewest cache hits.
    Uint8 best_hits = 127;
    for (Uint8 i = 0; i < page_count; i++) {
      const auto& page = m_pages[i];
      if (page.hits < best_hits) {
        best_hits = page.hits;
        best_index = i;
      }
    }
  }

  Page& page = m_pages[best_index];

  // Flush the contents of this page out to replace it with a new page.
  if (!flush_page(page)) {
    return nullptr;
  }

  // Now the entry refers to this page.
  page.page_no = _page_no;

  if (_allocate) {
    // Zero the page data as this is freshly allocated storage in the cache.
    memset(page_data(page), 0, _allocate);
    page.size = _allocate;
    page.dirty = 1;
  } else {
    // Fill in the page cache now with new page data.
    //
    // It's not always possible to read in a whole page when at the end of the
    // stream. Here we need to store the actual amount of bytes this page
    // truelly represents, so that future flushing of the contents does not
    // over-flush the page and inflate the true size.
    page.size = m_context->on_read(page_data(page), m_page_size, page_offset(page));
  }

  return &page;
}

// Reads |_size| bytes from offset |_offset| in page |_page_no| into |data_|.
// Returns the number of bytes actually read into the page and zero on error.
Uint16 BufferedStream::read_page(Uint32 _page_no, Byte* data_, Uint16 _offset, Uint16 _size) {
  if (Page* page = lookup_page(_page_no)) {
    const auto size = Algorithm::min(_size, page->size);
    memcpy(data_, page_data(*page) + _offset, size);
    return size;
  }
  return 0;
}

// Writes |_size| bytes from |_data| into the page |_page_no| at offset |_offset|.
// Returns the number of bytes actually written into the page and zero on error.
Uint16 BufferedStream::write_page(Uint32 _page_no, const Byte* _data, Uint16 _offset, Uint16 _size) {
  if (Page *page = lookup_page(_page_no, _offset + _size)) {
    // The page is going to be made dirty by this write.
    page->dirty = 1;
    const auto size = Algorithm::min(_size, page->size);
    memcpy(page_data(*page) + _offset, _data, size);
    return size;
  }
  return 0;
}

// Flushes all pages in the cache that are dirty.
bool BufferedStream::on_flush() {
  // Nothing to flush if no context.
  if (!m_context) {
    return true;
  }

  if (m_pages.each_fwd([this](Page& page_) { return flush_page(page_); })) {
    m_pages.clear();
    return true;
  }

  return false;
}

const String& BufferedStream::name() const & {
  return m_context->name();
}

BufferedStream::Iterator BufferedStream::page_iterate(Uint64 _size, Uint64 _offset) const {
  const Uint32 beg_page = _offset / m_page_size;
  const Uint32 end_page = (_offset + _size - 1) / m_page_size;
  const Uint16 beg_offset = _offset % m_page_size;
  const Uint16 end_offset = 0;
  const Uint16 beg_size = Algorithm::min(m_page_size - beg_offset, Sint32(_size));
  const Uint16 end_size = (_offset + _size) - end_page * m_page_size;
  return Iterator{{beg_page, beg_size, beg_offset}, {end_page, end_size, end_offset}, m_page_size};
}

bool BufferedStream::resize(Uint16 _page_size, Uint8 _page_count) {
  // Flush everything in cache.
  if (!on_flush()) {
    return false;
  }

  if (!m_pages.reserve(_page_count)) {
    return false;
  }

  if (m_buffer.resize(_page_size * _page_count)) {
    m_page_size = _page_size;
    m_page_count = _page_count;
    return true;
  }

  return false;
}

bool BufferedStream::attach(Context* _context) {
  // Nothing to do when already attached.
  if (m_context == _context) {
    return true;
  }

  // Flush everything in cache.
  if (!on_flush()) {
    return false;
  }

  // Change the context.
  m_context = _context;

  // Adopt the flags of the context.
  m_flags = _context ? (_context->flags() | FLUSH) : 0;

  return true;
}

Uint64 BufferedStream::on_read(Byte* data_, Uint64 _size, Uint64 _offset) {
  // Enumerate in pages.
  Uint64 bytes = 0;
  auto i = page_iterate(_size, _offset);
  do {
    const auto& info = i.info();
    if (const auto result = read_page(info.page, data_ + bytes, info.offset, info.size)) {
      bytes += result;
    } else {
      break;
    }
  } while (i.next());
  return bytes;
}

Uint64 BufferedStream::on_write(const Byte* _data, Uint64 _size, Uint64 _offset) {
  // Enumerate in pages.
  Uint64 bytes = 0;
  auto i = page_iterate(_size, _offset);
  do {
    const auto& info = i.info();
    if (const auto result = write_page(info.page, _data + bytes, info.offset, info.size)) {
      bytes += result;
    } else {
      break;
    }
  } while (i.next());
  return bytes;
}

bool BufferedStream::on_stat(Stat& stat_) const {
  auto stat = m_context->stat();
  if (!stat) {
    return false;
  }

  stat_.size = stat->size;

  // There may be a higher watermark size in the page cache.
  m_pages.each_fwd([&](const Page& _page) {
    const auto size = Uint64(_page.page_no * m_page_size + _page.size);
    stat_.size = Algorithm::max(stat_.size, size);
  });

  return true;
}

// [BufferedStream::Page]
BufferedStream::Page* BufferedStream::Page::hit() {
  hits = (hits + 1) & 127;
  return this;
}

} // namespace Rx