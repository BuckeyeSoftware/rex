#ifndef RX_CORE_STATICS_H
#define RX_CORE_STATICS_H
#include "rx/core/concepts/no_copy.h" // no_copy
#include "rx/core/concepts/no_move.h" // no_move

#include "rx/core/memory/uninitialized_storage.h" // uninitialized_storage

#include "rx/core/debug.h" // RX_MESSAGE

namespace rx {

struct static_node
  : concepts::no_copy
  , concepts::no_move
{
  template<typename T, typename... Ts>
  constexpr static_node(const char* name, memory::uninitialized_storage<T>& data,
    Ts&&... args);

  void init();
  void fini();

  const char* name() const;

private:
  friend struct static_globals;

  void init_global();
  void fini_global();

  void link();
  bool m_enabled;
  const char *m_name;
  static_node *m_next;
  static_node *m_prev;
  type_eraser m_data;
};

template<typename T, typename... Ts>
inline constexpr static_node::static_node(const char* name,
  memory::uninitialized_storage<T>& data, Ts&&... args)
  : m_enabled{true}
  , m_name{name}
  , m_next{nullptr}
  , m_prev{nullptr}
  , m_data{data.type_erase(utility::forward<Ts>(args)...)}
{
  link();
}

inline void static_node::init_global() {
  if (m_enabled) {
    RX_MESSAGE("init %s", m_name);
    m_data.init();
  }
}

inline void static_node::fini_global() {
  if (m_enabled) {
    m_data.fini();
    RX_MESSAGE("fini %s", m_name);
  }
}

inline void static_node::init() {
  m_data.init();
  m_enabled = false;
}

inline void static_node::fini() {
  m_data.fini();
  m_enabled = false;
}

inline const char* static_node::name() const {
  return m_name;
}

template<typename T>
struct static_global
  : concepts::no_copy
  , concepts::no_move
{
  template<typename... Ts>
  constexpr static_global(const char* name, Ts&&... args);

  constexpr T& operator*();
  constexpr T* operator&();
  constexpr T* operator->();
  constexpr const T& operator*() const;
  constexpr const T* operator&() const;
  constexpr const T* operator->() const;

  template<typename... Ts>
  auto operator()(Ts&&... args);

private:
  static_node m_node;
  memory::uninitialized_storage<T> m_data;
};

template<typename T>
template<typename... Ts>
inline constexpr static_global<T>::static_global(const char* name, Ts&&... args)
  : m_node{name, m_data, utility::forward<Ts>(args)...}
{
}

template<typename T>
inline constexpr T& static_global<T>::operator*() {
  return *m_data.data();
}

template<typename T>
inline constexpr T* static_global<T>::operator&() {
  return m_data.data();
}

template<typename T>
inline constexpr T *static_global<T>::operator->() {
  return m_data.data();
}

template<typename T>
inline constexpr const T& static_global<T>::operator*() const {
  return *m_data.data();
}

template<typename T>
inline constexpr const T* static_global<T>::operator&() const {
  return m_data.data();
}

template<typename T>
inline constexpr const T* static_global<T>::operator->() const {
  return m_data.data();
}

template<typename T>
template<typename... Ts>
inline auto static_global<T>::operator()(Ts&&... _arguments) {
  return (*m_data.data())(utility::forward<Ts>(_arguments)...);
}

struct static_globals {
  static void init();
  static void fini();

  static static_node* find(const char* name);

  template<typename F>
  static bool each(F&& function);

  // linked-list manipulation
  static void lock();
  static void unlock();

private:
  static static_node* head();
  static static_node* tail();
};

template<typename F>
bool static_globals::each(F&& function) {
  lock();
  for (auto node{head()}; node; node = node->m_next) {
    unlock();
    if (!function(node)) {
      return false;
    }
    lock();
  }
  unlock();
  return true;
}

} // namespace rx

#define RX_GLOBAL \
  ::rx::static_global

#endif // RX_CORE_STATICS_H
