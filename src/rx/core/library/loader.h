#ifndef RX_CORE_LIBRARY_LOADER_H
#define RX_CORE_LIBRARY_LOADER_H
#include "rx/core/string.h"

namespace Rx::library {

struct RX_API Loader {
  RX_MARK_NO_COPY(Loader);

  constexpr Loader();
  Loader(Loader&& _loader);
  ~Loader();

  Loader& operator=(Loader&& loader_);

  static Optional<Loader> open(Memory::Allocator& _allocator, const String& _file_name);

  // Link the function pointer given by |function_| up with the symbol in
  // the library given by |_symbol_name|. Returns true on success, false on
  // failure.
  template<typename F>
  bool link(F*& function_, const char* _symbol_name) const;

  template<typename F>
  bool link(F*& function_, const String& _symbol_name) const;

private:
  constexpr Loader(Memory::Allocator& _allocator, void* _handle);

  void close_unlocked();

  // Returns nullptr when |_symbol_name| isn't found.
  void* address_of(const char* _symbol_name) const;

  Memory::Allocator* m_allocator;
  void* m_handle;
};

inline constexpr Loader::Loader()
  : m_allocator{nullptr}
  , m_handle{nullptr}
{
}

inline Loader::Loader(Loader&& loader_)
  : m_allocator{loader_.m_allocator}
  , m_handle{Utility::exchange(loader_.m_handle, nullptr)}
{
}

inline constexpr Loader::Loader(Memory::Allocator& _allocator, void* _handle)
  : m_allocator{&_allocator}
  , m_handle{_handle}
{
}

template<typename F>
bool Loader::link(F*& function_, const char* _symbol_name) const {
  if (const auto proc = address_of(_symbol_name)) {
    *reinterpret_cast<void**>(&function_) = proc;
    return true;
  }
  return false;
}

template<typename F>
bool Loader::link(F*& function_, const String& _symbol_name) const {
  return link(function_, _symbol_name.data());
}

} // namespace Rx::Library

#endif // RX_CORE_LIBRARY_LOADER_H
