#include <string.h> // memcpy

#include "rx/core/config.h" // RX_PLATFORM_*
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/linear_buffer.h"

#if defined(RX_PLATFORM_POSIX)
#include <dirent.h> // DIR, struct dirent, opendir, readdir, rewinddir, closedir
#include <sys/stat.h> // mkdir, mode_t
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // WIN32_FIND_DATAW, HANDLE, LPCWSTR, INVALID_HANDLE_VALUE, FILE_ATTRIBUTE_DIRECTORY, FindFirstFileW, FindNextFileW, FindClose
#else
#error "missing directory implementation"
#endif

#include "rx/core/filesystem/directory.h"

namespace Rx::Filesystem {

Optional<Directory> Directory::Item::as_directory() const {
  if (is_directory()) {
    auto& allocator = m_directory->allocator();

    // Construct the path in-place as one allocation, as operator+ may require
    // up to three allocations to format the final path.
    const auto& root = m_directory->path();
    const auto& filename = name();

    // Storage for "${root}/${filename}" including null-terminator.
    const auto length = root.size() + 1 + filename.size() + 1;

    if (auto data = allocator.allocate(length)) {
      // Copy root without null-terminator.
      memcpy(data, root.data(), root.size());

      // Write path separator in the middle.
      data[root.size()] = '/';

      // Copy filename with null-terminator to also terminate our string.
      memcpy(data + root.size() + 1, filename.data(), filename.size() + 1);

      return Directory::open(allocator, Memory::View{&allocator, data, length, length});
    }
  }

  return nullopt;
}

#if defined(RX_PLATFORM_WINDOWS)
struct FindContext {
  LinearBuffer path_data;
  WIN32_FIND_DATAW find_data;
  HANDLE handle;
};
#endif

Optional<Directory> Directory::open(Memory::Allocator& _allocator, const String& _path) {
#if defined(RX_PLATFORM_POSIX)
  // Make a copy of the path name to store in Directory.
  auto path_copy = Utility::copy(_path);
  if (!path_copy) {
    return nullopt;
  }

  if (auto impl = opendir(_path.data())) {
    return Directory{_allocator, Utility::move(*path_copy), reinterpret_cast<void*>(impl)};
  }
#elif defined(RX_PLATFORM_WINDOWS)
  // The only thing we can cache between reuses of a directory object is the
  // path conversion and the initial find handle on Windows. Subsequent reuses
  // will need to reopen the directory.
  auto context = _allocator.create<FindContext>();
  if (!context) {
    return nullopt;
  }

  // Convert |m_path| to UTF-16 for Windows.
  const auto path_utf16 = m_path.to_utf16();
  static constexpr const wchar_t PATH_EXTRA[] = L"\\*";
  LinearBuffer path_data{_allocator};
  if (!path_data.resize(path_utf16.size() * 2 + sizeof PATH_EXTRA)) {
    return nullopt;
  }

  memcpy(path_data.data(), path_utf16.data(), path_utf16.size() * 2);
  memcpy(path_data.data() + path_utf16.size() * 2, PATH_EXTRA, sizeof PATH_EXTRA);

  // Execute one FindFirstFileW to check if the directory exists.
  const auto path = reinterpret_cast<const LPCWSTR>(path_data.data());
  const HANDLE handle = FindFirstFileW(path, &context->find_data);
  if (handle != INVALID_HANDLE_VALUE) {
    // The directory exists and has been opened. Cache the handle and the path
    // conversion for |each|.
    context->handle = handle;
    context->path_data = Utility::move(path_data);
    return Directory{_allocator, Utility::move(*path_copy), reinterpret_cast<void*>(context)};
  } else {
    _allocator.destroy<FindContext>(context);
  }
#endif
  return nullopt;
}

void Directory::release() {
#if defined(RX_PLATFORM_POSIX)
  if (m_impl) {
    closedir(reinterpret_cast<DIR*>(m_impl));
  }
#elif defined(RX_PLATFORM_WINDOWS)
  if (m_impl) {
    allocator().destroy<FindContext>(m_impl);
  }
#endif
}

bool Directory::enumerate(Function<bool(Item&&)>&& _function) {
  if (!m_impl) {
    return false;
  }

  bool result = true;

#if defined(RX_PLATFORM_POSIX)
  auto dir = reinterpret_cast<DIR*>(m_impl);
  struct dirent* next = readdir(dir);

  // Possible if the directory is removed between subsequent calls to |each|.
  if (!next) {
    // The directory is no longer valid, let operator bool reflect this.
    closedir(dir);
    m_impl = nullptr;
  }

  while (next) {
    // Skip '.' and '..'.
    while (next && next->d_name[0] == '.' && !(next->d_name[1 + (next->d_name[1] == '.')])) {
      next = readdir(dir);
    }

    if (next) {
      if (next->d_type == DT_DIR) {
        if (!_function({this, {allocator(), next->d_name}, Item::Type::DIRECTORY})) {
          result = false;
          break;
        }
      } else if (next->d_type == DT_REG) {
        if (!_function({this, {allocator(), next->d_name}, Item::Type::FILE})) {
          result = false;
          break;
        }
      }
      next = readdir(dir);
    } else {
      break;
    }
  }

  rewinddir(dir);
#elif defined(RX_PLATFORM_WINDOWS)
  auto* context = reinterpret_cast<FindContext*>(m_impl);

  // The handle has been closed, this can only happen when reusing the directory
  // object, i.e multiple calls to |each|.
  if (context->handle == INVALID_HANDLE_VALUE) {
    // Attempt to reopen the directory, since Windows lacks rewinddir.
    const auto path = reinterpret_cast<const LPCWSTR>(context->path_data.data());
    const auto handle = FindFirstFileW(path, &context->find_data);
    if (handle != INVALID_HANDLE_VALUE) {
      context->handle = handle;
    } else {
      // Destroy the Context and clear |m_impl| out so operator bool reflects this.
      allocator().destroy<FindContext>(context);
      m_impl = nullptr;
      return false;
    }
  }

  // Enumerate each file in the directory.
  for (;;) {
    // Skip '.' and '..'.
    if (context->find_data.cFileName[0] == L'.'
      && !context->find_data.cFileName[1 + !!(context->find_data.cFileName[1] == L'.')])
    {
      if (!FindNextFileW(context->handle, &context->find_data)) {
        break;
      }
      continue;
    }

    const WideString utf16_name = reinterpret_cast<Uint16*>(&context->find_data.cFileName);
    const Item::Type kind = context->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
      ? Item::Type::DIRECTORY
      : Item::Type::FILE;

    if (!_function({this, utf16_name.to_utf8(), kind})) {
      result = false;
      break;
    }

    if (!FindNextFileW(context->handle, &context->find_data)) {
      break;
    }
  }

  // There's no way to rewinddir on Windows, so just close the the find handle
  // and clear it out in the Context so subequent calls to |each| reopen it
  // instead.
  FindClose(context->handle);
  context->handle = INVALID_HANDLE_VALUE;
#endif

  return result;
}

bool create_directory(const String& _path) {
#if defined(RX_PLATFORM_POSIX)
  auto perms = 0;
  perms |= S_IRUSR; // Read bit for owner.
  perms |= S_IWUSR; // Write bit for owner.
  perms |= S_IXUSR; // Repurposed in POSIX for directories to mean "searchable".
  return mkdir(_path.data(), perms) == 0;
#elif defined(RX_PLATFORM_WINDOWS)
  // Use CreateDirectoryW so that Unicode |_path| names are allowed. Windows
  // also requires that "\\?\" be prepended to the |_path| to remove the 248
  // character limit. Windows 10 doesn't require this, but it doesn't hurt
  // to add it either.
  const auto path = ("\\\\?\\" + _path).to_utf16();
  return CreateDirectoryW(reinterpret_cast<LPCWSTR>(path.data()), nullptr);
#endif
}

} // namespace Rx::Filesystem
