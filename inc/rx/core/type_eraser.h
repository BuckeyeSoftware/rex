#ifndef RX_CORE_TYPE_ERASER_H
#define RX_CORE_TYPE_ERASER_H

#include <rx/core/traits.h> // call_{ctor, dtor}
#include <rx/core/concepts/no_copy.h> // no_copy

namespace rx {

// type erasing variant
struct type_eraser : concepts::no_copy {
  // initialize type eraser instance with |data|
  template<typename T>
  constexpr type_eraser(void *data, identity<T>);

  // move a type erased instance
  constexpr type_eraser(type_eraser&& eraser);

  // call the constructor on the type-erased object
  void init();

  // call the destructor on the type-erased object
  void fini();

private:
  void* m_data;

  void (*m_ctor)(void*);
  void (*m_dtor)(void*);
};

template<typename T>
inline constexpr type_eraser::type_eraser(void *data, identity<T>)
  : m_data{data}
  , m_ctor{call_ctor<T>}
  , m_dtor{call_dtor<T>}
{
}

inline constexpr type_eraser::type_eraser(type_eraser&& eraser)
  : m_data{eraser.m_data}
  , m_ctor{eraser.m_ctor}
  , m_dtor{eraser.m_dtor}
{
  eraser.m_data = nullptr;
  eraser.m_ctor = nullptr;
  eraser.m_dtor = nullptr;
}

inline void type_eraser::init() {
  if (m_data) {
    m_ctor(m_data);
  }
}

inline void type_eraser::fini() {
  if (m_data) {
    m_dtor(m_data);
  }
}

} // namespace rx

#endif // RX_CORE_TYPE_ERASER_H
