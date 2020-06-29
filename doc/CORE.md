# Core

Rex does not make use of the C++ standard library. Instead it implements it's own core library in `src/rx/core`. with an emphasis on simplicity and performance.

## Differences
There's many differences between our core library and the standard. They're outlined here.

### No exceptions
We don't make use of exceptions. Nothing can throw, this leads to a far simpler and more managable set of interfaces that don't need to think of what happens when something throws. It also leads to much faster code since everything is implicitly `noexcept`.

### No iterators
It was discovered that the exception to the rule of iterating through all elements in a container is quite rare in most code and the amount of code to implement iterators is quite significant for something that is quite rare. Making it a poor fit to implement at all. Instead, all containers implement an `each` method that can take a callback. Care was taken to make it safe to mutate while inside the callback too. The callback itself is implemented as a template argument so it's _always_ inlined.

When you do need to manage an index into a container like a `Vector` or `String` you can just use `Size`. Since `Map` and `Set` are unordered, an iterator for the purposes of inserting or removing an element before or after another makes little sense as there's no order. The `find` interface of the unordered containers also gives you a pointer to the node
which can be used to store and refer to elements later, like an iterator. One other nice benefit is that, provided the containers do not resize, these pointers are safe from invalidation caused by mutation to `Map` and `Set`.

One downside to this is that most generic algorithms cannot be implemented for our types.

## Polymorphic allocators
Everything in the core library takes a pointer to a polymorphic allocator. There exists many allocator implementations for all sorts of purposes. This permits one to use many containers on the stack, to use fixed-size blocks of memory, track usage, leaks, etc.

One downside to this is most types end up consuming more memory than most standard library types because that pointer requires additional memory in the structure to store.

## No new or delete
Since everything should go through the polymorphic allocators, all forms of `new` and `delete` are disabled, including array variants. You cannot call them, they just forward to `abort`.

The `Array` container exists specifically to do "array constructions" through an allocator, as well as provide a less ambigious way to do initializer lists for things like `Vector` and `Map`.

## No runtime support
The C++ runtime support library is not used. This means features such as: `new`, `delete`, exceptions, and RTTI does not exist. Instead a very minimal, stublet implementation exists to call `abort` if it encounters such things being used.

Similarly, Rex does not support concurrent initialization of static globals, instead `rx::Global` should be used. This interface provides a much stronger set of features for controlling global initialization too.

Pure virtual function calls just forward to `abort`.

## Performance
The core library was designed with performance in mind for all three places that performance matters:

  * Debug
  * Release
  * Build

The C++ standard library only focuses on release performance, leading to terrible debug and build performance.

