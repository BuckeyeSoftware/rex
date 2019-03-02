#include <stdio.h> // FILE, f{open,close,read,write,seek}
#include <string.h> // strcmp

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/filesystem/file.h> // file

namespace rx::filesystem {

file::file(const char* file_name, const char* mode)
  : m_impl{static_cast<void*>(fopen(file_name, mode))}
  , m_file_name{file_name}
  , m_mode{mode}
{
  // {empty}
}

file::file(file&& other)
  : m_impl{other.m_impl}
{
  other.m_impl = nullptr;
}

file::~file() {
  if (m_impl) {
    fclose(static_cast<FILE*>(m_impl));
  }
}

rx_u64 file::read(rx_byte* data, rx_u64 size) {
  RX_ASSERT(strcmp(m_mode, "rb") == 0 || strcmp(m_mode, "r") == 0,
    "cannot read with mode '%s'", m_mode);
  return fread(data, 1, size, static_cast<FILE*>(m_impl));
}

rx_u64 file::write(const rx_byte* data, rx_u64 size) {
  RX_ASSERT(strcmp(m_mode, "wb")  == 0, "cannot write with mode '%s'", m_mode);
  return fwrite(data, 1, size, static_cast<FILE*>(m_impl));
}

bool file::seek(rx_u64 where) {
  RX_ASSERT(strcmp(m_mode, "rb") == 0, "cannot seek with mode '%s'", m_mode);
  return fseek(static_cast<FILE*>(m_impl), where, SEEK_SET) == 0;
}

bool file::print(string&& contents) {
  RX_ASSERT(strcmp(m_mode, "w") == 0, "cannot print with mode '%s'", m_mode);
  return fprintf(static_cast<FILE*>(m_impl), "%s", contents.data()) > 0;
}

bool file::flush() {
  RX_ASSERT(strcmp(m_mode, "w") == 0 || strcmp(m_mode, "wb") == 0,
    "cannot flush with mode '%s'", m_mode);
  return fflush(static_cast<FILE*>(m_impl)) == 0;
}

} // namespace rx::filesystem
