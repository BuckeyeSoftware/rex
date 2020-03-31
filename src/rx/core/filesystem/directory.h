#ifndef RX_CORE_FILESYSTEM_DIRECTORY_H
#define RX_CORE_FILESYSTEM_DIRECTORY_H
#include "rx/core/string.h"
#include "rx/core/function.h"

#include "rx/core/concepts/no_copy.h"

#include "rx/core/hints/empty_bases.h"

namespace rx::filesystem {

struct RX_HINT_EMPTY_BASES directory
  : concepts::no_copy
{
  directory(memory::allocator& _allocator, const char* _path);
  directory(memory::allocator& _allocator, const string& _path);
  directory(memory::allocator& _allocator, string&& path_);
  directory(const char* _path);
  directory(const string& _path);
  directory(string&& path_);
  ~directory();

  operator bool() const;

  struct item {
    enum class type : rx_u8 {
      k_file,
      k_directory
    };

    bool is_file() const;
    bool is_directory() const;
    const string& name() const;
    string&& name();

  private:
    friend struct directory;

    item(string&& name_, type _type);

    string m_name;
    type m_type;
  };

  // enumerate directory with |_function| being called for each item
  // NOTE: does not consider hidden files, symbolic links, block devices, or ..
  void each(function<void(item&&)>&& _function);

  const string& path() const &;

  constexpr memory::allocator& allocator() const;

private:
  memory::allocator& m_allocator;
  string m_path;
  void* m_impl;
};

inline directory::directory(memory::allocator& _allocator, const char* _path)
  : directory{_allocator, string{_allocator, _path}}
{
}

inline directory::directory(memory::allocator& _allocator, const string& _path)
  : directory{_allocator, string{_allocator, _path}}
{
}

inline directory::directory(const char* _path)
  : directory{memory::system_allocator::instance(), _path}
{
}

inline directory::directory(const string& _path)
  : directory{memory::system_allocator::instance(), _path}
{
}

inline directory::directory(string&& path_)
  : directory{memory::system_allocator::instance(), utility::move(path_)}
{
}

RX_HINT_FORCE_INLINE directory::operator bool() const {
  return m_impl;
}

RX_HINT_FORCE_INLINE const string& directory::path() const & {
  return m_path;
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& directory::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE bool directory::item::is_file() const {
  return m_type == type::k_file;
}

RX_HINT_FORCE_INLINE bool directory::item::is_directory() const {
  return m_type == type::k_directory;
}

RX_HINT_FORCE_INLINE const string& directory::item::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE string&& directory::item::name() {
  return utility::move(m_name);
}

RX_HINT_FORCE_INLINE directory::item::item(string&& name_, type _type)
  : m_name{utility::move(name_)}
  , m_type{_type}
{
}

bool create_directory(const string& _path);

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_DIRECTORY_H
