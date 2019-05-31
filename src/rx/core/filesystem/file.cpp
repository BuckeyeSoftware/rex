#include <stdio.h> // FILE, f{open,close,read,write,seek,flush}
#include <errno.h> // errno
#include <string.h> // strcmp, strerror

#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/log.h"
#include "rx/core/filesystem/file.h" // file

RX_LOG("filesystem/file", log_file);

namespace rx::filesystem {

file::file(void* _impl, const char* _file_name, const char* _mode)
  : m_impl{_impl}
  , m_file_name{_file_name}
  , m_mode{_mode}
{
}

file::file(const char* _file_name, const char* _mode)
  : m_impl{static_cast<void*>(fopen(_file_name, _mode))}
  , m_file_name{_file_name}
  , m_mode{_mode}
{
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
  close();
}

void file::close() {
  if (m_impl) {
    fclose(static_cast<FILE*>(m_impl));
    m_impl = nullptr;
  }
}

file& file::operator=(file&& _other) {
  close();
  m_impl = _other.m_impl;
  _other.m_impl = nullptr;
  return *this;
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
  return fseek(static_cast<FILE*>(m_impl), static_cast<long>(where), SEEK_SET) == 0;
}

optional<rx_u64> file::size() {
  RX_ASSERT(m_impl, "invalid");
  RX_ASSERT(strcmp(m_mode, "rb") == 0, "cannot get size with mode '%s'", m_mode);

  auto fp{static_cast<FILE*>(m_impl)};
  if (fseek(fp, 0, SEEK_END) != 0) {
    return nullopt;
  }

  auto result{ftell(fp)};
  if (result == -1L) {
    fseek(fp, 0, SEEK_SET);
    return nullopt;
  }

  fseek(fp, 0, SEEK_SET);
  return result;
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

optional<array<rx_byte>> read_binary_file(memory::allocator* _allocator, const string& _file_name) {
  file open_file{_file_name, "rb"};
  if (!open_file) {
    log_file(log::level::k_error, "failed to open file '%s' [%s]", _file_name,
      strerror(errno));
    return nullopt;
  }

  const auto size{open_file.size()};
  if (size) {
    array<rx_byte> data{_allocator, *size};
    if (!open_file.read(data.data(), data.size())) {
      log_file(log::level::k_error, "failed to read file '%s' [%s]", _file_name,
        strerror(errno));
      return nullopt;
    }
    return data;
  } else {
    // NOTE: taking advantage of stdio buffering here to make this reasonable
    array<rx_byte> data{_allocator, 1};
    for(rx_byte* cursor{data.data()}; open_file.read(cursor, 1); cursor++) {
      data.resize(data.size() + 1);
    }
    return data;
  }

  return nullopt;
}

} // namespace rx::filesystem
