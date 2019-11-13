# Core

Rex does not make use of the C++ standard library. Instead it implements a core library in `src/rx/core` which implements some similar interfaces but there's some significant differences made for the sake of simplicity and performance.

## Differences
There's many differences between our core library and the standard. They're outlines here.

### No exceptions
We don't make use of exceptions. Nothing can throw, this leads to a far simpler and more managable set of interfaces that don't need to think of what happens when something throws. It also leads to much faster code since everything is implicitly `noexcept`.

### No iterators
It was discovered that the exception to the rule of iterating through all elements in a container is quite rare in most code and the amount of code to implement iterators is quite significant for something that is quite rare. Making it a poor fit to implement at all. Instead, all containers implement an `each` method that can take a callback. Care was taken to make it safe to mutate while inside the callback too. The callback itself is implemented as a template argument so it's _always_ inlined.

When you do need to manage an index into a container like a `vector` or `string` you can just use `rx_size`. Since `map` and `set` are unordered, an iterator for the purposes of inserting or removing an element before or after another makes little since. The `find` interface of the unordered containers also gives you a pointer to the node
which can be used to store and refer to elements later, like an iterator. One other nice benefit is that, provided the containers do not resize, these pointers are safe from invalidation caused by mutation to `map` and `set`.

One downside to this is that most generic algorithms cannot be implemented for our types.

## Polymorphic allocators
Everything in the core library takes a pointer to a polymorphic allocator. There exists many allocator implementations for all sorts of purposes. This permits one to use many containers on the stack, to use fixed-size blocks of memory, track usage, leaks, etc.

One downside to this is most types end up consuming more memory than most standard library types because that pointer requires additional memory in the structure to store.

## No new or delete
Since everything should go through the polymorphic allocators, all forms of `new` and `delete` are disabled, including array variants. You cannot call them, they just forward to `abort`.

## No runtime support
The C++ runtime support library for things like `new` and `delete`, exceptions and RTTI is simply disabled from the builds. Instead a very minimal, stublet implementation exists to call `abort` if it encounters such things being used.

## Performance
Performance of runtime and debug builds as well as build performance were all major considerations for the core library. The C++ standard library often has poor runtime performance for debug builds and is known for it's slow build times. We don't have those problems as very careful design choices were made early on.

Cache efficient is also a major design choice which led to our `map` and `set` implementations to use Robinhood hashing. This isn't possible with the standard interface in `std::unordered_{map,set}` since the standard requires the key and value be stored together leading to poor cache utilization.

## Algorithm

There's a few algorithm implementations:
  * `clamp`
  * `insertion_sort`
  * `max`
  * `min`
  * `quick_sort`
  * `topological_sort`

## Concepts

Concepts are not to be confused with C++20's "concepts", instead they're inheritable classes which annotate a class.

The following concepts exist
  * `interface` Makes a class non-copyable and non-movable with a virtual destructor (i.e an "interface")
  * `no_copy` Makes a class non-copyable.
  * `no_move` Makes a class non-moveable.

## Concurrency

The following concurrency primitives are implemented:
  * `atomic` Exact implementation of `std::atomic<T>`.
  * `condition_variable`.
  * `future` Similar to `std::future<T>`
  * `mutex` A non-recursive mutex.
  * `scope_lock` A generic locked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `scope_unlock` A generic unlocked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `spin_lock` A non-recursive spin-lock.
  * `thread_pool` A generic thread pool.
  * `thread` A kernel thread.
  * `wait_group` Helper primitive to wait for a group of work to complete.
  * `yield` Relinquish the thread to the OS.

## Filesystem

The following filesystem primitives are implemented:
  * `directory` Open and manipulate a directory.
  * `file` Open and manipulate files.

## Hints

Many hints for the compiler:
  * `force_inline` Used to force a function to inline.
  * `likely` To mark a branch as likely happening for better scheduling.
  * `no_inline` Used to force a function to __not__ inline.
  * `restrict` Indicate that local variables to a function do not overlap in memory to avoid spills and loads.
  * `unlikely` To mark a branch as being unlikely for better scheduling.
  * `unreachable` To mark an area of code as being unreachable.

## Math

Rex does not use the standard C's `<math.h>` library for it produces inconsistent result across platforms and it's performance can be beat in many cases where precision isn't a concern. Most of the functions are also implemented for integers which the standard library does not provide implementations for.

