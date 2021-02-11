#ifndef RX_CORE_FILESYSTEM_DIRECTORY_H
#define RX_CORE_FILESYSTEM_DIRECTORY_H
#include "rx/core/string.h"
#include "rx/core/function.h"
#include "rx/core/optional.h"

namespace Rx::Filesystem {

struct RX_API Directory {
  RX_MARK_NO_COPY(Directory);

  Directory(Memory::Allocator& _allocator, const char* _path);
  Directory(Memory::Allocator& _allocator, const String& _path);
  Directory(Memory::Allocator& _allocator, String&& path_);
  Directory(const char* _path);
  Directory(const String& _path);
  Directory(String&& path_);
  Directory(Directory&& directory_);
  ~Directory();

  operator bool() const;

  template<typename F>
  bool each(F&& function_);

  struct Item {
    RX_MARK_NO_COPY(Item);
    RX_MARK_NO_MOVE(Item);

    bool is_file() const;
    bool is_directory() const;
    const String& name() const;
    String&& name();

    const Directory& directory() const &;

    Optional<Directory> as_directory() const;

  private:
    friend struct Directory;

    enum class Type : Uint8 {
      FILE,
      DIRECTORY
    };

    Item(const Directory* _directory, String&& name_, Type _type);

    const Directory* m_directory;
    String m_name;
    Type m_type;
  };

  const String& path() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  using Enumerator = Function<bool(Item&&)>;

  // enumerate directory with |_function| being called for each item
  // NOTE: does not consider hidden files, symbolic links, block devices, or ..
  bool enumerate(Enumerator&& _function);

  Memory::Allocator* m_allocator;
  String m_path;
  void* m_impl;
};

inline Directory::Directory(Memory::Allocator& _allocator, const char* _path)
  : Directory{_allocator, String{_allocator, _path}}
{
}

inline Directory::Directory(Memory::Allocator& _allocator, const String& _path)
  : Directory{_allocator, String{_allocator, _path}}
{
}

inline Directory::Directory(const char* _path)
  : Directory{Memory::SystemAllocator::instance(), _path}
{
}

inline Directory::Directory(const String& _path)
  : Directory{Memory::SystemAllocator::instance(), _path}
{
}

inline Directory::Directory(String&& path_)
  : Directory{Memory::SystemAllocator::instance(), Utility::move(path_)}
{
}

inline Directory::Directory(Directory&& directory_)
  : m_allocator{directory_.m_allocator}
  , m_path{Utility::move(directory_.m_path)}
  , m_impl{Utility::exchange(directory_.m_impl, nullptr)}
{
}

RX_HINT_FORCE_INLINE Directory::operator bool() const {
  return m_impl;
}

template<typename F>
bool Directory::each(F&& function_) {
  // When |function_| doesn't return boolean result.
  if constexpr (!Traits::IS_SAME<Traits::InvokeResult<F, Item&&>, bool>) {
    // Wrap the function dispatch since |enumerate| expects something that
    // returns boolean to indicate continued enumeration.
    auto wrap = [function = Utility::move(function_)](Item&& item_) {
      function(Utility::move(item_));
      return true;
    };
    if (auto fun = Enumerator::create(Utility::move(wrap))) {
      return enumerate(Utility::move(*fun));
    }
  } else if (auto fun = Enumerator::create(Utility::move(function_))) {
    return enumerate(Utility::move(*fun));
  }
  return false;
}

RX_HINT_FORCE_INLINE const String& Directory::path() const & {
  return m_path;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Directory::allocator() const {
  return *m_allocator;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_file() const {
  return m_type == Type::FILE;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_directory() const {
  return m_type == Type::DIRECTORY;
}

RX_HINT_FORCE_INLINE const Directory& Directory::Item::directory() const & {
  return *m_directory;
}

RX_HINT_FORCE_INLINE const String& Directory::Item::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE String&& Directory::Item::name() {
  return Utility::move(m_name);
}

RX_HINT_FORCE_INLINE Directory::Item::Item(const Directory* _directory, String&& name_, Type _type)
  : m_directory{_directory}
  , m_name{Utility::move(name_)}
  , m_type{_type}
{
}

bool create_directory(const String& _path);

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_DIRECTORY_H
