#ifndef RX_CORE_FUNCTION_H
#define RX_CORE_FUNCTION_H

#include <rx/core/array.h>

namespace rx {

template<typename T>
struct function;

template<typename R, typename... Ts>
struct function<R(Ts...)> {
  function()
    : m_invoke{nullptr}
    , m_construct{nullptr}
    , m_destruct{nullptr}
  {
  }

  template<typename F>
  function(F fn)
    : m_invoke{invoke<F>}
    , m_construct{construct<F>}
    , m_destruct{destruct<F>}
  {
    m_data.resize(sizeof fn);
    m_construct(m_data.data(), reinterpret_cast<rx_byte*>(&fn));
  }

  function(const function& fn)
    : m_invoke{fn.m_invoke}
    , m_construct{fn.m_construct}
    , m_destruct{fn.m_destruct}
  {
    if (m_invoke) {
      m_data.resize(fn.m_data.size());
      m_construct(m_data.data(), fn.m_data.data());
    }
  }

  function& operator=(const function& fn) {
    if (m_destruct) {
      m_destruct(m_data.data());
    }

    m_invoke = fn.m_invoke;
    m_construct = fn.m_construct;
    m_destruct = fn.m_destruct;

    if (m_invoke) {
      m_data.resize(fn.m_data.size());
      m_construct(m_data.data(), fn.m_data.data());
    }

    return *this;
  }

  ~function() {
    if (m_destruct) {
      m_destruct(m_data.data());
    }
  }

  R operator()(Ts&&... args) {
    if constexpr(traits::is_same<R, void>) {
      return m_invoke(m_data.data(), utility::forward<Ts>(args)...);
    } else {
      m_invoke(m_data.data(), utility::forward<Ts>(args)...);
    }
  }

private:
  using invoke_fn = R (*)(rx_byte*, Ts&&...);
  using construct_fn = void (*)(rx_byte*, const rx_byte*);
  using destruct_fn = void (*)(rx_byte*);

  template<typename F>
  static R invoke(rx_byte* fn, Ts&&... args) {
    if constexpr(traits::is_same<R, void>) {
      (*reinterpret_cast<F*>(fn))(utility::forward<Ts>(args)...);
    } else {
      return (*reinterpret_cast<F*>(fn))(utility::forward<Ts>(args)...);
    }
  }

  template<typename F>
  static void construct(rx_byte* dst, const rx_byte* src) {
    utility::construct<F>(dst, *reinterpret_cast<const F*>(src));
  }

  template<typename F>
  static void destruct(rx_byte *fn) {
    utility::destruct<F>(fn);
  }

  invoke_fn m_invoke;
  construct_fn m_construct;
  destruct_fn m_destruct;
  array<rx_byte> m_data;
};


} // namespace rx::core

#endif // RX_CORE_FUNCTION_H
