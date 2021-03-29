#ifndef RX_CORE_BUFFERED_STREAM_H
#define RX_CORE_BUFFERED_STREAM_H
#include "rx/core/stream.h"
#include "rx/core/vector.h"

namespace Rx {

struct RX_API BufferedStream
  : Stream
{
  static constexpr const auto BUFFER_PAGE_SIZE = 4096_u16;
  static constexpr const auto BUFFER_PAGE_COUNT = 64_u8;

  constexpr BufferedStream(Memory::Allocator& _allocator);

  BufferedStream(BufferedStream&& buffered_stream_);
  ~BufferedStream();

  BufferedStream& operator=(BufferedStream&& buffered_stream_);

  static Optional<BufferedStream> create(Memory::Allocator& _allocator,
    Uint16 _page_size = BUFFER_PAGE_SIZE, Uint8 _page_count = BUFFER_PAGE_COUNT);

  // Resize the buffer.
  // Maximum of 256 pages in the page cache (Uint8 _page_count).
  // Maximum of 64 KiB in a page (Uint16 _page_size).
  // Maximum of 16 MiB of cached data (64 KiB * 256).
  //
  // Returns true on success, false otherwise. When this function fails, the
  // buffered stream retains the page size and count it had before this
  // function was called.
  [[nodiscard]] bool resize(Uint16 _page_size, Uint8 _page_count);

  // Attach stream to the buffer stream.
  [[nodiscard]] bool attach(Stream* _stream);

  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);
  virtual bool on_stat(Stat& stat_) const;
  virtual bool on_flush();

  // Cannot be called on an invalid stream.
  virtual const String& name() const & { return m_stream->name(); }

private:
  // sizeof(Page) = 8.
  struct Page {
    // The exact page number of this page. The byte offset of this page is
    // |page_no * m_page_size|.
    Uint32 page_no;

    // Actual number of bytes in this page. May not be the same as |m_page_size|
    // as it may be the last page or, |m_stream.write| that filled this page
    // did not read in a whole |m_page_bytes| due to a stream error.
    Uint16 size;

    // The page index inside |m_buffer| for the data representing this page.
    // Since it's the index, it should be multiplied by |m_page_size| for the
    // byte offset inside |m_buffer|.
    Uint8 buffer_index;

    // Called each time this page is hit. Saturates |hits| because it's only
    // 7-bits and overflow is not desired here. If we overflow then the LRU
    // would flush a possibly common page from cache to replace it since it
    // would have a lower hit rate due to wrapping around.
    Page* hit();

    struct {
      Uint8 hits : 7;  // Number of times this page was hit in cache.
      Uint8 dirty : 1; // Indicates if this page is dirty and needs to be flushed.
    };
  };

  // Retrieve pointer to the page data for the given page.
  RX_HINT_FORCE_INLINE Byte* page_data(const Page& _page) {
    return m_buffer.data() + _page.buffer_index * m_page_size;
  }

  // Retrieve stream-relative offset for a page.
  RX_HINT_FORCE_INLINE Uint64 page_offset(const Page& _page) {
    return _page.page_no * m_page_size;
  }

  bool flush_page(Page& page_);
  Page* find_page(Uint32 _page_no);
  Page* lookup_page(Uint32 _page_no, Uint16 _allocate = 0);
  Page* fill_page(Uint32 _page_no, Uint16 _allocate);
  Uint16 read_page(Uint32 _page_no, Byte* data_, Uint16 _offset, Uint16 _size);
  Uint16 write_page(Uint32 _page_no, const Byte* _data, Uint16 _offset, Uint16 _size);

  // Given a read or write, all that remains now is to break it up into page
  // sized operations. Here we need a page iterator of sorts that we can consume
  // for any operation. Since an operation may not consume a full page, the
  // boundaries need to report the right offsets and sizes within a page to
  // honor the request.
  struct Iterator {
    constexpr Iterator(const BufferedStream& _buffered_stream,
      Uint32 _last_page, Uint16 _last_size)
      : m_buffered_stream{_buffered_stream}
      , m_last_page{_last_page}
      , m_last_size{_last_size}
    {
    }

    Uint32 page;
    Uint16 offset;
    Uint16 size;

    void next() {
      page++;
      offset = 0;
      size = page == m_last_page
        ? m_last_size
        : m_buffered_stream.m_page_size;
    }

    operator bool() const {
      return page != m_last_page + 1;
    }

  private:
    const BufferedStream& m_buffered_stream;
    Uint32 m_last_page;
    Uint16 m_last_size;
  };

  Iterator page_iterate(Uint64 _size, Uint64 _offset) const;

  Stream* m_stream;
  LinearBuffer m_buffer;
  Vector<Page> m_pages;
  Uint16 m_page_size;
  Uint8 m_page_count;
};

inline constexpr BufferedStream::BufferedStream(Memory::Allocator& _allocator)
  : Stream{0}
  , m_stream{nullptr}
  , m_buffer{_allocator}
  , m_pages{_allocator}
  , m_page_size{0}
  , m_page_count{0}
{
}

inline BufferedStream::~BufferedStream() {
  on_flush();
}

} // namespace Rx

#endif // RX_CORE_BUFFERED_STREAM_H