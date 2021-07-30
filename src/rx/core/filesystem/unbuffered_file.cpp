#include <string.h>

#include "rx/core/algorithm/min.h"
#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/log.h"

#if defined(RX_PLATFORM_POSIX)
#include <sys/stat.h> // fstat, struct stat
#include <unistd.h> // open, close, pread, pwrite
#include <fcntl.h>
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace Rx::Filesystem {

RX_LOG("filesystem/file", logger);

static inline Uint32 flags_from_mode(const StringView& _mode) {
  Uint32 flags = 0;

  flags |= Stream::STAT;

  for (Size n = _mode.size(), i = 0; i < n; i++) {
    switch (_mode[i]) {
    case 'r':
      flags |= Stream::READ;
      break;
    case 'w':
      flags |= Stream::WRITE;
      break;
    case '+':
      flags |= Stream::READ;
      flags |= Stream::WRITE;
      break;
    }
  }

  return flags;
}

#if defined(RX_PLATFORM_POSIX)
union ImplCast { void* impl; int fd; };
#define impl(x) (ImplCast{reinterpret_cast<void*>(x)}.fd)
#elif defined(RX_PLATFORM_WINDOWS)
#define impl(x) (reinterpret_cast<HANDLE>(x))
#endif

#if defined(RX_PLATFORM_POSIX)
static void* open_file([[maybe_unused]] Memory::Allocator& _allocator,
  const StringView& _file_name, const StringView& _mode)
{
  // Determine flags to open file by.
  int flags = 0;

  // An UnbufferedFile has no caching or buffering, not even in kernel-space.
  //
  // The purpose of BufferedFile is to implement the same page cache mechanisms
  // some kernels implement, entirely in user-space. This has several advantages
  //  * Copies from a user-space page-cache are faster than a kernel-space page-cache.
  //  * Not all kernels implement page caching, e.g consoles.
  //  * Can explicitly manage caches to enable more optimization opportunities.
  //  * Can explicitly flush caches for data consistency.
  //  * Can have a page-cache on virtual files not backed by the OS.
  //
  // The O_DIRECT flag, if present, instructs the kernel not to back the file
  // with kernel page-cache. Leaving page caching on in the kernel would double
  // both the memory used by a BufferedFile and the number of copies made.
#if defined(O_DIRECT)
  flags |= O_DIRECT;
#endif

  if (_mode.contains('+')) {
    flags = O_RDWR;
  } else if (_mode[0] == 'r') {
    flags = O_RDONLY;
  } else {
    flags = O_WRONLY;
  }

  if (_mode[0] != 'r') {
    flags |= O_CREAT;
  }

  if (_mode[0] == 'w') {
    flags |= O_TRUNC;
  } else if (_mode[0] == 'a') {
    flags |= O_APPEND;
  }

  // Don't ever duplicate FDs on fork.
  flags |= O_CLOEXEC;

  int fd = open(_file_name.data(), flags, 0666);
  if (fd < 0) {
    return nullptr;
  }

  return reinterpret_cast<void*>(fd);
}

static bool close_file(void* _impl) {
  return close(impl(_impl)) == 0;
}

static bool read_file(void* _impl, Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  const auto result = pread(impl(_impl), _data, _size, _offset);
  if (result == -1) {
    n_bytes_ = 0;
    return false;
  }
  n_bytes_ = static_cast<Size>(result);
  return true;
}

static bool write_file(void* _impl, const Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  const auto result = pwrite(impl(_impl), _data, _size, _offset);
  if (result == -1) {
    n_bytes_ = 0;
    return false;
  }
  n_bytes_ = static_cast<Size>(result);
  return true;
}

static bool stat_file(void* _impl, Stream::Stat& stat_) {
  struct stat buf;
  if (fstat(impl(_impl), &buf) != -1) {
    stat_.size = buf.st_size;
    return true;
  }
  return false;
}
#elif defined(RX_PLATFORM_WINDOWS)
static void* open_file([[maybe_unused]] Memory::Allocator& _allocator, const char* _file_name, const char* _mode) {
  WideString file_name = String::format(_allocator, "%s", _file_name).to_utf16();

  DWORD dwDesiredAccess = 0;
  DWORD dwShareMode = 0;
  DWORD dwCreationDisposition = 0;
  DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

  // The '+' inside |_mode| indicates R+W
  if (strchr(_mode, '+')) {
    dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
    dwShareMode |= (FILE_SHARE_READ | FILE_SHARE_WRITE);
  } else if (*_mode == 'r') {
    // Read-only.
    dwDesiredAccess |= GENERIC_READ;
    dwShareMode |= FILE_SHARE_READ;
  } else {
    // Write-only.
    dwDesiredAccess |= GENERIC_WRITE;
    dwShareMode |= FILE_SHARE_WRITE;
  }

  if (*_mode == 'r') {
    // Read only if the file exists.
    dwCreationDisposition = OPEN_EXISTING;
  } else {
    // Write even if the file doesn't exist.
    dwCreationDisposition = OPEN_ALWAYS;
  }

  if (*_mode == 'w') {
    // When writing, truncate to 0 bytes existing files.
    dwCreationDisposition = CREATE_ALWAYS;
  } else if (*_mode == 'a') {
    // Appending.
    dwDesiredAccess |= FILE_APPEND_DATA;
  }

  HANDLE handle = CreateFileW(
    reinterpret_cast<LPCWSTR>(file_name.data()),
    dwDesiredAccess,
    dwShareMode,
    nullptr,
    dwCreationDisposition,
    dwFlagsAndAttributes,
    nullptr);

  return handle != INVALID_HANDLE_VALUE ? handle : nullptr;
}

static bool close_file(void* _impl) {
  return CloseHandle(impl(_impl));
}

static bool read_file(void* _impl, Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  OVERLAPPED overlapped{};
  overlapped.OffsetHigh = static_cast<Uint32>((_offset & 0xFFFFFFFF00000000_u64) >> 32);
  overlapped.Offset = static_cast<Uint32>((_offset & 0xFFFFFFFF_u64));

  long unsigned int n_read_bytes = 0;
  const bool result = ReadFile(
    impl(_impl),
    reinterpret_cast<void*>(_data),
    static_cast<DWORD>(Algorithm::min(_size, 0xFFFFFFFF_z)),
    &n_read_bytes,
    &overlapped);

  n_bytes_ = static_cast<Size>(n_read_bytes);

  if (!result && GetLastError() != ERROR_HANDLE_EOF) {
    n_bytes_ = 0;
    return false;
  }

  return true;
}

static bool write_file(void* _impl, const Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  OVERLAPPED overlapped{};
  overlapped.OffsetHigh = static_cast<Uint32>((_offset & 0xFFFFFFFF00000000_u64) >> 32);
  overlapped.Offset = static_cast<Uint32>((_offset & 0xFFFFFFFF_u64));

  long unsigned int n_write_bytes = 0;
  const bool result = WriteFile(
    impl(_impl),
    reinterpret_cast<const void*>(_data),
    static_cast<DWORD>(Algorithm::min(_size, 0xFFFFFFFF_z)),
    &n_write_bytes,
    &overlapped);

  n_bytes_ = static_cast<Size>(n_write_bytes);

  return result;
}

static bool stat_file(void* _impl, Stream::Stat& stat_) {
  BY_HANDLE_FILE_INFORMATION info;
  if (GetFileInformationByHandle(impl(_impl), &info)) {
    // Windows splits size into two 32-bit quanities. Reconstruct as 64-bit value.
    stat_.size = (static_cast<Uint64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    return true;
  }
  return false;
}
#endif

UnbufferedFile::UnbufferedFile(UnbufferedFile&& other_)
  : Context{Utility::move(other_)}
  , m_impl{Utility::exchange(other_.m_impl, nullptr)}
  , m_name{Utility::move(other_.m_name)}
  , m_mode{Utility::exchange(other_.m_mode, nullptr)}
{
}

UnbufferedFile::UnbufferedFile(Uint32 _flags, void* _impl, String&& name_, const char* _mode)
  : Context{_flags}
  , m_impl{_impl}
  , m_name{Utility::move(name_)}
  , m_mode{_mode}
{
}

UnbufferedFile& UnbufferedFile::operator=(UnbufferedFile&& file_) {
  if (this != &file_) {
    (void)close();
    Context::operator=(Utility::move(file_));
    m_impl = Utility::exchange(file_.m_impl, nullptr);
    m_name = Utility::move(file_.m_name);
    m_mode = Utility::exchange(file_.m_mode, nullptr);
  }
  return *this;
}

Optional<UnbufferedFile> UnbufferedFile::open(Memory::Allocator& _allocator,
  const StringView& _file_name, const StringView& _mode)
{
  if (auto name = _file_name.to_string(_allocator)) {
    if (auto file = open_file(_allocator, _file_name.data(), _mode.data())) {
      return UnbufferedFile{flags_from_mode(_mode), file, Utility::move(*name), _mode.data()};
    }
  }
  return nullopt;
}

Uint64 UnbufferedFile::on_read(Byte* _data, Uint64 _size, Uint64 _offset) {
  // Process reads in loop since |read_file| can read less than requested.
  Uint64 bytes = 0;
  while (bytes < _size) {
    Size read = 0;
    if (!read_file(m_impl, _data + bytes, _size - bytes, _offset + bytes, read) || !read) {
      break;
    }
    bytes += read;
  }
  return bytes;
}

Uint64 UnbufferedFile::on_write(const Byte* _data, Uint64 _size, Uint64 _offset) {
  // Process writes in loop since |write_file| can write less than requested.
  Uint64 bytes = 0;
  while (bytes < _size) {
    Size wrote = 0;
    if (!write_file(m_impl, _data + bytes, _size - bytes, _offset + bytes, wrote) || !wrote) {
      break;
    }
    bytes += wrote;
  }
  return bytes;
}

Optional<Stream::Stat> UnbufferedFile::on_stat() const {
  if (Stream::Stat result; stat_file(m_impl, result)) {
    return result;
  }
  return nullopt;
}

bool UnbufferedFile::on_truncate(Uint64 _size) {
#if defined(RX_PLATFORM_POSIX)
  return ftruncate64(impl(m_impl), _size) == 0;
#elif defined(RX_PLATFORM_WINDOWS)
  return SetEndOfFile(impl(m_impl));
#endif
  return false;
}

Uint64 UnbufferedFile::on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size) {
#if defined(RX_PLATFORM_LINUX)
  off64_t dst = _dst_offset;
  off64_t src = _src_offset;
  // Process copy in loop since |copy_file_range| can do less than requested.
  Uint64 bytes = 0;
  while (bytes < _size) {
    auto copy = copy_file_range(impl(m_impl), &src, impl(m_impl), &dst, _size - bytes, 0);
    if (copy <= 0) {
      break;
    }
    bytes += Uint64(copy);
  }
  return bytes;
#endif
  // Use the fallback implementation which just copies in blocks.
  return Context::on_copy(_dst_offset, _src_offset, _size);
}

bool UnbufferedFile::close() {
  if (m_impl && close_file(m_impl)) {
    m_impl = nullptr;
    return true;
  }
  return false;
}

const String& UnbufferedFile::name() const & {
  return m_name;
}

// When reading in the entire contents of a file use an UnbufferedFile.
Optional<LinearBuffer> read_binary_file(Memory::Allocator& _allocator,
  const StringView& _file_name)
{
  if (auto file = UnbufferedFile::open(_allocator, _file_name, "r")) {
    return file->read_binary(_allocator);
  }
  logger->error("failed to open file '%s' [%s]", _file_name);
  return nullopt;
}

Optional<LinearBuffer> read_text_file(Memory::Allocator& _allocator,
  const StringView& _file_name)
{
  if (auto file = UnbufferedFile::open(_allocator, _file_name, "r")) {
    return file->read_text(_allocator);
  }
  logger->error("failed to open file '%s'", _file_name);
  return nullopt;
}

} // namespace Rx::Filesystem