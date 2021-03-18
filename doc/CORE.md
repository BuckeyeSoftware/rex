# Core

Rex does not make use of the C++ standard library. Instead it implements its own core library in `src/rx/core`. with an emphasis on simplicity, performance and robustness.

## Differences
There's many differences between our core library and the standard. They're outlined here.

### No exceptions
We don't make use of exceptions. Nothing can throw, this leads to a far simpler and more managable set of interfaces that don't need to think of what happens when something throws. It also leads to much faster code since everything is implicitly `noexcept`.

### No iterators
It was discovered that the exception to the rule of iterating through all elements in a container is quite rare in most code and the amount of code to implement iterators is quite significant for something that is quite rare. This tradeoff concluded with the notion that it's a poor fit to implement at all. Instead, all containers implement an `each` method that can take a callback. Care was taken to make it safe to mutate while inside the callback too. The callback itself is implemented as a template function expecting a template argument invocable marked always-inline, so it's _always_ inlined, even in debug builds.

When you do need to manage an index into a container like a `Vector` or `String` you can just use `Size`. Since `Map` and `Set` are unordered, an iterator for the purposes of inserting or removing an element before or after another makes little sense as there's no guranteed order of the collection. The `find` interface of the unordered containers also gives you a pointer to the node
which can be used to store and refer to elements later, like an iterator. One other nice benefit is that, provided the containers do not resize, these pointers are safe from invalidation caused by mutation to `Map` and `Set`. Debug builds can detect invalidation under resize when mutating inside an `each`.

One downside to this is that most generic algorithms cannot be implemented for our types and range-based for loops are generally unusable without adapter code.

## Polymorphic allocators
Everything in the core library takes a pointer to a polymorphic allocator. There exists many allocator implementations for all sorts of purposes. This permits one to use many containers on the stack, to use fixed-size blocks of memory, track usage, leaks, etc.

One downside to this is most types end up consuming more memory than most standard library types because that pointer requires additional memory in the structure to store.

## No new or delete
Since everything should go through the polymorphic allocators, all forms of `new` and `delete` are disabled, including array variants. You cannot call them, they just forward to `abort`.

The `Array` container exists specifically to do "array constructions" through an allocator, as well as provide a less ambigious way to do initializer lists for things like `Vector` and `Map`.

## No runtime support
The C++ runtime support library is not used. This means features such as: `new`, `delete`, exceptions, and RTTI do not exist. Instead a very minimal, stublet implementation exists to call `Rx::abort` if it encounters such things being used.

Similarly, Rex does not support concurrent initialization of static globals, instead `Rx::Global` should be used. This interface provides a much stronger set of features for controlling global initialization too.

Pure virtual function calls just forward to `Rx::abort`.

## Performance
The core library was designed with performance in mind in all areas performance matters. Most library implementations in C++ do not concern themselves with good debug performance, only release. Similarly, compile-time performance is never considered.