The following math kernel functions have been implemented:
  * `abs` Absolute.
  * `ceil` Ceiling.
  * `cos` Cosine.
  * `floor` Floor.
  * `force_eval` Force the evaluation of something even in presense of compiler optimizations.
  * `half` Half precision floating point type.
  * `isnan` Check if something is NaN.
  * `log2` Log base 2.
  * `mod` Modulo.
  * `pow` Power.
  * `shape` Shape the bits of floats and integers.
  * `sign` Extract the sign.
  * `sin` Sine.
  * `sqrt` Square root.
  * `tan` Tangent.

## Memory

Everything that allocates memory in the core library, including the containers, depend on a polymorphic allocator interface provided by `allocator`. As a result, many types of allocators can be used virtually anywhere.

The following allocators exist:
  * `bump_point_allocator`
  * `heap_allocator`
  * `single_shot_allocator`
  * `stats_allocator`
  * `system_allocator`

Some additional, low-level memory manipulation and containers exist as well such as `uninitialized_storage`.

## PRNG

There exists some implementations of random-number generators:

  * `mt19937` Mersenne Twister

## Traits

Unlike the standard library, the core library in Rex tries to keep each type trait in it's own header file to accelerate build times. Not every trait is implemented, just the ones the core library uses and the `_t` and `_v` variants do not exist. Only the `_t` versions exist and they're implicit. So `remove_reference<T>` is the `_t` version, likewise, `is_array<T>` is the `_v` version.

There's also a few additional traits.

The following traits exist:
  * `add_const`
  * `add_cv`
  * `add_lvalue_reference`
  * `add_pointer`
  * `add_rvalue_reference`
  * `add_volatile`
  * `conditional`
  * `decay`
  * `enable_if`
  * `is_array`
  * `is_assignable`
  * `is_callable`
  * `is_floating_point`
  * `is_function`
  * `is_integral`
  * `is_lvalue_reference`
  * `is_pointer`
  * `is_reference`
  * `is_referenceable`
  * `is_rvalue_reference`
  * `is_same`
  * `is_trivially_copyable`
  * `is_trivially_destructible`
  * `is_void`
  * `remove_all_extents`
  * `remove_const`
  * `remove_cv`
  * `remove_cvref`
  * `remove_extent`
  * `remove_pointer`
  * `remove_reference`
  * `return_type`
  * `type_identity`

## Utility

A very small subset of the functionality found by `<utility>` exists here.

  * `bit` A bunch of bit manipulation functions and bitwise helpers.
  * `construct` A more powerful, placement `new` wrapper.
  * `declval` Exact implementation of `std::declval`.
  * `destruct` A more powerful way to call the destructor.
  * `forward` Exact implementation of `std::forward`.
  * `move` Exact implementation of `std::move`.
  * `nat` Not-a-type, used to enable `constexpr` initialization of unions.
  * `swap` Exact implementation of `std::swap`.

## Containers
  * `array` Similar to `std::array`.
  * `bitset` A fixed-capacity bitset.
  * `function` A fast delegate that is similar to `std::function`.
  * `deferred_function` A fast delegate that gets called when the function goes out of scope.
  * `global` Global variables are wrapped with this type.
  * `map` An unordered map using Robin-hood hashing.
  * `optional` Optional type implementation.
  * `pool` A fixed-capacity object pool.
  * `queue` Queue implementation.
  * `set` An unordered set using Robin-hood hashing.
  * `string` A UTF8-safe string and a UTF16 conversion interface for Windows.
  * `vector` A dynamic resizing array.

## Misc
  * `abort` Take down the runtime safely while logging an abortion message.
  * `assert` Runtime assertions for `RX_DEBUG` builds. With optional messages.
  * `event` An event system with signal and slots. Slot adds a delegate, signal calls all delegates.
  * `format` Type safe formatting of types for printing.
  * `hash` Hash functions for various types and generalized hash combiner.
  * `json` A JSON5 reader and parser into a tree-like structure.
  * `log` Generalized, thread-safe, concurrent logging framework.
  * `profiler` A CPU and GPU profiler framework.
  * `types` Sized types like `rx_{u,s}{8,16,32,64}`.
