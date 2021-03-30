#include "rx/core/filesystem/buffered_file.h"

namespace Rx::Filesystem {

// [BufferedFile]
BufferedFile::BufferedFile(BufferedFile&& other_)
  : BufferedStream{Utility::move(other_)}
  , m_unbuffered_file{Utility::move(other_.m_unbuffered_file)}
{
  (void)attach(&m_unbuffered_file);
}

BufferedFile& BufferedFile::operator=(BufferedFile&& file_) {
  if (this != &file_) {
    (void)close();
    BufferedStream::operator=(Utility::move(file_));
    m_unbuffered_file = Utility::move(file_.m_unbuffered_file);
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

bool BufferedFile::close() {
  if (!flush()) {
    return false;
  }

  if (!m_unbuffered_file.close()) {
    return false;
  }

  (void)attach(nullptr);

  return true;
}

bool BufferedFile::print(String&& contents_) {
  const auto data = reinterpret_cast<const Byte*>(contents_.data());
  const auto size = contents_.size();
  return write(data, size) == size;
}

const String& BufferedFile::name() const & {
  return m_unbuffered_file.name();
}

BufferedFile::BufferedFile(BufferedStream&& buffered_stream_, UnbufferedFile&& unbuffered_file_)
  : BufferedStream{Utility::move(buffered_stream_)}
  , m_unbuffered_file{Utility::move(unbuffered_file_)}
{
  (void)attach(&m_unbuffered_file);
}

} // namespace Rx::Filesystem
