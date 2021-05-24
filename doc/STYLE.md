# Coding style and rule guide

* [Hard no's](#hard-nos)
  + [No exceptions](#no-exceptions)
  + [No multiple inheritence](#no-multiple-inheritence)
  + [No `dynamic_cast`](#no-dynamic_cast)
  + [No `new`, `new[]`, `delete`, or `delete[]`](#no-new-new-delete-or-delete)
  + [No multi-dimensional arrays.](#no-multi-dimensional-arrays)
  + [No standard library.](#no-standard-library)
  + [No `<math.h>`](#no-mathh)
* [Indentation](#indentation)
  + [Two spaces for indentation](#two-spaces-for-indentation)
  + [No tabs](#no-tabs)
  + [Do not indent nested namespaces](#do-not-indent-nested-namespaces)
  + [80-column soft limit](#80-column-soft-limit)
* [Spacing](#spacing)
  + [Statements](#statements)
  + [Mem-initializer list](#mem-initializer-list)
  + [Function call expressions](#function-call-expressions)
  + [Parenthesized expressions](#parenthesized-expressions)
  + [Scopes](#scopes)
  + [Sigils](#sigils)
* [Bracing](#bracing)
* [Casing](#casing)
* [Function parameters](#function-parameters)
* [Classes](#classes)
* [Const](#const)
  + [Placement](#placement)
* [Enums](#enums)
* [Globals](#globals)
* [Thread local storage](#thread-local-storage)
* [Structure alignment and packing](#structure-alignment-and-packing)
* [Almost always unsigned](#almost-always-unsigned)
  + [Loop in reverse complaint](#loop-in-reverse-complaint)
* [Use explicitly sized types](#use-explicitly-sized-types)
* [Size](#size)
* [Alternatives](#alternatives)

Note that most of this can be enforced with the `.clang-format` in the root of
the source tree.

Rex uses C++20.

## Hard no's
The following are a list of hard no's that are not tolerated in the code base
in any capacity and are provided here early.

* No exceptions.
* No multiple inheritence.
* No `dynamic_cast`.
* No `new`, `new[]`, `delete`, or `delete[]`. (They all call `abort` in Rex)
* No multi-dimensional arrays.
* No standard library.
* No `<math.h>`.

Here's the following rationale for each of the "Hard no's".

### No exceptions
Exceptions complicate the flow of control. They introduce complexity in requiring
non-trivial, "excpetion-safety". The use of exceptions encourages a style of
programming where non-exceptional errors become exceptions. They become an
impedence mismatch for applications that have non-local lifetime requirements.
These concerns makes it ill-suited for Rex.

### No multiple inheritence
Taking dependencies on multiple classes impedes compilation performance as full
class definitions need to be available in all translation units. Class inheritence
is about "is-a" relationships between type, it's non-trivial to describe "is-a"
relationships in a consistent manner for single-level inheritence, never mind
multiple-level inheritence. Similarly, "is-a" relationships are mental models
that map poorly and violate [Data Oriented Design](https://en.wikipedia.org/wiki/Data-oriented_design),
which is encouraged for system design.

### No `dynamic_cast`
Polymorphism has its uses but they should be limited. When multiple-inheritence
isn't allowed, implementing a type-check for casts between types with the
use of integers is not only far faster, it's far easier to reason about
when inspecting the state of objects in a debugger.

### No `new`, `new[]`, `delete`, or `delete[]`
Rex uses a fully polymorphic allocator interface where every container and
function that requires dynamic memory allocation, must be given as a dependency.
This allows backing everything with custom allocators with different lifetime
requirements without having to introduce separate types that are incompatible
with one another at interface boundaries. The use of traditional memory
management in C++ does not enable any of this behavior and is outright disabled
in Rex (calls `Rx::abort`). Memory management in Rex is part of the problem
domain.

### No multi-dimensional arrays.
The use of multi-dimensional arrays shows a misunderstanding of memory layout.
Similarly, it's hard to discern if a column-major or row-major layout will be
more optimal at runtime, depending on architecture and kernel. In actuality,
memory accessing is just 1D indexing and that shouldn't be hidden. It's trivial
to implement multi-dimensional arrays as wrapper types over single-dimension
arrays and that is encouraged.

### No standard library.
The standard library is ill-suited for Rex.

  * The C++ standard library makes extensive use of exceptions which are
    disabled in Rex. With exceptions disabled, the standard library's handling
    of errors result in a call to `std::terminate` which terminates the
    application. This behavior is not desired as errors cannot be handled and
    unexpected termination is not tolerated.
  * All standard library functionaliy that makes use of heap allocation does so
    through the use of the global system allocator. The added constraints of how
    Rex handles memory management makes this incompatible.
  * Implementations of the standard library are inconsistent. These
    inconsistencies can be challenging to overcome.
  * It often trades performance for convenience and performance is paramount to
    Rex.

To mitigate these short-comings, Rex employs its own foundation library as
a substitute for the standard library. Additional information is available
[here](CORE.md).

### No `<math.h>`
Rex implements its own math kernels and functions in software or with the help
of compiler intrinsics that are focus on our specific problem domain. In
particular they operate on the assumption that inputs are not pathological,
values will not contain `NaNs` and the rounding mode does not change from the
default ("round to nearest"). This enables a much wider array of optimization
possibilities than `<math.h>` permits. Additionally, Rex's math is designed
to be bit-identical across implementations enabling a level of consistency that
makes debugging and networking of calculations easier.

## Indentation
* Two spaces for indentation.
* No tabs.
* Do not indent nested namespaces.
* 80-column soft limit.

### Two spaces for indentation
Two spaces for indentation is a compromise. C++ code tends to indent heavily
due to the use of namespaces, classes, and functions. Futher, lambdas
and nesting of lambdas in more "functional-style" code causes the code to move
to the right.

Maintaining sensible line-lengths is more convieient for tooling and reading.
> It's the style Google and LLVM uses too, if you're looking for some precedence.

### No tabs
Tabs for indentation would be ideal, however mixing of tabs and spaces occurs
when trying to align code. This practice fails to maintain alignment under
variable tab-width. Spaces do not exhibit this problem.

### Do not indent nested namespaces
For the same reasons as two spaces for indentation, too much indentation moves
the code too far right.

### 80 column soft limit
This is a reasonable line-length as many tools operate on the assumption of 80
columns still.

## Spacing

### Statements
Always put a space between all statements and the opening parenthesis of that
statement:
```c
if(x)      // invalid
while(x)   // invalid
switch(x)  // invalid

if (x)     // valid
while (x)  // valid
switch (x) // valid
```

### Mem-initializer list
The mem-initializer list of a class should be formatted this way:
```c
struct Foo {
  Foo();
  float x, y, z;
};

Foo::Foo()
  : x{0}
  , y{0}
  , z{0}
{
}
```

> Take note of the `:` and `,` being aligned in a line for the constructor.

### Function call expressions
**Do not** put spaces around parenthesis in a function call expression, or
before the parenthesis in a function call expression.
```c
printf( "hello world\n" ); // invalid
printf ("hello world\n");  // invalid

printf("hello world\n");   // valid
```

### Parenthesized expressions
**Do not** introduce unnecessary spacing in parenthesized expressions.
```c
( ( x + 1 ) * 2 ) / 0.5 // invalid
((x + 1) * 2) / 0.5     // valid
```

### Scopes
Always put a space proceeding the closing parenthesis of a statement expecting
a scope.
```c
if (x){  // invalid
if (x) { // valid
```

### Sigils
The reference and pointer sigil should be grouped with the type rather than the
identifier. This also applies for rvalue references and parameter packs.
```c
int *x;        // invalid
int &x;        // invalid
int &&x;       // invalid
int &&...args; // invalid

int* x;        // valid
int& x;        // valid
int&& x;       // valid
int&&... args; // valid
```

## Bracing
Rex uses the one true brace style [1TBS](https://en.wikipedia.org/wiki/Indentation_style#Variant:_1TBS_(OTBS)). It looks something like the following.
```c
if (x) {
  <code>
} else {
  <code>
}

do {
  <code>
} while (expr);

for (int i = 0; i < 10; i++) {
  <code>
}

struct X {
  <code>
};

namespace Foo {
  <code>
}
```

**Do not** omit the braces in a statement even if only one statement proceeds it.
```c
if (x) foo(); // invalid
if (x)
  foo();      // invalid

// valid
if (x) {
  foo();
}
```

The one **and only** exception to this rule is if another statement proceeding
it requires its own braces.
```c
// valid
if (x) for (Size i = 0; i < 10; i++) {
  <code>
}
```

## Casing
* All functions, including methods of a class are `snake_case`.
* All function locals, including arguments are `snake_case`.
* All types, including classes, enums, typedefs, and using are `PascalCase`.
* All constants, including enumerators are `TITLECASE`.
* All macros are `TITLECASE`.
* All namespaces are `PascalCase`.
* Source files, headers, and directories are `snake_case`.

## Function parameters
* Function inputs should be **pre**ceeded with an underscore.
* Function outputs should be **pro**ceeded with an underscore.

A function input that is also an output should be treated as an output.
```c
void foo(int x);   // invalid
void foo(int* x);  // invalid

void foo(int _x);  // valid
void foo(int* x_); // valid
```

## Classes
* Prefer the `struct` keyword over `class`.
* Non-public class members should be prefixed with `m_`.
* Static class members should be prefixed with `s_`.
* **Do not** use multiple-inheritence.
* Avoid inheritence; if needed - inheritence should be public and can only be one-level deep.
* **Do not** implement method bodies inline the class, put them _outside_.
  This is faster for compilers to parse and it's easier to read the class definition as a synopsis of the functionality it provides when it doesn't have inline code in it.
* Class constructors **cannot fail**.
* Use `static Optional<T> T::copy(const T&)` for class copies that can fail.

## Const
* Try to use `const` as much as possible.
* Write `const` safe code.

### Placement
The placement of `const` is west-const, not east-const.
```c
int const x; // invalid
const int x; // valid
```

## Enums
All enums should be of the `enum class` variant, unless nested in a function or
class. All enums should have their numeric type explicitly specified.
```cpp
enum Foo { ... }; // invalid
enum class Foo { ... }; // invalid
enum class Foo : Uint8 { ... }; // valid
```

## Globals
All globals should be of type `Rx::Global<T>` and named with `g_` prefix. The
use of any other type for a global is not allowed. The `Rx::Global` wrapper
helps make handling of globals much easier, faster, and less error-prone.

In particular it solves all the usual problems with globals in C++.
  * Static initialization order fiasco.
  * Concurrent initialization.
  * Hot reloading.
  * Querying initialization state.
  * Deferring initialization due to dependencies.
  * Plugin reloading.

Convienently, the type `Rx::GlobalGroup` exists to organize globals into
collections that can be initialized and finalized as a group.

## Thread local storage
The use of the `thread_local` keyword is supported. All compilers and toolchains
have support for it now.

## Structure alignment and packing
Everything in Rex has 16-byte alignment requirements. This simplifies the
handling of word-at-a-time algorithms, SIMD code, and is generally the required
alignment on most architectures. All allocators in Rex return memory
suitably aligned by 16 bytes. The usual C/C++ structure packing rules still
apply.

## Almost always unsigned
The need for signed integer arithmetic is often misplaced as most integers
never represent negative values. The indexing of an array and iteration count
of a loop reflects this concept as well. There should be a propensity to use
unsigned integers more often than signed, but it can be an unfamiliar observation
in existing code that use signed integer arithmetic. The diagnostics that would
be emitted by compilers are silenced when all integers are signed, or suppressed
by potentially unsafe type casts.

### Loop in reverse complaint
The most common complaint about the use of unsigned is when a loop counter needs
to count in reverse and the body of the loop needs to execute when the counter
is zero. The condition `i >= 0` cannot be expressed as it's always true. It may
be tempting to write a for loop like the following.
```c
for (Sint64 i = Sint64(size) - 1; i >= 0; i--) {
  <code>
}
```

Putting aside the argument that a `size >= 0x7fffffffffffffff` would be
pathological, this introduces a silent bug that leads to a hard crash when given
a pathological value. The only acceptable solution in Rex is to recognize that
unsigned integer underflow is **well defined** and make use of it.
```c
for (Size i = size - 1; i < size; i--) {
  <code>
}
```
The loop counter begins from `size - 1` and counts down on each iteration. When
the counter reaches zero, the decrement causes the counter to underflow and wrap
around to the max possible value of `Size` type. This value is far larger than
`size` so the condition `i < size` evaluates false, and the loop stops.

With this approach, the hard crash on the pathological input is avoided since it
permits every possible size value from [0, 0xFFFFFFFFFFFFFFFF). No casts are
needed. Another common work around is to offset the index by one by introducing
per-iteration arithmetic, this avoids the need for such work arounds too.

## Use explicitly sized types
* Use `Uint8`, `Uint16`, `Uint32`, `Uint64`, `Sint8`, `Sint16`, `Sint32`,
  and `Sint64` over non-sized integer types.
* Use `Float16`, `Float32`, and `Float64` over non-sized floating-point type.

## Size
The `Size` type is an unsigned integer type of the result of the `sizeof`,
`sizeof...`, and `alignof` operator. It's an alias for `size_t`.

The `Size` type can store the maximum size of a theoretically possible object
of any type (including array). A type whose size cannot be represented by
`Size` isn't possible.

The `Size` type should **always** be used for array indexing and loop counting,
since using any other type, such as `Uint32`, for array indexing will fail where
the index exceeds `UINT32_MAX`. Similarly, the use of other types may require
expensive modular arithmetic on some platforms.

**NEVER** use `Size` to represent the size or offsets inside files, use `Uint64`.

## Alternatives
There are alternative functions that replace common libc functions that are
reccomended are used instead. A simple replacement table is provided below.

| C header     | C function or pattern | Rex header             | Rex function     |
|--------------|-----------------------|------------------------|------------------|
| `<string.h>` | `memchr`              | `"rx/memory/search.h"` | `Memory::search` |
| `<string.h>` | `memmem`              | `"rx/memory/search.h"` | `Memory::search` |
| `<string.h>` | `memcpy`              | `"rx/memory/memcpy.h"` | `Memory::copy`   |
| `<string.h>` | `memset`              | `"rx/memory/fill.h"`   | `Memory::fill`   |
| `<string.h>` | `memset(dst, 0, n)`   | `"rx/memory/zero.h"`   | `Memory::zero`   |
| `<string.h>` | `memmove`             | `"rx/memory/move.h"`   | `Memory::move`   |