#ifndef RX_CORE_CONFIG_H
#define RX_CORE_CONFIG_H

// implement missing compiler feature test macros
#if !defined(__is_identifier)
#define __is_identifier(_x) 1
#endif

#if !defined(__has_attribute)
#define __has_attribute(_x) 0
#endif

#if !defined(__has_builtin)
#define __has_builtin(_x) 0
#endif

#if !defined(__has_extension)
#define __has_extension(_x) 0
#endif

#if !defined(__has_feature)
#define __has_feature(_x) 0
#endif

#if !defined(__has_include)
#define __has_include(...) 0
#endif

#define __has_keyword(_x) !(__is_identifier(_x))

// determine compiler
#if defined(__clang__)
# define RX_COMPILER_CLANG
#elif defined(__GNUC__)
# define RX_COMPILER_GCC
#else
# error "unsupported compiler"
#endif

// determine platform
#if defined(_WIN32)
# define RX_PLATFORM_WINDOWS
#elif defined(__linux__)
# define RX_PLATFORM_LINUX
# define RX_PLATFORM_POSIX
#else
# error "unsupported platform"
#endif

// determine endianess
#if defined(__LITTLE_ENDIAN__)
# if __LITTLE_ENDIAN__
#   define RX_BYTE_ORDER_LITTLE_ENDIAN
# endif // __LITTLE_ENDIAN__
#endif // __LITTLE_ENDIAN__

#if defined(__BIG_ENDIAN__)
# if __BIG_ENDIAN__
#   define RX_BYTE_ORDER_BIG_ENDIAN
# endif // __BIG_ENDIAN__
#endif // __BIT_ENDIAN__

#if defined(__BYTE_ORDER__)
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#   define RX_BYTE_ORDER_LITTLE_ENDIAN
# elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#   define RX_BYTE_ORDER_BIG_ENDIAN
# endif // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#endif // __BYTE_ORDER__

#if !defined(RX_BYTE_ORDER_LITTLE_ENDIAN) && !defined(RX_BYTE_ORDER_BIG_ENDIAN)
# include <endian.h>
# if __BYTE_ORDER__ == __LITTLE_ENDIAN
#   define RX_BYTE_ORDER_LITTLE_ENDIAN
# elif __BYTE_ORDER__ == __BIG_ENDIAN
#   define RX_BYTE_ORDER_BIG_ENDIAN
# else // __BYTE_ORDER__ == __BIG_ENDIAN
#   error "unable to determine endian"
# endif
#endif // !defined(RX_BYTE_ORDER_LITTLE_ENDIAN) && !defined(RX_BYTE_ORDER_BIG_ENDIAN)

#endif // RX_CORE_CONFIG_H