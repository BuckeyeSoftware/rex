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

#if !defined(__has_keyword)
#define __has_keyword(_x) !(__is_identifier(_x))
#endif

// Determine compiler.
#if defined(__clang__)
# define RX_COMPILER_CLANG
#elif defined(__GNUC__)
# define RX_COMPILER_GCC
#elif defined(_MSC_VER)
# define RX_COMPILER_MSVC
#else
# error "Unsupported compiler."
#endif

// Determine platform.
#if defined(_WIN32)
# define RX_PLATFORM_WINDOWS
#elif defined(__ANDROID__)
# define RX_PLATFORM_ANDROID
# define RX_PLATFORM_POSIX
#elif defined(__linux__)
# define RX_PLATFORM_LINUX
# define RX_PLATFORM_POSIX
#elif defined(__EMSCRIPTEN__)
# define RX_PLATFORM_EMSCRIPTEN
# define RX_PLATFORM_POSIX
#else
# error "Unsupported platform."
#endif

// Determine architecture.
#if defined(__x86_64__) || defined(_M_X64)
# define RX_ARCHITECTURE_AMD64
#elif defined(__i386__) || defined(_M_IX86)
# define RX_ARCHITECTURE_X86
#elif defined(__aarch64__) || defined(_M_ARM64)
# define RX_ARCHITECTURE_ARM64
#elif defined(__ppc64__) || defined(__PPC64__)
# define RX_ARCHITECTURE_PPC64
#elif defined(__EMSCRIPTEN__)
# define RX_ARCHITECTURE_WASM32
#else
# error "Unsupported architecture."
#endif

// Determine float rounding mode.
#if defined(__FLT_EVAL_METHOD__)
# define RX_FLOAT_EVAL_METHOD __FLT_EVAL_METHOD__
#elif defined(_M_IX86)
# if _M_IX86_FP >= 2
// float       => float
// double      => double
// long double => long double
#  define RX_FLOAT_EVAL_METHOD 0
# else
// float       => long double
// double      => long double
// long double => long double
#  define RX_FLOAT_EVAL_METHOD 2
# endif
#elif defined(_M_X64)
# define RX_FLOAT_EVAL_METHOD 0
#else
# error "Unsupported floating point."
#endif

// Determine endianess.
#if defined(RX_PLATFORM_WINDOWS)
# define RX_BYTE_ORDER_LITTLE_ENDIAN
#endif

#if defined(__LITTLE_ENDIAN__)
# if __LITTLE_ENDIAN__
#  define RX_BYTE_ORDER_LITTLE_ENDIAN
# endif
#elif defined(__BIG_ENDIAN__)
# if __BIG_ENDIAN__
#  define RX_BYTE_ORDER_BIG_ENDIAN
# endif
#elif defined(__BYTE_ORDER__)
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define RX_BYTE_ORDER_LITTLE_ENDIAN
# elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define RX_BYTE_ORDER_BIG_ENDIAN
# endif
#elif !defined(RX_BYTE_ORDER_LITTLE_ENDIAN) && !defined(RX_BYTE_ORDER_BIG_ENDIAN)
# error "Unsupported endianess."
#endif


// Determine visibility macros.
#if defined(RX_PLATFORM_WINDOWS)
#define RX_HIDDEN
#define RX_EXPORT __declspec(dllexport)
#define RX_IMPORT __declspec(dllimport)
#elif defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HIDDEN __attribute__((visibility("hidden")))
#define RX_EXPORT __attribute__((visibility("default")))
#define RX_IMPORT
#endif

#if !defined(RX_API)
#define RX_API
#endif

// disable some compiler warnings we don't care about
#if defined(RX_COMPILER_MSVC)
# pragma warning(disable: 4146) // unary minus operator applied to unsigned type, result still unsigned
# pragma warning(disable: 4522) // multiple assignment operators specified
#endif // defined(RX_COMPILER_MSVC)

#endif // RX_CORE_CONFIG_H