Careful design choices were made to avoid excessive build times, such as avoiding large headers (this is why each trait is in it's own individual file in `core/traits`.) in addition to other things.

We make several large optimizations in the name of better runtime performance, too many to list here. However, some of the major ones include:

### Polymorphic allocators
Every container can be given a custom allocator. Small, on-stack containers with short life-times can be constructed by instantiating an allocator over some stack memory and giving it to the container in place of the system allocator. In fact, it's generally discouraged to use the system allocator, every instance of an object that needs to allocate memory, or has dependencies that require an allocator must be given am appropriate instance to an allocator that represents the system that object belongs to. This form of dependency injection is done explicitly.

### Robin-hood hash containers
One unfortunute consequence of C++'s hash containers: `std::unordered_{map,set}` is their interface must talk in `pair` which means keys and values are often stored together. This has awful cache characteristics for when you only need the keys or values. Similarly, all implementations use a bucketing strategy that leads to cache-inefficieny during traversal. We use a fully flat implementation of these containers with Robin-hood hashing, it's been finly tuned for cache efficiency. Keys and values are padded and packed to avoid ever stradding cache-lines without wasting too much memory.

### Disowning memory
Any container that manages a single, contiguous allocation can have it's memory stolen from it through a `disown` method, returning an untyped, owning view. The view contains a pointer to the allocator which owns the memory, a pointer to the memory, as well as the size of the allocation in bytes and the last owner's effective used capacity of that allocation in bytes. This view can be given to anything that can reasonably make sense of the contents. This allows copy-free transmuations like the following:

  * `String`       => `LinearBuffer`
  * `String`       => `Vector<T>`
  * `Vector<T>`    => `LinearBuffer`
  * `Vector<T>`    => `String`
  * `LinearBuffer` => `Vector<T>`
  * `LinearBuffer` => `String`
  * `RingBuffer`   => `Vector<T>`

Similarly, other types like `Map` and `Set` are implemented in terms of Robinhood hashing and maintain a single contiguous allocation for keys, values, and hash values. This means you can disown the keys of a `Set<T>` as an example into `Vector<T>`. There will be gaps of course since it is a hash set, but `Set<T>` and `Map<T>` have hash values that are zero in the same index for uninitialized slots.

### Small size optimization
Most standard libraries implement this for `std::string` and `std::function`. We do this as well, virtually everywhere. The `LinearBuffer` type contains an adjustable in-situ buffer which is the basis of most systems, enabling small `String`, `Vector`, `Map`, `Set`, and `Function` optimizations.

### Hints
We implement and utilize a wide variety of compiler hints to get proper code generation where the compiler cannot deduce things about branches, memory aliasing, alignment, etc.

### Exception free code
Avoiding the use of exceptions and outright disabling it means everything is `noexcept`.

### No implicit copies and always guranteed moves
There's no copy constructors or assignment operators for any types that are non-trivial. Instead all types that support copy operations do so through a static `copy` member function returning an `Optional` type. This is done for two purposes. The first purpose is to make it so surprsing, inefficient copies cannot happen. The second reason is because copies can fail. In C++, when a copy construction or assignment fails, the only way to handle errors is through the use of exception handling. In an exceptionless environment, such errors would lead to calls to `std::terminate` which would violate robustness requirements.

To make copies of data the use of global `copy(const T&)` function is used, returning `Optional<T>`.
To make moves of data the use of global `move(T&&)` function is used, returning `T&&`. This is the same as `std::move`, with the minor difference in that it _always moves_.

> This approach makes it impossible to have implicit copies.

## Algorithm

There's a few algorithm implementations:
  * `clamp`
  * `insertion_sort`
  * `max`
  * `min`
  * `quick_sort`
  * `topological_sort`

## Concepts
It's encouraged to prefer concepts to traits.

The following concepts exist:
  * `Assignable`
  * `Enum`
  * `FloatingPoint`
  * `Integral`
  * `Invocable`
  * `Referenceable`
  * `TriviallyCopyable`
  * `TriviallyDestructible`

## Concurrency

None of the concurrency primitives are safe out-of-process.

The following concurrency types are implemented:
  * `Atomic` Almost an exact implementation of `std::atomic<T>` except with better lock-free gurantees.
  * `ConditionVariable`. Standard condition variable.
  * `Mutex` A non-recursive mutex.
  * `RecursiveMutex` A recursive mutex.
  * `ScopeLock` A generic locked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `ScopeUnlock` A generic unlocked scope (works with any `T` that implements `lock` and `unlock` functions.)
  * `SpinLock` A non-recursive spin-lock with finely tuned backoff policy and yield.
  * `ThreadPool` A generic thread pool.
  * `Thread` A kernel thread.
  * `WaitGroup` Helper primitive to wait for a group of work to complete.

The following concurrency primitives are implemented:
  * `yield` Relinquish the thread to the OS.

## Filesystem

The following filesystem types are implemented:
  * `Directory` Open and manipulate a directory.
  * `File` Open and manipulate files.

The following filesystem functions are implemented:
  * `read_text_stream` Read a stream representing text, normalizing line endings and converting contents to UTF.
  * `read_binary_stream` Read a stream representing binary data.

## Hash

Hashing functions and utilities:

  * `combine` TEA algorithm for combining hashes.
  * `djbx33a` Interleave DJB hash four times for 128-bit hash.
  * `fnv1a` The fnv1a hash function.
  * `hasher` Type generic hash template that can be specialized.
  * `mix_enum` Mix the bits of an enumerator to use in hashes.
  * `mix_float` Mix the bits of a floating point value to use in hashes.
  * `mix_int` Mix the bits of an integer value to use in hashes.
  * `mix_pointer` Mix the representation of a pointer value to use in hashes.

## Hints

Many hints for the compiler:
  * `assume_aligned` Hint that a given pointer is aligned.
  * `empty_bases` Hint that base classes are empty to get EBO.
  * `force_inline` Used to force a function to inline.
  * `format` Used to indicate function takes format string-literal.
  * `likely` To mark a branch as likely happening for better scheduling.
  * `may_alias` Disable alias-analysis to avoid strict-aliasing optimizations from breaking code that requires undefined behavior.
  * `no_inline` Used to force a function to **not** inline. This is useful in large switch table dispatch into kernel functions from being inlined in the switch itself.
  * `restrict` Indicate that local variables to a function do not overlap in memory to avoid redundant spills and loads.
  * `thread` Thread sanitizer annotations.
  * `unlikely` To mark a branch as being unlikely for better instruction scheduling.
  * `unreachable` To mark an area of code as being unreachable.

## Math

Rex does not use the standard C's `<math.h>` library for it produces inconsistent result across platforms and it's performance can be beat in many cases where precision isn't a concern. Most of the functions are also implemented for integers which the standard library does not provide implementations for.

The following math kernel functions have been implemented for all types, including half-precision floating point:
  * `abs` Absolute.
  * `ceil` Ceiling.
  * `cos` Cosine.
  * `acos` Arccosine.
  * `floor` Floor.
  * `force_eval_f{32,64}` Force the evaluation of something even in presense of compiler optimizations.
  * `fract` Compute the fractional part.
  * `half` Half precision floating point type.
  * `isnan` Check if something is NaN in a portable way.
  * `ldexp` Generate value from significand and exponent.
  * `log2` Log base 2.
  * `mod` Modulo.
  * `pow` Power.
  * `round` Rounding of values.
  * `scalbnf` Multiplies a number by an integral power of float radix.
  * `shape` Shape the bits of floats and integers.
  * `sign` Extract the sign.
  * `sin` Sine.
  * `asin` Arcsine.
  * `sqrt` Square root.
  * `tan` Tangent.
  * `atan` Arctangent.
  * `atan2` Two-argument arctangent.

## Memory

Everything that allocates memory in the core library, including the containers, depend on a polymorphic allocator interface provided by `Allocator`. As a result, many types of allocators can be used anywhere.

The following allocator types exist:
  * `BuddyAllocator` Implementation of a Buddy allocator.
  * `BumpPointAllocator` Linear burn allocator.
  * `ElectricFenceAllocator` Debug allocator that traps over-writes and under-reads in an electric fence.
  * `HeapAllocator` The default heap allocator.
  * `SingleShotAllocator` One time use allocator.
  * `StatsAllocator` Wraps an existing allocator to provide statistics about it.
  * `SystemAllocato` The default system allocator.

Some additional, low-level memory types exist as well such as:
  * `Allocator` The allocator interface allocators must implement.
  * `Aggregate` Perform an aggregate allocation of different types in one allocation instead of multiple.
  * `UnintializedStorage` Storage with specific size and alignment that can be used in constexpr contexts while staying uninitialized.
  * `VMA` Virtual memory allocator interfaces.

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

Unlike the standard library, the core library in Rex tries to keep each type trait in it's own header file to accelerate build times. Not every trait is implemented, just the ones the core library uses and the `_t` and `_v` variants do not exist. Only the `_t` versions exist and they're implicit. So `RemoveReference<T>` is the `_t` version, likewise, `IS_ARRAY<T>` is the `_v` version.

The formatting is a bit different. Constants are always TITLECASE, while types are PascalCase. This is why it's `IS_ARRAY` and `RemoveReference<T>` in the above example.

There's also a few additional traits.

The following traits exist:
  * `add_pointer`
  * `conditional`
  * `decay`
  * `invoke_result`
  * `is_array`
  * `is_function`
  * `is_lvalue_reference`
  * `is_restrict`
  * `is_same`
  * `remove_const`
  * `remove_cv`
  * `remove_cvref`
  * `remove_extent`
  * `remove_reference`
  * `remove_volatile`
  * `underlying_type`

## Utility

A very small subset of the functionality found by `<utility>` exists here.

The following types exist:
  * `Pair` Similar to `std::pair`.

The following functions exist:
  * `bit` A bunch of bit manipulation functions and bitwise helpers.
  * `construct` A more powerful, placement `new` wrapper.
  * `copy` A safe way to copy types. Returns `Optional<T>` and dispatches `T::copy` if exists.
  * `declval` Exact implementation of `std::declval`.
  * `destruct` A more powerful way to call the destructor (address can be taken too).
  * `exchange` Exact implementation of `std::exchange`.
  * `forward` Exact implementation of `std::forward`.
  * `invoke` Similar to `std::invoke`.
  * `move` Exact implementation of `std::move`.
  * `pair` Similar to `std::pair`.
  * `swap` Exact implementation of `std::swap`.

## Containers

The following types exist:
  * `Array` Similar to `std::array`. 1D only.
  * `Bitset` A fixed-capacity bitset.
  * `DynamicPool` A dynamic-capacity pool.
  * `Function` A fast delegate that is similar to `std::function`, move-only.
  * `Global` Global variables are wrapped with this type.
  * `IntrusiveCompressedList` A space-optimized intrusive doubly-linked list.
  * `IntrusiveList` An intrusive doubly-linked list.
  * `LinearBuffer` A linear buffer with in-situ storage.
  * `Map` An unordered flat map using Robin-hood hashing.
  * `Optional` Optional type implementation.
  * `Ptr` A unique pointer implementation.
  * `Set` An unordered flat set using Robin-hood hashing.
  * `StaticPool` A fixed-capacity pool.
  * `StringTable` A UTF-8-safe string table with language conversions.
  * `String` A UTF-8-safe string and a UTF16 conversion interface for Windows with SSO.
  * `TaggedPtr` A tagged pointer for space saving optimizations.
  * `WideString` A UTF-16 safe string used to round-trip convert to `String`.
  * `Vector` A dynamic resizing array.

## Misc

The following types exist:
  * `Event` An event system with signal and slots. Slot adds a delegate, signal calls all delegates.
  * `JSON` A JSON & JSON5 reader and parser into a tree-like structure with thread-safe traversal.
  * `Profiler` A CPU and GPU profiler framework.
  * `SourceLocation` Type that describes file, line, and function where `RX_SOURCE_LOCATION` is used.
  * `Stream` Stream interface including stream conversion functions.

Other things not easily documented:
  * `abort` Take down the runtime safely while logging an abortion message.
  * `assert` Runtime assertions for `RX_DEBUG` builds. With optional messages.
  * `config` Feature test macros.
  * `format` Type safe formatting of types for printing.
  * `log` Generalized, thread-safe, concurrent logging framework.
  * `markers` Helper macros for marking types as no-copy, no-move, etc.
  * `pp` Preprocessor macros for standard token manipulation and preprocessing tricks.
  * `types` Types like `{U,S}int{8,16,32,64}`, and other common types.
