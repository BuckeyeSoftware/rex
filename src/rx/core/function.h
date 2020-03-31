#ifndef RX_CORE_FUNCTION_H
#define RX_CORE_FUNCTION_H
#include "rx/core/ref.h"

#include "rx/core/memory/system_allocator.h"

#include "rx/core/traits/is_callable.h"
#include "rx/core/traits/enable_if.h"

#include "rx/core/utility/exchange.h"

namespace rx {

// 32-bit: 12 bytes
// 64-bit: 24 bytes
template<typename T>
struct function;

template<typename R, typename... Ts>
struct function<R(Ts...)> {
  constexpr function(memory::allocator& _allocator);
  constexpr function();

  template<typename F, typename = traits::enable_if<traits::is_callable<F, Ts...>>>
  function(memory::allocator& _allocator, F&& _function);

  template<typename F, typename = traits::enable_if<traits::is_callable<F, Ts...>>>
  function(F&& _function);

  function(const function& _function);
  function(function&& function_);

  function& operator=(const function& _function);
  function& operator=(function&& function_);
  function& operator=(rx_nullptr);

  ~function();

  R operator()(Ts... _arguments) const;

  operator bool() const;

  constexpr memory::allocator& allocator() const;

private:
  enum class lifetime {
    k_construct,
    k_destruct
  };

  using invoke_fn = R (*)(const rx_byte*, Ts&&...);
  using modify_lifetime_fn = void (*)(lifetime, rx_byte*, const rx_byte*);

  template<typename F>
  static R invoke(const rx_byte* _function, Ts&&... _arguments) {
    if constexpr(traits::is_same<R, void>) {
      (*reinterpret_cast<const F*>(_function))(utility::forward<Ts>(_arguments)...);
    } else {
      return (*reinterpret_cast<const F*>(_function))(utility::forward<Ts>(_arguments)...);
    }
  }

  template<typename F>
  static void modify_lifetime(lifetime _lifetime, rx_byte* _dst, const rx_byte* _src) {
    switch (_lifetime) {
    case lifetime::k_construct:
      utility::construct<F>(_dst, *reinterpret_cast<const F*>(_src));
      break;
    case lifetime::k_destruct:
      utility::destruct<F>(_dst);
      break;
    }
  }

  // Keep control block aligned so the function proceeding it in |m_data| is
  // always aligned by |k_alignment| too.
  struct alignas(memory::allocator::k_alignment) control_block {
    constexpr control_block(modify_lifetime_fn _modify_lifetime, invoke_fn _invoke)
      : modify_lifetime{_modify_lifetime}
      , invoke{_invoke}
    {
    }
    modify_lifetime_fn modify_lifetime;
    invoke_fn invoke;
  };

  void destroy();

  control_block* control();
  const control_block* control() const;

  rx_byte* storage();
  const rx_byte* storage() const;

