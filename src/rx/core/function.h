#ifndef RX_CORE_FUNCTION_HDR
#define RX_CORE_FUNCTION_HDR
#include "rx/core/linear_buffer.h"
#include "rx/core/concepts/invocable.h"
#include "rx/core/utility/copy.h"
#include "rx/core/traits/decay.h"

namespace Rx {

template<typename T>
struct Function;

template<typename R, typename... Ts>
struct Function<R(Ts...)> {
  RX_MARK_NO_COPY(Function);

  template<typename F>
    requires Concepts::Invocable<F, Ts...>
  static Optional<Function<R(Ts...)>> create(F&& _function);

  constexpr Function();
  constexpr Function(NullPointer);
  Function(Function&& function_);
  ~Function();

  Function& operator=(Function&& function_);
  Function& operator=(NullPointer);

  R operator()(Ts...) const;

  bool is_valid() const;

  operator bool() const;

private:
  enum class Operation {
    DESTRUCT,
    MOVE,
  };

  static void move(Function* dst_, Function* src_) {
    // Moving is tricky because if |function_| is in-situ, then any captured
    // data that holds references to |this| will be invalidated with a regular
    // move, as one cannot move in-situ data, it must be copied. Similarly we
    // cannot just memcpy the contents, as the data may be non-trivial.
    //
    // The one time a move can be done is when |function_| is **not** in-situ,
    // as the move just becomes a pointer exchange which won't invalidate
    // anything.
    if (src_->m_storage.in_situ()) {
      // Cannot fail: if |function_| fits in-situ, it'll fit in-situ here too.
      (void)dst_->m_storage.resize(src_->m_storage.size());

      // Copy construct the control block, then move construct the function.
      auto* control = dst_->control();
      Utility::construct<Control>(control, *src_->control());
      control->modify(Operation::MOVE, dst_->function(), src_->function());

      // Reset the movee to initial in-situ state.
      src_->m_storage.clear();
    } else {
      dst_->m_storage = Utility::move(src_->m_storage);
    }
  }

  void release() {
    if (is_valid()) {
      control()->modify(Operation::DESTRUCT, function(), nullptr);
    }
  }

  // Keep the function storage 16 byte aligned.
  struct alignas(16) Control {
    R (*invoke)(const Byte* _function, Ts&&... _args);
    void (*modify)(Operation _operation, Byte* dst_, Byte* _src);
  };

  template<typename F>
  static R invoke(const Byte* _function, Ts&&... _arguments) {
    auto& invoke = *reinterpret_cast<const F*>(_function);
    return invoke(Utility::forward<Ts>(_arguments)...);
  }

  // Pack multiple lifetime modifications into a single function and dispatch
  // based on the |_operation| passed. This is done to store a single function
  // pointer rather than two, saving space in the in-situ storage of the
  // function.
  template<typename F>
  static void modify(Operation _operation, Byte* dst_, Byte* _src) {
    switch (_operation) {
    case Operation::DESTRUCT:
      Utility::destruct<F>(dst_);
      break;
    case Operation::MOVE:
      Utility::construct<F>(dst_, Utility::move(*reinterpret_cast<F*>(_src)));
      break;
    }
  }

  Control* control() { return reinterpret_cast<Control*>(m_storage.data()); }
  const Control* control() const { return reinterpret_cast<const Control*>(m_storage.data()); }

  Byte* function() { return reinterpret_cast<Byte*>(control() + 1); }
  const Byte* function() const { return reinterpret_cast<const Byte*>(control() + 1); }

  // Like sizeof but includes the size of the control block used to store the
  // type-erased dispatch functions.
  template<typename F>
  static inline constexpr const Size SIZE_OF = sizeof(Control) + sizeof(F);

  static_assert(alignof(Control) == 16,
    "Control block has invalid alignment");
  static_assert(Concepts::TriviallyCopyable<Control>,
    "Control block is not trivially copyable");

  LinearBuffer m_storage;
};

template<typename R, typename... Ts>
template<typename F>
  requires Concepts::Invocable<F, Ts...>
Optional<Function<R(Ts...)>> Function<R(Ts...)>::create(F&& _function) {
  // Decay the incoming F to allow creating from all the invocable types:
  // * regular functions
  // * function pointers
  // * function references
  // * static member functions
  // * functors
  // * lambdas
  using T = Traits::Decay<F>;

  Function<R(Ts...)> result;
  if (!result.m_storage.resize(SIZE_OF<T>)) {
    return nullopt;
  }

  Utility::construct<Control>(result.control(), Control{&invoke<F>, &modify<F>});
  Utility::construct<T>(result.function(), Utility::forward<F>(_function));

  return result;
}

template<typename R, typename... Ts>
constexpr Function<R(Ts...)>::Function() = default;

template<typename R, typename... Ts>
constexpr Function<R(Ts...)>::Function(NullPointer) {
  // Nothing to do.
}

template<typename R, typename... Ts>
Function<R(Ts...)>::Function(Function&& function_) {
  move(this, &function_);
}

template<typename R, typename... Ts>
Function<R(Ts...)>::~Function() {
  release();
}

template<typename R, typename... Ts>
Function<R(Ts...)>& Function<R(Ts...)>::operator=(Function&& function_) {
  if (&function_ != this) {
    release();
    move(this, &function_);
  }
  return *this;
}

template<typename R, typename... Ts>
Function<R(Ts...)>& Function<R(Ts...)>::operator=(NullPointer) {
  release();
  m_storage.clear();
  return *this;
}

template<typename R, typename... Ts>
R Function<R(Ts...)>::operator()(Ts... _args) const {
  RX_ASSERT(is_valid(), "null function");
  return control()->invoke(function(), Utility::forward<Ts>(_args)...);
}

template<typename R, typename... Ts>
bool Function<R(Ts...)>::is_valid() const {
  return m_storage.size() != 0;
}

template<typename R, typename... Ts>
Function<R(Ts...)>::operator bool() const {
  return is_valid();
}

} // namespace Rx

#endif // RX_CORE_FUNCTION2_HDR