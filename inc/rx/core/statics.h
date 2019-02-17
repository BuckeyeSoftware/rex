#ifndef RX_CORE_STATICS_H
#define RX_CORE_STATICS_H

#include <rx/core/concepts/no_copy.h> // no_copy
#include <rx/core/concepts/no_move.h> // no_move

#include <rx/core/debug.h> // RX_MESSAGE
#include <rx/core/memory/uninitialized_storage.h> // uninitialized_storage

namespace rx {

struct static_node
  : concepts::no_copy
  , concepts::no_move
{
  template<typename T>
  constexpr static_node(const char* name, memory::uninitialized_storage<T>& data);
  void init();
  void fini();
private:
  friend struct static_globals;
  void link();
  const char *m_name;
  static_node *m_next;
  static_node *m_prev;
  type_eraser m_data;
};

template<typename T>
inline constexpr static_node::static_node(const char* name, memory::uninitialized_storage<T>& data)
  : m_name{name}
  , m_next{nullptr}
  , m_prev{nullptr}
  , m_data{data.type_erase()}
{
  link();
}

inline void static_node::init() {
  RX_MESSAGE("init static global %s\n", m_name);
  m_data.init();
}

inline void static_node::fini() {
  RX_MESSAGE("fini static global %s\n", m_name);
  m_data.fini();
}

template<typename T>
struct static_global
  : concepts::no_copy
  , concepts::no_move
{
  constexpr static_global(const char* name);

  T& operator*();
  T* operator->();
  const T& operator*() const;
  const T* operator->() const;

private:
  static_node m_node;
  memory::uninitialized_storage<T> m_data;
};

template<typename T>
inline constexpr static_global<T>::static_global(const char* name)
  : m_node{name, m_data}
{
}

template<typename T>
T& static_global<T>::operator*() {
  return *m_data.data();
}

template<typename T>
T *static_global<T>::operator->() {
  return m_data.data();
}

template<typename T>
const T& static_global<T>::operator*() const {
  return *m_data.data();
}

template<typename T>
const T* static_global<T>::operator->() const {
  return m_data.data();
}

struct static_globals {
  static void init();
  static void fini();

  static static_node* find(const char* name);
  static void remove(static_node* node);
};

} // namespace rx

#endif // RX_CORE_STATICS_H