  ref<memory::allocator> m_allocator;
  rx_byte* m_data;
  rx_size m_size;
};

template<typename R, typename... Ts>
inline constexpr function<R(Ts...)>::function(memory::allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{nullptr}
  , m_size{0}
{
}

template<typename R, typename... Ts>
inline constexpr function<R(Ts...)>::function()
  : function{memory::system_allocator::instance()}
{
}

template<typename R, typename... Ts>
template<typename F, typename>
inline function<R(Ts...)>::function(F&& _function)
  : function{memory::system_allocator::instance(), utility::forward<F>(_function)}
{
}

template<typename R, typename... Ts>
template<typename F, typename>
inline function<R(Ts...)>::function(memory::allocator& _allocator, F&& _function)
  : function{_allocator}
{
  m_size = sizeof(control_block) + sizeof _function;
  m_data = allocator().allocate(m_size);
  RX_ASSERT(m_data, "out of memory");

  utility::construct<control_block>(m_data, &modify_lifetime<F>, &invoke<F>);
  utility::construct<F>(storage(), utility::forward<F>(_function));
}

template<typename R, typename... Ts>
inline function<R(Ts...)>::function(const function& _function)
  : function{_function.allocator()}
{
  if (_function.m_data) {
    m_size = _function.m_size;
    m_data = allocator().allocate(m_size);
    RX_ASSERT(m_data, "out of memory");

    // Copy construct the control block and the function.
    utility::construct<control_block>(m_data, *_function.control());
    control()->modify_lifetime(lifetime::k_construct, storage(), _function.storage());
  }
}

template<typename R, typename... Ts>
inline function<R(Ts...)>::function(function&& function_)
  : m_allocator{function_.m_allocator}
  , m_data{utility::exchange(function_.m_data, nullptr)}
  , m_size{utility::exchange(function_.m_size, 0)}
{
}

template<typename R, typename... Ts>
inline function<R(Ts...)>& function<R(Ts...)>::operator=(const function& _function) {
  RX_ASSERT(&_function != this, "self assignment");

  if (m_data) {
    control()->modify_lifetime(lifetime::k_destruct, storage(), nullptr);
  }

  if (_function.m_data) {
    // Reallocate storage to make function fit.
    if (_function.m_size > m_size) {
      m_size = _function.m_size;
      m_data = allocator().reallocate(m_data, m_size);
      RX_ASSERT(m_data, "out of memory");
    }
    // Copy construct the control block and the function.
    utility::construct<control_block>(m_data, *_function.control());
    control()->modify_lifetime(lifetime::k_construct, storage(), _function.storage());
  } else {
    destroy();
  }

  return *this;
}

template<typename R, typename... Ts>
inline function<R(Ts...)>& function<R(Ts...)>::operator=(function&& function_) {
  RX_ASSERT(&function_ != this, "self assignment");

  if (m_data) {
    control()->modify_lifetime(lifetime::k_destruct, storage(), nullptr);
  }

  m_allocator = function_.m_allocator;
  m_size = utility::exchange(function_.m_size, 0);
  m_data = utility::exchange(function_.m_data, nullptr);

  return *this;
}

template<typename R, typename... Ts>
inline function<R(Ts...)>& function<R(Ts...)>::operator=(rx_nullptr) {
  if (m_data) {
    control()->modify_lifetime(lifetime::k_destruct, storage(), nullptr);
    destroy();
  }
  return *this;
}

template<typename R, typename... Ts>
inline function<R(Ts...)>::~function() {
  if (m_data) {
    control()->modify_lifetime(lifetime::k_destruct, storage(), nullptr);
    destroy();
  }
}

template<typename R, typename... Ts>
inline R function<R(Ts...)>::operator()(Ts... _arguments) const {
  if constexpr(traits::is_same<R, void>) {
    control()->invoke(storage(), utility::forward<Ts>(_arguments)...);
  } else {
    return control()->invoke(storage(), utility::forward<Ts>(_arguments)...);
  }
}

template<typename R, typename... Ts>
function<R(Ts...)>::operator bool() const {
  return m_size != 0;
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE constexpr memory::allocator& function<R(Ts...)>::allocator() const {
  return m_allocator;
}

template<typename R, typename... Ts>
inline void function<R(Ts...)>::destroy() {
  allocator().deallocate(m_data);
  m_data = nullptr;
  m_size = 0;
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE typename function<R(Ts...)>::control_block* function<R(Ts...)>::control() {
  return reinterpret_cast<control_block*>(m_data);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE const typename function<R(Ts...)>::control_block* function<R(Ts...)>::control() const {
  return reinterpret_cast<const control_block*>(m_data);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE rx_byte* function<R(Ts...)>::storage() {
  return m_data + sizeof(control_block);
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE const rx_byte* function<R(Ts...)>::storage() const {
  return m_data + sizeof(control_block);
}

} // namespace rx::core

#endif // RX_CORE_FUNCTION_H
