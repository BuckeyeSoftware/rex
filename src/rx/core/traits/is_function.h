#ifndef RX_CORE_TRAITS_IS_FUNCTION_H
#define RX_CORE_TRAITS_IS_FUNCTION_H

namespace Rx::Traits {

template<typename>
inline constexpr const bool IS_FUNCTION = false;

// Specialization for regular functions.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...)> = true;

// Specialization for variadic functions.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...)> = true;

// Specialization for regular function types that have cv-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile> = true;

// Specialization for variadic function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile> = true;

// Specialization for regular function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile &&> = true;

// Specialization for variadic function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile &> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const &&> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile &&> = true;

// Specialization for noexcept regular functions.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) noexcept> = true;

// Specialization for noexcept variadic functions.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) noexcept> = true;

// Specialization for noexcept regular function types that have cv-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile noexcept> = true;

// Specialization for noexcept variadic function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile noexcept> = true;

// Specialization for noexcept regular function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) volatile && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts...) const volatile && noexcept> = true;

// Specialization for noexcept variadic function types that have ref-qualifiers.
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile & noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) volatile && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const && noexcept> = true;
template<typename R, typename... Ts>
inline constexpr const bool IS_FUNCTION<R(Ts..., ...) const volatile && noexcept> = true;

} // namespace Rx::Traits

#endif // RX_CORE_IS_FUNCTION_H