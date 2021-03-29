#include "rx/core/filesystem/buffered_file.h"

namespace Rx::Filesystem {

// [BufferedFile]
BufferedFile::BufferedFile(BufferedFile&& other_)
  : Stream{Utility::move(other_)}
  , m_unbuffered_file{Utility::move(other_.m_unbuffered_file)}
  , m_buffered_stream{Utility::move(other_.m_buffered_stream)}
{
  (void)m_buffered_stream.attach(&m_unbuffered_file);
}

BufferedFile& BufferedFile::operator=(BufferedFile&& file_) {
  if (this != &file_) {
    (void)close();
    Stream::operator=(Utility::move(file_));
    m_unbuffered_file = Utility::move(file_.m_unbuffered_file);
    m_buffered_stream = Utility::move(file_.m_buffered_stream);
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
  if (!m_buffered_stream.flush()) {
    return false;
  }

  if (!m_unbuffered_file.close()) {
    return false;
  }

  (void)m_buffered_stream.attach(nullptr);

  return true;
}

bool BufferedFile::print(String&& contents_) {
  const auto data = reinterpret_cast<const Byte*>(contents_.data());
  const auto size = contents_.size();
  return m_buffered_stream.write(data, size) == size;
}

const String& BufferedFile::name() const & {
  return m_unbuffered_file.name();
}

Uint64 BufferedFile::on_read(Byte* data_, Uint64 _size, Uint64 _offset) {
  return m_buffered_stream.on_read(data_, _size, _offset);
}

Uint64 BufferedFile::on_write(const Byte* _data, Uint64 _size, Uint64 _offset) {
  return m_buffered_stream.on_write(_data, _size, _offset);
}

bool BufferedFile::on_stat(Stat& stat_) const {
  return m_buffered_stream.on_stat(stat_);
}

bool BufferedFile::on_flush() {
  return m_buffered_stream.on_flush();
}

BufferedFile::BufferedFile(BufferedStream&& buffered_stream_, UnbufferedFile&& unbuffered_file_)
  : Stream{unbuffered_file_.flags() | FLUSH}
  , m_unbuffered_file{Utility::move(unbuffered_file_)}
  , m_buffered_stream{Utility::move(buffered_stream_)}
{
  (void)m_buffered_stream.attach(&m_unbuffered_file);
}

} // namespace Rx::Filesystem
