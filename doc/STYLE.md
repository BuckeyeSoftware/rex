# Coding style and rule guide

Note that most of this can be enforced with the `.clang-format` in the root of
the source tree.

Rex uses C++14.

## Hard no's
The following are a list of hard no's that are not tolerated in the code base
in any capacity and are provided here early.

* No exceptions.
* No multiple inheritence.
* No `dynamic_cast`.
* No `new`, `new[]`, `delete`, or `delete[]`.
* No multi-dimensional arrays.
* No standard library.

## Indentation
* Two spaces for indentation.
* No tabs.
* Do not indent nested namespaces.

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

Always put a space proceeding the closing paranthesis of a statement expecting
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
* Source files, headers and directories are named `snake_case`.

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

## Almost always unsigned
There's very few instance where signed integer arithmetic is what you want.
If the value can never be `< 0`, never use a signed integer. This is probably
the one area in Rex that most programmers will be unfamiliar with.

### Loop in reverse complaint
One common complaint about the use of use of unsigned is when a loop counter
needs to iterate in reverse and you want the 0th case to also be executed,
there's no way to write `i > 0` as the conditional. The solution here is to
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
