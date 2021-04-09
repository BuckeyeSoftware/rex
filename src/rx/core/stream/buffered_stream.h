#ifndef RX_CORE_STREAM_BUFFERED_STREAM_H
#define RX_CORE_STREAM_BUFFERED_STREAM_H
#include "rx/core/stream/context.h"
#include "rx/core/vector.h"

/// \file buffered_stream.h

namespace Rx::Stream {

/// \brief Buffered stream.
///
/// A BufferedStream has the same interface as a Context except stream
/// operations are buffered with a page cache.
///
/// All stream operations have their offsets rounded to a page size granularity
/// and count of pages. Such operations then go directly to the page cache.
/// The page cache is not contiguous. Each time an operation hits the page cache
/// directly, the page's hit count is incremented. The page with the fewest hits
/// is reused when none of the pages in cache can service an operation. With
/// exception to the first and last page of a stream, all underlying stream
/// reads and writes will be of the size of a page.
///
/// A BufferedStream models the sort of page caching that an operating system
/// implements for files, except implemented in user-space. Rex's
/// Filesystem::UnbufferedFile attempts to disable page caching otherwise two
/// caches would exist doubling the memory usage of a Filesystem::BufferedFile.
///
/// \note The maximum amount of memory a BufferedStream can buffer is given by
/// the limits of 64 KiB for a page and 256 pages (16 MiB).
struct RX_API BufferedStream
  : Context
{
  /// Default page size (in bytes) for the page cache.
  static constexpr const auto BUFFER_PAGE_SIZE = 4096_u16;
  /// Default page count for the page cache.
  static constexpr const auto BUFFER_PAGE_COUNT = 64_u8;

  /// \brief Construct a BufferedStream
  /// \param _allocator The allocator to use for underlying stream operations.
  constexpr BufferedStream(Memory::Allocator& _allocator);

  /// \brief Move construct a buffered stream.
  /// \param buffered_stream_ The buffered stream to move from.
  ///
  /// Assigns the state of \p buffered_stream_ to \c *this and sets
  /// \p buffered_stream_ to a default constructed state.
  BufferedStream(BufferedStream&& buffered_stream_);

  /// \brief Destroy a buffered stream.
  /// \note Will call on_flush(), asserting if the flush fails.
  ~BufferedStream();

  /// \brief Moves the buffered stream.
  ///
  /// If \c *this still has buffered contents, calls on_flush(). Otherwise,
  /// assigns the state of \p buffered_stream_ to \c *this and sets
  /// \p buffered_stream_ to a default constructed state.
  ///
  /// \param buffered_stream_ Another buffered stream to assign this buffered
  /// stream.
  /// \returns \c *this.
  BufferedStream& operator=(BufferedStream&& buffered_stream_);

  /// \brief Created a BufferedStream.
  ///
  /// \param _allocator The allocator to use for page cache.
  /// \param _page_size The size of a page in bytes.
  /// \param _page_count The number of pages for the cache.
  /// \returns The BufferedStream on success, nullopt otherwise.
  ///
  /// \note The creation can fail if there's not enough memory in \p _allocator
  /// to allocate the initial page cache.
  ///
  /// \note The cache can be resized later by a call to resize().
  static Optional<BufferedStream> create(Memory::Allocator& _allocator,
    Uint16 _page_size = BUFFER_PAGE_SIZE, Uint8 _page_count = BUFFER_PAGE_COUNT);

  /// \brief Resize the buffer.
  ///
  /// The maximum size a buffer can be resized is given by:
  ///  * 256 pages in the page cache \p _page_size.
  ///  * 64 KiB in a page \p _page_count.
  ///  * 16 MiB of cached data (64 KiB * 256).
  ///
  /// \param _page_size The size of a page in bytes.
  /// \param _page_count The number of pages for the cache.
  /// \returns On successful resize, \c true. Otherwise, \c false. When this
  /// function fails, the buffered stream retains the page size and count it
  /// had before this function was called.
  ///
  /// \note This function can fail if flushing of existing cached pages failed
  /// to write completely to the underlying Context or the allocator passed
  /// during construction ran out of memory to resize the internal page cache.
  [[nodiscard]] bool resize(Uint16 _page_size, Uint8 _page_count);

  /// \brief Attach context to this stream.
  ///
  /// Associates an underlying Context with the stream to provide buffering
  /// ontop of.
  ///
  /// \param _stream The stream to attach or nullptr to detach a stream.
  [[nodiscard]] bool attach(Context* _stream);

  /// \brief The name of the stream.
  /// \warning Cannot be called if no stream is attached.
  virtual const String& name() const &;

  /// \brief Read data.
  /// Reads \p _size bytes from the stream at \p _offset into \p data_.
  /// \returns The number of bytes actually read from \p _offset into \p data_.
  virtual Uint64 on_read(Byte* data_, Uint64 _size, Uint64 _offset);

  /// \brief Write data.
  /// Writes \p _size bytes from \p _data into the steam at \p _offset.
  /// \returns The number of bytes actually written at \p _offset from \p _data.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size, Uint64 _offset);

  /// \brief Stat the stream for information.
  /// Updates the stat object referenced by \p stat_.
  /// \returns On a successful stat, \c true. Otherwise, \c false.
  virtual bool on_stat(Stat& stat_) const;

  /// \brief Flush all dirty pages out to the underlying stream.
  /// \returns On a successful flush, \c true. Otherwise, \c false.
  /// \note A flush can fail if not all pages are written out successfully.
  virtual bool on_flush();

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

  struct PageInfo {
    Uint32 page;   ///< The page number.
    Uint16 size;   ///< The size of the page operation.
    Uint16 offset; ///< The offset inside the page to begin from.
  };

  struct Iterator {
    constexpr Iterator(const BufferedStream& _buffered_stream,
      PageInfo _beg_page, PageInfo _end_page, Size _page_size)
      : m_buffered_stream{_buffered_stream}
      , m_this_page{_beg_page}
      , m_last_page{_end_page}
      , m_page_size{_page_size}
    {
    }

    PageInfo info() const { return m_this_page; }

    bool next() {
      if (m_this_page.page == m_last_page.page) {
        return false;
      }

      m_this_page.page++;
      m_this_page.offset = 0;
      m_this_page.size = m_this_page.page != m_last_page.page ? m_page_size : m_last_page.size;
      return true;
    }

  private:
    const BufferedStream& m_buffered_stream;
    PageInfo m_this_page;
    PageInfo m_last_page;
    Size m_page_size;
  };

  Iterator page_iterate(Uint64 _size, Uint64 _offset) const;

  Context* m_context;
  LinearBuffer m_buffer;
  Vector<Page> m_pages;
  Uint16 m_page_size;
  Uint8 m_page_count;
};

inline constexpr BufferedStream::BufferedStream(Memory::Allocator& _allocator)
  : Context{0}
  , m_context{nullptr}
  , m_buffer{_allocator}
  , m_pages{_allocator}
  , m_page_size{0}
  , m_page_count{0}
{
}

inline BufferedStream::~BufferedStream() {
  RX_ASSERT(on_flush(), "failed to flush");
}

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_BUFFERED_STREAM_H