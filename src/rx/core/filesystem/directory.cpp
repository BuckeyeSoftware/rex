#if defined(RX_PLATFORM_POSIX)
#include <dirent.h>
#elif defined(RX_PLATFORM_WINDOWS)
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error "missing directory implementation"
#endif

#include <rx/core/filesystem/directory.h>

namespace rx::filesystem {

directory::directory(memory::allocator* _allocator, const char* _path)
  : m_allocator{_allocator}
{
#if defined(RX_PLATFORM_POSIX)
  m_impl = reinterpret_cast<void*>(opendir(_path));
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO
#endif
}

directory::~directory() {
#if defined(RX_PLATFORM_POSIX)
  if (m_impl) {
    closedir(reinterpret_cast<DIR*>(m_impl));
  }
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO
#endif
}

void directory::each(function<void(const item&)>&& _function) {
  RX_ASSERT(m_impl, "invalid directory");

#if defined(RX_PLATFORM_POSIX)
  auto dir{reinterpret_cast<DIR*>(m_impl)};
  struct dirent* next{readdir(dir)};
  while (next) {
    // skip '..'
    while (next && next->d_name[0] == '.' && !(next->d_name[1 + (next->d_name[1] == '.')])) {
      next = readdir(dir);
    }

    if (next) {
      // only accept regular files and directories, symbol links are not allowed
      switch (next->d_type) {
      case DT_DIR:
        _function({m_allocator, next->d_name, item::type::k_directory});
        break;
      case DT_REG:
        _function({m_allocator, next->d_name, item::type::k_file});
        break;
      }

      next = readdir(dir);
    } else {
      break;
    }
  }
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO
#endif
}

} // namespace rx::filesystem