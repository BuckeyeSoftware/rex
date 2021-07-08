#include "rx/core/stream/buffered_stream.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

#include "rx/core/memory/zero.h"
#include "rx/core/memory/copy.h"

namespace Rx::Stream {

// [BufferedStream]
BufferedStream::BufferedStream(BufferedStream&& buffered_stream_)
  : Context{Utility::move(buffered_stream_)}
  , m_stream{Utility::exchange(buffered_stream_.m_stream, nullptr)}
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
    m_stream = Utility::exchange(buffered_stream_.m_stream, nullptr);
    m_buffer = Utility::move(buffered_stream_.m_buffer);
    m_pages = Utility::move(buffered_stream_.m_pages);
    m_page_size = Utility::exchange(buffered_stream_.m_page_size, 0);
    m_page_count = Utility::exchange(buffered_stream_.m_page_count, 0);
  }
  return *this;
}

BufferedStream::~BufferedStream() {
  RX_ASSERT(on_flush(), "failed to flush");
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
    const auto bytes =
      m_stream->on_write(page_data(page_), page_.size, page_offset(page_));
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
      Memory::zero(page_data(*page) + page->size, _allocate - page->size);
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

  // Fill in the page cache now with new page data.
  const auto read = (m_stream->flags() & READ)
    ? m_stream->on_read(page_data(page), m_page_size, page_offset(page))
    : 0;

  if (read < _allocate) {
    // Zero the area to allocate and expand allocation.
    Memory::zero(page_data(page) + read, _allocate - read);
    page.size = _allocate;
    page.dirty = 1;
  } else {
    // Store the actual amount of bytes this page truely represents so that
    // future flushing of the contents do not over-flush the page.
    page.size = read;
  }

  return &page;
}

// Reads |_size| bytes from offset |_offset| in page |_page_no| into |data_|.
// Returns the number of bytes actually read into the page and zero on error.
Uint16 BufferedStream::read_page(Uint32 _page_no, Byte* data_, Uint16 _offset, Uint16 _size) {
  if (Page* page = lookup_page(_page_no)) {
    const auto size = Algorithm::min(_size, Uint16(page->size - _offset));
    Memory::copy(data_, page_data(*page) + _offset, size);
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
    const auto size = Algorithm::min(_size, Uint16(page->size - _offset));
    Memory::copy(page_data(*page) + _offset, _data, size);
    return size;
  }
  return 0;
}

const String& BufferedStream::name() const & {
  return m_stream->name();
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
    // Zero the contents of the buffer.
    Memory::zero(m_buffer.data(), m_buffer.size());

    m_page_size = _page_size;
    m_page_count = _page_count;
    return true;
  }

  return false;
}

bool BufferedStream::attach(Context& _stream) {
  // Nothing to do when already attached.
  if (m_stream == &_stream) {
    return true;
  }

  // Flush everything in cache.
  if (!on_flush()) {
    return false;
  }

  // Change the stream.
  m_stream = &_stream;

  // Adopt the flags of the stream.
  m_flags = m_stream->flags() | FLUSH;

  return true;
}

[[nodiscard]] bool BufferedStream::detach() {
  // Flush everything in cache.
  if (!on_flush()) {
    return false;
  }

  m_stream = nullptr;

  return true;
}

Uint64 BufferedStream::on_read(Byte* data_, Uint64 _size, Uint64 _offset) {
  // Reads larger than a page size, skip the buffering.
  if (_size > m_page_size && on_flush()) {
    return m_stream->on_read(data_, _size, _offset);
  }

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
  // Writes larger than a page size, skip the buffering.
  if (_size > m_page_size && on_flush()) {
    return m_stream->on_write(_data, _size, _offset);
  }

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

Optional<Stat> BufferedStream::on_stat() const {
  auto stat = m_stream->on_stat();
  if (!stat) {
    return nullopt;
  }

  Stat result;
  result.size = stat->size;

  // There may be a higher watermark size in the page cache.
  m_pages.each_fwd([&](const Page& _page) {
    const auto page_end = Uint64(_page.page_no * m_page_size + _page.size);
    result.size = Algorithm::max(result.size, page_end);
  });

  return result;
}

// Flushes all pages in the cache that are dirty.
bool BufferedStream::on_flush() {
  // Nothing to flush if no stream.
  if (!m_stream) {
    return true;
  }

  // Nothing to flush if no pages.
  if (m_pages.is_empty()) {
    return true;
  }

  if (m_pages.each_fwd([this](Page& page_) { return flush_page(page_); })) {
    m_pages.clear();
    // Remember to flush the underlying stream if supported too.
    return (m_stream->flags() & FLUSH) ? m_stream->on_flush() : true;
  }

  return false;
}

bool BufferedStream::on_truncate(Uint64 _size) {
  // Just flush all pages and truncate the underlying stream.
  return on_flush() && m_stream->on_truncate(_size);
}

Uint64 BufferedStream::on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size) {
  // Just flush all pages and do the copy on the underlying stream.
  return on_flush() ? m_stream->on_copy(_dst_offset, _src_offset, _size) : 0;
}

// [BufferedStream::Page]
BufferedStream::Page* BufferedStream::Page::hit() {
  hits = (hits + 1) & 127;
  return this;
}

} // namespace Rx