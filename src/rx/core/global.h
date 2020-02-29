#ifndef RX_CORE_GLOBAL_H
#define RX_CORE_GLOBAL_H
#include "rx/core/assert.h"
#include "rx/core/intrusive_xor_list.h"
#include "rx/core/memory/uninitialized_storage.h"

namespace rx {

// Constant, sharable properties for a given global<T>.
//
// This saves 4 bytes in global_node.
struct global_shared {
  rx_u16 size;
  rx_u16 alignment;
  void (*finalizer)(void* _data);
};

// 32-bit: 32 bytes
// 64-bit: 64 bytes
struct global_node {
  template<typename T, typename... Ts>
  global_node(const char* _group, const char* _name,
    memory::uninitialized_storage<T>& _storage, const global_shared* _shared,
    Ts&&... _arguments);

  void init();
  void fini();

  template<typename... Ts>
  void init(Ts&&... _arguments);

  const char* name() const;

  rx_byte* data();
  const rx_byte* data() const;

  template<typename T>
  T* cast();

  template<typename T>
  const T* cast() const;

private:
  friend struct globals;
  friend struct global_group;

  void init_global();
  void fini_global();

  template<typename F, typename... Rs>
  struct arguments : arguments<Rs...> {
    constexpr arguments(F&& _first, Rs&&... _rest)
      : arguments<Rs...>(utility::forward<Rs>(_rest)...)
      , first{utility::forward<F>(_first)}
    {
    }
    F first;
  };
  template<typename F>
  struct arguments<F> {
    constexpr arguments(F&& _first)
      : first{utility::forward<F>(_first)}
    {
    }
    F first;
  };

  template<rx_size I, typename F, typename... Rs>
  struct read_argument {
    static auto value(const arguments<F, Rs...>* _arguments) {
      return read_argument<I - 1, Rs...>::value(_arguments);
    }
  };
  template<typename F, typename... Rs>
  struct read_argument<0, F, Rs...> {
    static F value(const arguments<F, Rs...>* _arguments) {
      return _arguments->first;
    }
  };
  template<rx_size I, typename F, typename... Rs>
  static auto argument(const arguments<F, Rs...>* _arguments) {
    return read_argument<I, F, Rs...>::value(_arguments);
  }

  template<rx_size...>
  struct unpack_sequence {};
  template<rx_size N, rx_size... Ns>
  struct unpack_arguments
    : unpack_arguments<N - 1, N - 1, Ns...>
  {
  };
  template<rx_size... Ns>
  struct unpack_arguments<0, Ns...> {
    using type = unpack_sequence<Ns...>;
  };

  template<typename... Ts>
  static void construct_arguments(rx_byte* _argument_store, Ts... _arguments) {
    utility::construct<arguments<Ts...>>(_argument_store,
      utility::forward<Ts>(_arguments)...);
  }

  template<typename T, typename... Ts, rx_size... Ns>
  static void construct_global(unpack_sequence<Ns...>, rx_byte* _storage,
    [[maybe_unused]] rx_byte* _argument_store)
  {
    utility::construct<T>(_storage,
      argument<Ns>(reinterpret_cast<arguments<Ts...>*>(_argument_store))...);
  }

  // Combine both operations into a single function so we only have to store
  // a single function pointer rather than multiple.
  //
  // This saves 4 bytes on 32-bit.
  // This saved 8 bytes on 64-bit.
  enum class storage_mode {
    k_init_global,
    k_fini_arguments
  };

  template<typename T, typename... Ts>
  static void storage_dispatch(storage_mode _mode, rx_byte* _global_store,
    rx_byte* _argument_store)
  {
    switch (_mode) {
    case storage_mode::k_init_global:
      using pack = typename unpack_arguments<sizeof...(Ts)>::type;
      construct_global<T, Ts...>(pack{}, _global_store, _argument_store);
      break;
    case storage_mode::k_fini_arguments:
      if constexpr(sizeof...(Ts) != 0) {
        utility::destruct<arguments<Ts...>>(_argument_store);
      }
      break;
    }
  }

  static rx_byte* allocate_arguments(rx_size _size);
  static void deallocate_arguments(rx_byte* _arguments);

  const global_shared* m_shared;

  intrusive_xor_list::node m_grouped;
  intrusive_xor_list::node m_ungrouped;

  const char* m_group;
  const char* m_name;

  rx_byte* m_argument_store;

  void (*m_storage_dispatch)(storage_mode _mode, rx_byte* _global_store,
    rx_byte* _argument_store);

  enum : rx_u16 {
    k_enabled     = 1 << 0,
    k_initialized = 1 << 1,
    k_arguments   = 1 << 2
  };

  rx_u16 m_flags;
};

// 32-bit: 32 + sizeof(T) bytes
// 64-bit: 64 + sizeof(T) bytes
template<typename T>
struct global {
  static inline const global_shared s_shared = {
    sizeof(T),
    alignof(T),
    &utility::destruct<T>
  };

  template<typename... Ts>
  global(const char* _group, const char* _name, Ts&&... _arguments);

  constexpr const char* name() const;

  constexpr T* operator&();
  constexpr const T* operator&() const;

  constexpr T& operator*();
  constexpr const T& operator*() const;

