#ifndef RX_CORE_LIBRARY_LOADER_H
#define RX_CORE_LIBRARY_LOADER_H
#include "rx/core/string.h"

namespace Rx::Library {

struct RX_API Loader {
  RX_MARK_NO_COPY(Loader);

  constexpr Loader();
  Loader(Loader&& _loader);
  ~Loader();

  Loader& operator=(Loader&& loader_);

  /// \brief Opens dynamic library
  ///
  /// \param _allocator The allocator to use for allocations during load.
  /// \param _file_name The name of the library to open, ecluding file extension.
  ///
  /// \return On success a library, otherwise nullopt.
  static Optional<Loader> open(Memory::Allocator& _allocator, const StringView& _file_name);

  /// \brief Link a function
  ///
  /// Link the function pointer given by \p function_ up with the symbol in
  /// the library given by \p _symbol_name. Returns true on success, false on
  /// failure.
  ///
  /// \param function_ The function to link.
  /// \param _symbol_name The symbol to lookfor and link to \p function_.
  ///
  /// \return \c true on success, \c false otherwise.
  template<typename F>
  bool link(F*& function_, const StringView& _symbol_name) const;

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
bool Loader::link(F*& function_, const StringView& _symbol_name) const {
  if (const auto proc = address_of(_symbol_name.data())) {
    *reinterpret_cast<void**>(&function_) = proc;
    return true;
  }
  return false;
}

} // namespace Rx::Library

#endif // RX_CORE_LIBRARY_LOADER_H
