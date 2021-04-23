#include "rx/core/filesystem/buffered_file.h"

namespace Rx::Filesystem {

BufferedFile::~BufferedFile() {
  // Flush and detach |m_unbufferd_file| from the BufferedStream so that it's
  // destructor does not attempt to flush its page cache on an empty stream.
  RX_ASSERT(detach(), "flush failed");
}

BufferedFile::BufferedFile(BufferedFile&& other_)
  : BufferedStream{Utility::move(other_)}
  , m_unbuffered_file{Utility::move(other_.m_unbuffered_file)}
{
  // NOTE(dweiler): Cannot fail as there is nothing to flush on construction.
  (void)attach(m_unbuffered_file);
}

BufferedFile& BufferedFile::operator=(BufferedFile&& file_) {
  if (this != &file_) {
    RX_ASSERT(detach(), "flush failed");

    BufferedStream::operator=(Utility::move(file_));
    m_unbuffered_file = Utility::move(file_.m_unbuffered_file);

    // NOTE(dweiler): Cannot fail as there is nothing to flush on construction.
    (void)attach(m_unbuffered_file);
  }
  return *this;
}

Optional<BufferedFile> BufferedFile::open(Memory::Allocator& _allocator,
  const char* _file_name, const char* _mode, Uint16 _page_size, Uint8 _page_count)
{
  if (auto buffer = BufferedStream::create(_allocator, _page_size, _page_count)) {
    if (auto file = UnbufferedFile::open(_allocator, _file_name, _mode)) {
      return BufferedFile{Utility::move(*buffer), Utility::move(*file)};
    }
  }
  return nullopt;
}

BufferedFile::BufferedFile(BufferedStream&& buffered_stream_, UnbufferedFile&& unbuffered_file_)
  : BufferedStream{Utility::move(buffered_stream_)}
  , m_unbuffered_file{Utility::move(unbuffered_file_)}
{
  // NOTE(dweiler): Cannot fail as there is nothing to flush on construction.
  (void)attach(m_unbuffered_file);
}

} // namespace Rx::Filesystem