#include <stdio.h> // FILE, f{open,close,read,write,seek,flush}
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
  , m_file_name{other.m_file_name}
  , m_mode{other.m_mode}
{
  other.m_impl = nullptr;
  other.m_file_name = nullptr;
  other.m_mode = nullptr;
}

file::~file() {
  if (m_impl) {
    fclose(static_cast<FILE*>(m_impl));
  }
}

rx_u64 file::read(rx_byte* data, rx_u64 size) {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "rb") == 0 || strcmp(m_mode, "r") == 0,
    "cannot read with mode '%s'", m_mode);
  return fread(data, 1, size, static_cast<FILE*>(m_impl));
}

rx_u64 file::write(const rx_byte* data, rx_u64 size) {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "wb")  == 0, "cannot write with mode '%s'", m_mode);
  return fwrite(data, 1, size, static_cast<FILE*>(m_impl));
}

bool file::seek(rx_u64 where) {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "rb") == 0, "cannot seek with mode '%s'", m_mode);
  return fseek(static_cast<FILE*>(m_impl), where, SEEK_SET) == 0;
}

bool file::print(string&& contents) {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "w") == 0, "cannot print with mode '%s'", m_mode);
  return fprintf(static_cast<FILE*>(m_impl), "%s", contents.data()) > 0;
}

bool file::flush() {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "w") == 0 || strcmp(m_mode, "wb") == 0,
    "cannot flush with mode '%s'", m_mode);
  return fflush(static_cast<FILE*>(m_impl)) == 0;
}

bool file::read_line(string& line) {
  auto* fp = static_cast<FILE*>(m_impl);

  line.clear();
  for (;;) {
    char buffer[4096];
    if (!fgets(buffer, sizeof buffer, fp)) {
      if (feof(fp)) {
        return !line.is_empty();
      }

      return false;
    }

    rx_size length{strlen(buffer)};

    if (length && buffer[length - 1] == '\n') {
      length--;
    }

    if (length && buffer[length - 1] == '\r') {
      length--;
    }

    line.append(buffer, length);

    if (length < sizeof buffer - 1) {
      return true;
    }
  }

  return false;
}

bool file::is_valid() const {
  return m_impl != nullptr;
}

} // namespace rx::filesystem
