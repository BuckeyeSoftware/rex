#ifndef RX_CORE_FILESYSTEM_PATH_RESOLVER_H
#define RX_CORE_FILESYSTEM_PATH_RESOLVER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

namespace rx::filesystem {

// # Incremental Path Resolver
//
// An incremental path resolver that generates a fully qualified path URI for
// the virtual file system.
//
// You can append to the path sub paths or file names and it will form a fully
// qualified path name. Alternatively, you can push individual characters into
// the resolver and as it recieves them it'll translate the path accordingly.
//
// When you're ready to use the qualified path, call finalize().
struct path_resolver {
  constexpr path_resolver();
  constexpr path_resolver(memory::allocator* _allocator);

  [[nodiscard]] bool append(const string& _path);
  [[nodiscard]] bool append(const char* _path);
  [[nodiscard]] bool push(int _ch);

  const char* path() const;

  string operator[](rx_size _index) {
    const auto beg = _index ? m_stack.data[_index - 1] : 0;
    const auto end = m_stack.data[_index];
    return {m_data.allocator(), m_data.data() + beg, m_data.data() + end};
  }

  rx_size parts() const {
    return m_stack.size;
  }

private:
  struct stack {
    constexpr stack();

    bool push();
    rx_size pop();

    union {
      rx_size head;
      rx_size data[255];
    };
    rx_size size;
    rx_size next;
  };

  [[nodiscard]] bool reserve_more(rx_size _size);

  vector<char> m_data;
  stack m_stack;
  rx_size m_dots;
};

inline constexpr path_resolver::stack::stack()
  : head{1}
  , size{1}
  , next{-1_z}
{
}

inline constexpr path_resolver::path_resolver()
  : path_resolver{&memory::g_system_allocator}
{
}

inline constexpr path_resolver::path_resolver(memory::allocator* _allocator)
  : m_data{_allocator}
  , m_dots{0}
{
}

inline const char* path_resolver::path() const {
  return m_data.data();
}

inline bool path_resolver::append(const string& _path) {
  return reserve_more(_path.size()) && append(_path.data());
}

inline bool path_resolver::reserve_more(rx_size _size) {
  return m_data.reserve(m_data.capacity() + _size);
}

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_PATH_RESOLVER_H
