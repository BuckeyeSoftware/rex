#ifndef RX_CORE_LIBRARY_LOADER_H
#define RX_CORE_LIBRARY_LOADER_H
#include "rx/core/string.h"
#include "rx/core/optional.h"

namespace rx::library {

struct RX_HINT_EMPTY_BASES loader
  : concepts::no_copy
{
  loader(const string& _file_name);
  loader(memory::allocator& _allocator, const string& _file_name);
  loader(loader&& _loader);
  ~loader();

  loader& operator=(loader&& loader_);

  operator bool() const;
  bool is_valid() const;

  // Link the function pointer given by |function_| up with the symbol in
  // the library given by |_symbol_name|. Returns true on success, false on
  // failure.
  template<typename F>
  bool link(F*& function_, const char* _symbol_name) const;

  template<typename F>
  bool link(F*& function_, const string& _symbol_name) const;

private:
  void close_unlocked();

  // Returns nullptr when |_symbol_name| isn't found.
  void* address_of(const char* _symbol_name) const;

  ref<memory::allocator> m_allocator;
  void* m_handle;
};

inline loader::loader(const string& _file_name)
  : loader{memory::system_allocator::instance(), _file_name}
{
}

inline loader::loader(loader&& loader_)
  : m_allocator{loader_.m_allocator}
  , m_handle{utility::exchange(loader_.m_handle, nullptr)}
{
}

inline loader::operator bool() const {
  return m_handle != nullptr;
}

inline bool loader::is_valid() const {
  return m_handle != nullptr;
}

template<typename F>
inline bool loader::link(F*& function_, const char* _symbol_name) const {
  RX_ASSERT(m_handle, "no handle");
  if (const auto proc = address_of(_symbol_name)) {
    *reinterpret_cast<void**>(&function_) = proc;
    return true;
  }
  return false;
}

template<typename F>
inline bool loader::link(F*& function_, const string& _symbol_name) const {
  return link(function_, _symbol_name.data());
}

} // namespace rx::library

#endif // RX_CORE_LIBRARY_LOADER_H