  constexpr T* operator->();
  constexpr const T* operator->() const;

  template<typename... Ts>
  auto operator()(Ts&&... _arguments);

  constexpr T* data();
  constexpr const T* data() const;

private:
  global_node m_node;
  memory::uninitialized_storage<T> m_global_store;
};

// 32-bit: 20 bytes
// 64-bit: 40 bytes
struct global_group {
  global_group(const char* _name);

  constexpr const char* name() const;

  global_node* find(const char* _name);

  void init();
  void fini();

  template<typename F>
  void each(F&& _function);

private:
  friend struct globals;
  friend struct global_node;

  void init_global();
  void fini_global();

  const char* m_name;

  // Nodes for this group. This is constructed after a call to |globals::link|.
  intrusive_xor_list m_list;

  // Link for global linked-list of groups in |globals|.
  intrusive_xor_list::node m_link;
};

struct globals {
  static global_group* find(const char* _name);

  // Goes over global linked-list of groups, adding nodes to the group
  // if the |global_node::m_group| matches the group name.
  static void link();

  static void init();
  static void fini();

private:
  friend struct global_node;
  friend struct global_group;

  static void link(global_node* _node);
  static void link(global_group* _group);

  // Global linked-list of groups.
  static inline intrusive_xor_list s_group_list;

  // Global linked-list of ungrouped nodes.
  static inline intrusive_xor_list s_node_list;
};

// global_node
template<typename T, typename... Ts>
inline global_node::global_node(const char* _group, const char* _name,
  memory::uninitialized_storage<T>& _global_store, const global_shared* _shared,
  Ts&&... _arguments)
  : m_shared{_shared}
  , m_group{_group ? _group : "system"}
  , m_name{_name}
  , m_storage_dispatch{storage_dispatch<T, Ts...>}
  , m_flags{k_enabled}
{
  // The |_global_store| should be immediately after |this|.
  RX_ASSERT(reinterpret_cast<rx_uintptr>(&_global_store)
    == reinterpret_cast<rx_uintptr>(this + 1), "misalignment");

  if constexpr (sizeof...(Ts) != 0) {
    m_argument_store = allocate_arguments(sizeof(arguments<Ts...>));
    construct_arguments(m_argument_store, utility::forward<Ts>(_arguments)...);
    m_flags |= k_arguments;
  }

  globals::link(this);
}

template<typename... Ts>
inline void global_node::init(Ts&&... _arguments) {
  static_assert(sizeof...(Ts) != 0,
    "use void init() for default construction");

  if (m_flags & k_arguments) {
    m_storage_dispatch(storage_mode::k_fini_arguments, data(), m_argument_store);
  }

  construct_arguments(m_argument_store, utility::forward<Ts>(_arguments)...);

  init();
}

inline const char* global_node::name() const {
  return m_name;
}

inline rx_byte* global_node::data() {
  // The layout of a global<T> is such that the storage for it is right after
  // the node. That node is |this|, this puts the storage one-past |this|.
  return reinterpret_cast<rx_byte*>(this + 1);
}

inline const rx_byte* global_node::data() const {
  return reinterpret_cast<const rx_byte*>(this + 1);
}

template<typename T>
inline T* global_node::cast() {
  RX_ASSERT(m_shared->size == sizeof(T), "invalid type cast");
  RX_ASSERT(m_shared->alignment == alignof(T), "invalid type cast");
  return reinterpret_cast<T*>(data());
}

template<typename T>
inline const T* global_node::cast() const {
  RX_ASSERT(m_shared->size == sizeof(T), "invalid type cast");
  RX_ASSERT(m_shared->alignment == alignof(T), "invalid type cast");
  return reinterpret_cast<const T*>(data());
}

// global_group
inline global_group::global_group(const char* _name)
  : m_name{_name}
{
  globals::link(this);
}

inline constexpr const char* global_group::name() const {
  return m_name;
}

template<typename F>
inline void global_group::each(F&& _function) {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    _function(node.data());
  }
}

// global
template<typename T>
template<typename... Ts>
inline global<T>::global(const char* _group, const char* _name, Ts&&... _arguments)
  : m_node{_group, _name, m_global_store, &s_shared, utility::forward<Ts>(_arguments)...}
{
}

template<typename T>
inline constexpr const char* global<T>::name() const {
  return m_node.name();
}

template<typename T>
inline constexpr T* global<T>::operator&() {
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* global<T>::operator&() const {
  return m_global_store.data();
}

template<typename T>
inline constexpr T& global<T>::operator*() {
  return *m_global_store.data();
}

template<typename T>
constexpr const T& global<T>::operator*() const {
  return *m_global_store.data();
}

template<typename T>
constexpr T* global<T>::operator->() {
  return m_global_store.data();
}

template<typename T>
constexpr const T* global<T>::operator->() const {
  return m_global_store.data();
}

template<typename T>
template<typename... Ts>
inline auto global<T>::operator()(Ts&&... _arguments) {
  return operator*()(utility::forward<Ts>(_arguments)...);
}

template<typename T>
inline constexpr T* global<T>::data() {
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* global<T>::data() const {
  return m_global_store.data();
}

} // namespace rx

#endif // RX_CORE_GLOBAL_H