Careful design choices were made to avoid excessive build times, such as avoiding large headers (this is why each trait is in it's own individual file in `core/traits`.) in addition to other things.

We make several large optimizations in the name of better runtime performance, too many to list here. However, some of the major ones include:

### Polymorphic allocators
Every container can be given a custom allocator. Small, on-stack containers with short life-times can be constructed by instantiating an allocator over some stack memory and giving it to the container in place of the system allocator.

### Robin-hood hash containers
One unfortunute consequence of C++'s hash containers, `unordered_{mapm,set}` is their interface must talk in `pair` which means they're often stored together. This has awful cache characteristics for when you only need the keys or values. Similarly, all implementations use a bucketing strategy that leads to cache-inefficieny. We use a fully flat implementation of these containers with Robin-hood hashing.

### Uninitialized vectors
There's no way with C++'s `vector` to initialize with size or resize the contents such that they stay uninitialized. This means something like `vector<char> data(1024)` will force all 1024 bytes to be zero initialized, only for you to replace them later.

We support both forms for trivially-constructible types through the use of the `UninitializedTag{}` tag value.

### Disowning memory
Any container that manages a single, contiguous allocation can have it's memory stolen from it through a `disown` method, returning an untyped, owning view. This view can be given to anything that can reasonably make sense of the contents. This allows copy-free transmuations like the following:

  * `Vector<char>` => `String`
  * `Vector<Byte>` => `String`
  * `String` => `Vector<char>`
  * `String` => `Vector<Byte>`

This is used extensively in file loading code.

### Small string optimization
Most standard libraries implement this, we do as well.

### Hints
We implement and utilize a wide variety of compiler hints to get proper code generation where the compiler cannopt deduce things about branches, memory aliasing, alignment, etc.

### Exception free code
Avoiding the use of exceptions and outright disabling it means everything is `noexcept`.

## Algorithm

There's a few algorithm implementations:
  * `clamp`
  * `insertion_sort`
  * `max`
  * `min`
  * `quick_sort`
  * `topological_sort`

## Concurrency

The following concurrency types are implemented:
  * `Atomic` Exact implementation of `std::atomic<T>`.
  * `ConditionVariable`.
  * `Mutex` A non-recursive mutex.
  * `ScopeLock` A generic locked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `ScopeUnlock` A generic unlocked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `SpinLock` A non-recursive spin-lock.
  * `ThreadPool` A generic thread pool.
  * `Thread` A kernel thread.
  * `WaitGroup` Helper primitive to wait for a group of work to complete.

The following concurrency primtiives are implements:
  * `yield` Relinquish the thread to the OS.

## Filesystem

The following filesystem types are implemented:
  * `Directory` Open and manipulate a directory.
  * `File` Open and manipulate files.

The following filesystem functions are implements:
  * `read_text_stream`
  * `read_binary_stream`

## Hints

Many hints for the compiler:
  * `assume_aligned` Hint that a given pointer is aligned.
  * `empty_bases` Hint that base classes are empty to get EBO.
  * `force_inline` Used to force a function to inline.
  * `likely` To mark a branch as likely happening for better scheduling.
  * `may_alias` Disable alias-analysis to aovid strict-aliasing optimizations from breaking code that requires undefined behavior.
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

The following allocator types exist:
  * `ElectricFenceAllocator`
  * `BumpPointAllocator`
  * `HeapAllocator`
  * `SingleShotAllocator`
  * `StatsAllocator`
  * `HeapAllocator`

Some additional, low-level memory types exist as well such as:
  * `UnintializedStorage`

## PRNG

There exists some implementations of random-number generators

The following PRNG types exist:
  * `MT19937` Mersenne Twister

## Serialize

A generic binary searializer for any `Stream`.

The following types exist:
  * `Encoder`
  * `Decoder`

## Time

Time library

The following types exist:
  * `Span` Represents a time span
  * `Stopwatch` Simple stop watch using QPC.

The following functions exist:
  * `delay` Delay calling thread
  * `qpc` Query performance counter

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
  * `is_restrict`
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
  * `underlying_type`

## Utility

A very small subset of the functionality found by `<utility>` exists here.

The following types exist:
  * `UninitializedTag` Key-hole type to use on containers you'd prefer stay uninitialized, like `vector`.
  * `Nat` Not-a-type, used to enable `constexpr` initialization of unions.
  * `Pair` Similar to `std::pair`.

The following functions exist:
  * `bit` A bunch of bit manipulation functions and bitwise helpers.
  * `construct` A more powerful, placement `new` wrapper.
  * `declval` Exact implementation of `std::declval`.
  * `destruct` A more powerful way to call the destructor.
  * `forward` Exact implementation of `std::forward`.
  * `move` Exact implementation of `std::move`.
  * `swap` Exact implementation of `std::swap`.

## Containers

The following types exist:
  * `Array` Similar to `std::array`. 1D only.
  * `Bitset` A fixed-capacity bitset.
  * `DynamicPool` A dynamic-capacity pool.
  * `StaticPool` A fixed-capacity pool.
  * `IntrusiveList` An intrusive doubly-linked list.
  * `IntrusiveCompressedList` A space-optimized intrusive doubly-linked list.
  * `Function` A fast delegate that is similar to `std::function`.
  * `DeferredFunction` A fast delegate that gets called when the function goes out of scope.
  * `Global` Global variables are wrapped with this type.
  * `Map` An unordered flat map using Robin-hood hashing.
  * `Set` An unordered flat set using Robin-hood hashing.
  * `Optional` Optional type implementation.
  * `String` A UTF-8-safe string and a UTF16 conversion interface for Windows.
  * `WideString` A UTF-16 safe string used to round-trip convert to `String`.
  * `StringTable` A UTF-8-safe string table.
  * `Vector` A dynamic resizing array.

## Misc

The following types exist:
  * `Event` An event system with signal and slots. Slot adds a delegate, signal calls all delegates.
  * `Profiler` A CPU and GPU profiler framework.
  * `Stream` Stream interface including stream conversion functions.
  * `JSON` A JSON5 reader and parser into a tree-like structure.

Other things not easily documented:
  * `abort` Take down the runtime safely while logging an abortion message.
  * `assert` Runtime assertions for `RX_DEBUG` builds. With optional messages.
  * `config` Feature test macros.
  * `format` Type safe formatting of types for printing.
  * `hash` Hash functions for various types and generalized hash combiner.
  * `log` Generalized, thread-safe, concurrent logging framework.
  * `types` Sized types like `{U,S}int{8,16,32,64}`
