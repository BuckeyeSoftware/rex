#ifndef RX_CORE_GLOBAL_H
#define RX_CORE_GLOBAL_H
#include "rx/core/tagged_ptr.h"
#include "rx/core/intrusive_xor_list.h"
#include "rx/core/memory/uninitialized_storage.h"

namespace rx {

// 32-bit: 24 bytes
// 64-bit: 48 bytes
struct global_node {
  template<typename T, typename... Ts>
  global_node(const char* _group, const char* _name,
    memory::uninitialized_storage<T>& _storage, Ts&&... _arguments);

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

  template<typename T>
  void validate_cast_for() const;

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

  // Combine multiple operations into a single function so we only have to store
  // a single function pointer rather than multiple.
  enum class storage_mode {
    k_init_global,
    k_fini_global,
    k_traits_global,
    k_fini_arguments
  };

  template<typename T, typename... Ts>
  static rx_u32 storage_dispatch(storage_mode _mode,
    [[maybe_unused]] rx_byte* _global_store, [[maybe_unused]] rx_byte* _argument_store)
  {
    switch (_mode) {
    case storage_mode::k_init_global:
      using pack = typename unpack_arguments<sizeof...(Ts)>::type;
      construct_global<T, Ts...>(pack{}, _global_store, _argument_store);
      break;
    case storage_mode::k_fini_global:
      utility::destruct<T>(_global_store);
      break;
    case storage_mode::k_fini_arguments:
      if constexpr(sizeof...(Ts) != 0) {
        utility::destruct<arguments<Ts...>>(_argument_store);
      }
      break;
    case storage_mode::k_traits_global:
      return (sizeof(T) << 16_u32) | alignof(T);
    }
    return 0;
  }

  static rx_byte* reallocate_arguments(rx_byte* _existing, rx_size _size);

  // Stored in tag bits of |m_argument_store|.
  enum : rx_byte {
    k_enabled     = 1 << 0,
    k_initialized = 1 << 1,
    k_arguments   = 1 << 2
  };

  tagged_ptr<rx_byte> m_argument_store;

  intrusive_xor_list::node m_grouped;
  intrusive_xor_list::node m_ungrouped;

  const char* m_group;
  const char* m_name;

  rx_u32 (*m_storage_dispatch)(storage_mode _mode, rx_byte* _global_store,
    rx_byte* _argument_store);
};

// 32-bit: 24 + sizeof(T) bytes
// 64-bit: 48 + sizeof(T) bytes
template<typename T>
struct global {
  template<typename... Ts>
  global(const char* _group, const char* _name, Ts&&... _arguments);

  void init();
  void fini();

  template<typename... Ts>
  void init(Ts&&... _arguments);

  constexpr const char* name() const;

  constexpr T* operator&();
  constexpr const T* operator&() const;

  constexpr T& operator*();
  constexpr const T& operator*() const;

  constexpr T* operator->();
  constexpr const T* operator->() const;

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
  memory::uninitialized_storage<T>& _global_store, Ts&&... _arguments)
  : m_group{_group ? _group : "system"}
  , m_name{_name}
  , m_storage_dispatch{storage_dispatch<T, Ts...>}
{
  // The |_global_store| should be immediately after |this|.
  RX_ASSERT(reinterpret_cast<rx_uintptr>(&_global_store)
    == reinterpret_cast<rx_uintptr>(this + 1), "misalignment");

  if constexpr (sizeof...(Ts) != 0) {
    rx_byte* argument_store = reallocate_arguments(nullptr, sizeof(arguments<Ts...>));
    m_argument_store = {argument_store, k_enabled | k_arguments};
    construct_arguments(m_argument_store.as_ptr(), utility::forward<Ts>(_arguments)...);
  } else {
    m_argument_store = {nullptr, k_enabled};
  }

  globals::link(this);
}

template<typename... Ts>
inline void global_node::init(Ts&&... _arguments) {
  static_assert(sizeof...(Ts) != 0,
    "use void init() for default construction");

  auto argument_store = m_argument_store.as_ptr();
  if (m_argument_store.as_tag() & k_arguments) {
    m_storage_dispatch(storage_mode::k_fini_arguments, data(), argument_store);
  }

  construct_arguments(argument_store, utility::forward<Ts>(_arguments)...);

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
  validate_cast_for<T>();
  return reinterpret_cast<T*>(data());
}

template<typename T>
inline const T* global_node::cast() const {
  validate_cast_for<T>();
  return reinterpret_cast<const T*>(data());
}

template<typename T>
void global_node::validate_cast_for() const {
  const auto traits = m_storage_dispatch(storage_mode::k_traits_global, nullptr, nullptr);
  RX_ASSERT(sizeof(T) == ((traits >> 16) & 0xFFFF), "invalid size");
  RX_ASSERT(alignof(T) == (traits & 0xFFFF), "invalid allignment");
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
  : m_node{_group, _name, m_global_store, utility::forward<Ts>(_arguments)...}
{
}

template<typename T>
inline void global<T>::init() {
  m_node.init();
}

template<typename T>
inline void global<T>::fini() {
  m_node.fini();
}

template<typename T>
template<typename... Ts>
inline void global<T>::init(Ts&&... _arguments) {
  m_node.init(utility::forward<Ts>(_arguments)...);
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
inline constexpr T* global<T>::data() {
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* global<T>::data() const {
  return m_global_store.data();
}

} // namespace rx

#endif // RX_CORE_GLOBAL_H
