#ifndef RX_CORE_REF_H
#define RX_CORE_REF_H

namespace rx {

template<typename T>
struct ref {
  constexpr ref(T& _ref);
  constexpr ref(T&&) = delete;

  constexpr operator T&() const;
  constexpr T& get() const;

private:
  T* m_data;
};

template<typename T>
inline constexpr ref<T>::ref(T& _ref)
  : m_data{&_ref}
{
}

template<typename T>
inline constexpr ref<T>::operator T&() const {
  return *m_data;
}

template<typename T>
inline constexpr T& ref<T>::get() const {
  return *m_data;
}

} // namespace rx

#endif // RX_CORE_REF_H
