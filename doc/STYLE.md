# Coding style and rule guide

* [Hard no's](#hard-no-s)
  + [No exceptions](#no-exceptions)
  + [No multiple inheritence](#no-multiple-inheritence)
  + [No `dynamic_cast`](#no--dynamic-cast-)
  + [No `new`, `new[]`, `delete`, or `delete[]`](#no--new----new------delete---or--delete---)
  + [No multi-dimensional arrays.](#no-multi-dimensional-arrays)
  + [No standard library.](#no-standard-library)
  + [No `<math.h>`](#no---mathh--)
* [Indentation](#indentation)
  + [Two spaces for indentation](#two-spaces-for-indentation)
  + [No tabs](#no-tabs)
  + [Do not indent nested namespaces](#do-not-indent-nested-namespaces)
  + [80-column soft limit](#80-column-soft-limit)
* [Spacing](#spacing)
  + [Mem-initializer list](#mem-initializer-list)
* [Bracing](#bracing)
* [Casing](#casing)
* [Function parameters](#function-parameters)
* [Classes](#classes)
* [Const](#const)
  + [Placement](#placement)
* [Globals](#globals)
* [Thread local storage](#thread-local-storage)
* [Structure alignment and packing](#structure-alignment-and-packing)
* [Almost always unsigned](#almost-always-unsigned)
  + [Loop in reverse complaint](#loop-in-reverse-complaint)
* [Use explicitly sized types](#use-explicitly-sized-types)
* [Size](#size)
* [Organization](#organization)

Note that most of this can be enforced with the `.clang-format` in the root of
the source tree.

Rex uses C++17.

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
Exceptions complicate the flow of control. They introduce a ton of complexity
since you now have to write "exception-safe" code which is non-trivial. The use
of them encourages a style of programming where non-exceptional errors become
exceptions. They become an impedence mismatch for applications that have
non-local lifetime requirements (engines.)

### No multiple inheritence
Taking dependencies on multiple classes impedes compilation performance as full
class definitions need to be available in all translation units. Class inheritence
is about "is-a" relationships between type, it's non-trivial to describe "is-a"
relationships in a consistent manner for single-level inheritence, never mind
multiple-level inheritence. Similarly, "is-a" relationships are mental models
that map poorly and violate Data Oriented Design.

### No `dynamic_cast`
Polymorphism has it's uses but uses should be limited. When multiple-inheritence
isn't allowed, implementing your own type-check for casts between types with the
use of simple integers is not only far faster, it's far easier to reason about
when inspecting the state of objects in a debugger.

### No `new`, `new[]`, `delete`, or `delete[]`
Rex uses a fully polymorphic allocator interface where every container and
function that requires dynamic memory allocation requires as a dependency. This
allows backing everything with custom allocators with different lifetime
requirements without having to introduce separate types that are incompatible
with one another at interface boundaries. The use of traditional memory
management in C++ does not enable any of this behavior and is outright disabled
in Rex (calls `abort`). Memory management in Rex is part of the problem domain.

### No multi-dimensional arrays.
The use of multi-dimensional arrays shows a misunderstanding of memory layout.
Similarly, it's hard to discern if a column-major or row-major layout will be
more optimal at runtime, depending on architecture and kernel. In the end all
memory accessing is just 1D indexing and we shouldn't hide from that. It's
trivial to implement multi-dimensional arrays as wrapper types over single-
dimension arrays and we want to encourage that.

### No standard library.
The C++ standard library makes extensive use of exceptions which we disable,
under this behavior the standard library calls `std::terminate` which is not
very robust, there's no way to handle errors this way.

All standard library containers and functions make use of the global system
allocator which lets them sneak past all the hard work we put into making memory
part of the problem domain.

The standard library is inconsistent in implementation across platforms, there's
no way to reason about it as it's not a fixed implementation. There's no way to
optimize it, there's no way to extend it in meaningful ways.

It solves the wrong problems, introduces slow compilation times through massive
headers full of includes. It's not cache-efficient and often trades performance
for dubious conveniences, see `std::unordered_{set,map}` as prime examples of
this.

To work around these short-comings of the standard library, we have our own
foundation library of containers, types, and functions we call the "core"
library which can be read about [here](CORE.md)

### No `<math.h>`
Rex implements it's own math kernels and functions in software or with the help
of compiler intrinsics that are focused primarily around game development. In
particular they operate on the assumption that inputs are not pathological,
values will not contain `NaNs` and the rounding mode does not change from the
default which is assumed to be "round to nearest". This enables a much wider
array of optimization possibilities than `<math.h>` enables. At the same time,
Rex's math also strives to be bit-compatible across implementations enabling
a level of consistency that should make debugging and networking easier.

## Indentation
* Two spaces for indentation.
* No tabs.
* Do not indent nested namespaces.
* 80-column soft limit.

Here's some rationales for these rules.

### Two spaces for indentation
Two spaces for indentation is a compromise. C++ code tends to indent heavily
due to the use of namespaces and classes, in addition, lambdas and nesting of
lambdas in more "functional-style" code causes the code to move to the right,
yet we still want to maintain sensible line-lengths, as too wide of line is
harder to read and often is outside the viewport requiring horizontal scrolling
in tools to even inspect. This is pretty important when looking at diffs,
especially in a web-browser. Halving, or quarterning (depending on preference)
the whitespace mitigates these problems while still being easy to work with.
It's the style Google and LLVM uses too, if you're looking for some precedence.

### No tabs
Tabs for indentation would be ideal, but people often mix tabs and spaces when
trying to "align" code, which completely falls apart the moment the tab width
is different. Disallowing tabs means local alignment of code, which may be more
visually pleasing to some, doesn't need to become a mess for others.

### Do not indent nested namespaces
For the same reasons as two spaces for indentation, too much indentation moves
the code too far right.

### 80-column soft limit
It's a sensible line-length, even in the age of wide-screen displays. Lots of
tools still operate on this assumption, including the web-browser when viewing
diffs.

## Spacing
Always put a space between all statements and the opening parenthesis of that
statement.
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

**Do not** put spaces around parenthesis in a function call expression, or
before the paratnehsis in a function call expression.
```c
printf( "hello world\n" ); // invalid
printf ("hello world\n"); // invalid

printf("hello world\n");   // valid
```

**Do not** introduce unnecessary spacing in parenthesized expressions.
```c
( ( x + 1 ) * 2 ) / 0.5 // invalid
((x + 1) * 2) / 0.5     // valid
```

Always put a space proceeding the closing parenthesis of a statement expecting
a scope.
```c
if (x){  // invalid
if (x) { // valid
```

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
We use the one true brace style. It looks something like the following.
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

**Do not** omit the braces in a statement even if you only have one statement
proceeding it.
```c
if (x) foo(); // invalid
if (x)
  foo();      // invalid

// valid
if (x) {
  foo();
}
```

The one **and only** exception to this rule is when you have another statement
proceeding it which requires braces and it's the only one.
```c
// valid
if (x) for (Size i = 0; i < 10; i++) {
  <code>
}
```

## Casing
* All functions, including methods of classes are `snake_case`.
* All function locals, including arguments are `snake_case`.
* All types, including classes, enums, typedefs, and using are `PascalCase`.
* All constants, including enumerators are `TITLECASE`.
* All macros are `TITLECASE`.
* All namespaces are `PascalCase`.
* Source files, headers and directories are `snake_case`.

## Function parameters
* Function inputs should be preceeded with an underscore.
* Function outputs should be proceeded with an underscore.

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
* Non-public static class members should be prefixed with `s_`.
* **Do not** use multiple-inheritence.
* Avoid inheritence, if needed it should be public and one-level deep only.
* **Do not** implement method bodies inline the class. Put them _outside_.
 (This is faster for compilers to parse, in addition it's easier to read the class definition as a synopsis of the functionality it provides when it doesn't have inline code in it.)

## Const
* Try to use `const` as much as possible.
* Write `const` safe code.

### Placement
The placement of `const` is west-const, not east-const.
```c
int const x; // invalid
const int x; // valid
```

## Globals
All globals should be of type `Rx::Global<T>` and named with `g_` prefix. The
use of any other type for a global is not allowed. The `Rx::Global` wrapper
helps make handling of globals much easier, faster, and less error-prone.

In particular is solves all the usual problems with globals in C++, including:
  * Static initialization order fiasco.
  * Concurrent initialization.
  * Hot reloading.
  * Querying initialization state.
  * Defering initialization when there's dependencies.
  * Plugin reloading.

In addition, there exists the notion of a global "group" to help organize globals
into groups that can be initialized and deinitialized at once. So a system that
depends on some globals can be convienently organized.

## Thread local storage
The use of the `thread_local` keyword is supported. All compilers and toolchains
have support for it now.

## Structure alignment and packing
Everything in Rex has 16-byte alignment requirements. This simplifies the
handling of a lot of word-at-a-time algorithms and SIMD code. Avoids the need to
make special aligned allocations and copies for SIMD kernels and it's generally
the required alignment on x86_64 anyways. All allocators in Rex return memory
suitably aligned by 16-byte. The usual C/C++ structure packing rules apply
otherwise.

## Almost always unsigned
There's very few instance where signed integer arithmetic is what you want.
If the value can never be `< 0`, never use a signed integer. This is probably
the one area in Rex that most programmers will be unfamiliar with.

### Loop in reverse complaint
One common complaint about the use of use of unsigned is when a loop counter
needs to iterate in reverse and you want the 0th case to also be executed,
there's no way to write `i >= 0` as the conditional. The solution here is to
exploit the fact that unsigned integer underflow is **well defined**.

Try to write the code like this.
```c
for (Size i = size - 1; i < size; i--) {
  <code>
}
```
As you can see, starting from `size - 1`, we walk in reverse and when `i == 0`
we underflow and end up at the maximum value of `Size` which is `> size` and
so the loop stops. Not only is this **extremely fast** since you require no
clever per-iteration arithmetic to handle a subtraction case. It's a fast path
in the CPU where the loop branch becomes a test for underflow, and branch on
underflow has lesser latency than a branch on comparison.

When `size` is already unsigned, it may be tempting to cast the result of it to
a signed type like `Sint64` here for the loop to write the condition `i >= 0`,
this introduces a bug when `size >= 0x7fffffffffffffff`. Try to avoid this.

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

## Organization
Rex has a 1:1 organizational code structure where the filesystem layout of
source files, matches the namespace layout, which matches the class name layout.

For instance, a `Color` type for a particle `Particle` may exist in a file
named `color.h` and optional `color.cpp`, placed in a directory named `particle`.
The `Color` type would be in the `Particle` namespace.

This structure makes it easier to navigate code when you view your filesystem
as being your "solution" when you don't use an IDE. This makes it very easy to
use text editors that can just "open folder" as a project, like VSCode, JEdit,
CLion, Sublime, etc.
