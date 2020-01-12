# Shaders

# Specification

## Basics

### Character Set and Phases of Compilation

The source character set used in RSL is Unicode in the UTF-8 encoding scheme. Only the following characters are allowed in the resulting stream of RSL tokens:
  * The letters `a-z`, `A-Z`, and the underscore (`_`).
  * The numbers `0-9`.
  * The symbols period (`.`), plus (`+`), dash (`-`), slash (`/`), asterisk (`*`), percent (`%`), angled brackets (`<` and `>`), square brackets (`[` and `]`), parentheses (`(` and `)`), caret (`^`), vertical bar (`|`), ampersand (`&`), tilde (`~`), equals (`=`), exclamation point (`!`), colon (`:`), semicolon (`;`), comma (`,`), and question mark (`?`).

A compile-time error will be given if any other character is used in a token.

There are no digraphs or trigraphs. There are no escape sequences or backslashes for use as line-continuation character.

Lines are relevant for diagnostics messages. They are terminated by carrige-return or line-feed. If both are used together, it will count only a single line termination. For the remainder of this document, any of these combinations is simply referred to as a new line.

In general, the language's use of this character set is case sensitive.

There are no character or string data types, so no quoting characters are included.

There is no end-of-file character.

### Comments

Comments are delimited by `/*` and `*/`, or by `//` and a new line. The begin comment delimiters (`/*` or `//`) are not recognized as comment delimiters inside of a comment, hence comments cannot be nested. A `/*` comment includes its terminating delimiter (`*/`). However, a `//` comment does not include (or eliminate) its terminated new line.

Inside comments, any byte values may be used, except a byte whose value is 0. No errors will be given for the content of comments and no validation on the content of the comments need be done.

### Tokens

The language is a sequence of RSL tokens. A token can be:

```
token:
  keyword
  type
  identifier
  integer-constant
  floating-constant
  operator
  ; {}
```

### Keywords

The following are the language's keywords and can only be used as described in this specification, or a compile-time error results.

```
break continue do for while switch case default
if else
discard return
```

### Identifiers

Identifiers are used for variable names, function names, and field selectors (field selectors select components of vectors and matrices.) Identifiers have the form

```
identifier:
  non-digit
  identifier non-digit
  identifier digit

non-digit: one of
  _ a b c d e f g h i j k l m n o p q r s t u v w x y z
    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z

digit: one of
  0 1 2 3 4 5 6 7 8
```

Identifiers starting with `rx_` are reserved for use by Rex and may not be declared in a shader; this results in a compile-time error.

### Definitions
Some language rules described below depend on the following definitions.

#### Static Use
A shader contains a _static use_ of (or _static assignment_ to) a variable `x`, if the shader contains a statement that would read (or write) `x`, whether or not run-time flow will of control will cause that statement to be executed.

#### Uniform and Non-Uniform Control Flow
When executing statements in a shader, control flow starts as _uniform control flow;_ all invocations (fragments, vertices, etc) enter the same control path into `main()`. Control flow becomes _non-uniform_ when different invocations take different paths through control-flow statements (selection, iteration, and jumps). Control flow subsequently returns to being uniform after such divergent sub-statements or skipped code completes, until the next time different control paths are taken.

Other examples of non-uniform control flow can occur within the switch statements and after conditional breaks, continues, early returns, and after fragment discards, when the condition is true for some invocations but not others. Loop ierations that only some invocations execute are also non-uniform control flow.

#### Dynamically Uniform Expressions
A shader expression is _dynamically uniform_ if all invocations evaluating it get the same resulting value. When loops are involved, this refers to the expression's value for the same loop iteration. When functions are involved, this refers to calls from the same call point.

Note that constant expressions are trivially dynamically uniform. It follows that typical loop counters based on these are dynamically uniform.

## Variables and Types
All variables and functions must be declared before being used. Variable and function names are identifiers.

There are not default typers. All variable and function declarations must have a declared type. A variable is declared by specifying its type followed by one or more names separated by commas. In many cases, a variable can be initialized as part of its declaration by using the assignment operator (`=`).

The Rex Shading Language is type safe. There are some implicit conversions between types. Exactly how and when this can occur is described later and as referenced by other sections in this specification.

### Basic Types

The Rex Shading Language supports the following basic data types, grouped as follows.

| type  | meaning                                                  |
| ----- | -------------------------------------------------------- |
| void  | for functions that do not return a value                 |
| bool  | a condition type, taking on values of `true` or `false`  |
| s32   | a signed 32-bit integer in two's complement              |
| u32   | an unsigned 32-bit integer                               |
| f32   | a single-precision floating-point scalar                 |
| f64   | a double-precision floating-point scalar                 |
| f32x2 | a two-component single-precision floating-point vector   |
| f32x3 | a three-component single-precision floating-point vector |
| f32x4 | a four-component single-precision floating-point vector  |
| s32x2 | a two-component signed integer vector                    |
| s32x3 | a three-component signed integer vector                  |
| s32x4 | a four-component signed integer vector                   |
| u32x2 | a two-component unsigned integer vector                  |
| u32x3 | a three-component unsigned integer vector                |
| u32x4 | a four-component unsigned integer vector                 |

#### Void

Functions that do not return a value must be declared `void`. There is no default function return type. The type `void` cannot be used in any other declarations, or a compile-time error results.

#### Booleans
To make condition execution of code easier to express, the type `bool` is supported. There is no expectation that hardware directly supports variables of this type. It is a genuine Boolean type, holding only one of two values meaning either true or false. Two keywords `true` and `false` can be used as literal Boolean constants. Booleans are declared and optionally initialized in the following example:

```c
bool success;      // declare "success" to be a Boolean
bool done = false; // declare and initialize "done"
```

The right hand side of the assignment operator (`=`) mut be an expression whose type is `bool`.

Expressions used for conditional jumps (`if`, `for`, `?:`, `while`, `do`-`while`) must evalue to the type `bool`.

#### Integers
Signed and unsigned integer variables are fully supported. In this document, the term _integer_ is meant to generally include both signed and unsigned integers.


#### Floating-Point Variables
Single-precision and double-precision floating point variables are available for use in a variety of scalar calculations. Generally, the term `floating-point` will refer to both single- and double-precision floating point. Floating-point variables are defined as in the following examples:

```c
f32 x, b = 1.5; // single-precision floating-point
f64 c, d = 2.0; // double-precision floating-point
```

As an input value to one of the processing units, a single-precision or double-precision floating-point variable is expected to match the corresponding IEEE 754 floating-point definition for precision and dynamic range. Floating-point variables within a shader are also encoded according to IEEE 754 specification for single-precision floating-point values (logically, not necessarily physically). While encodings are logically IEEE 754, operations (addition, multiplication, etc.) are not necessarily performed as required by IEEE 754.

#### Vectors
The Rex Shading Language includes data typrs for generic 2-, 3-, and 4-component vectors of floating-point values or integers. Floating-point vector variables can be used to store colors, normals, positions, texture coordinates, texture lookup results and the like.

#### Matrices
The Rex Shading Language has built-in types for 2x2, 2x3, 2x4, 3x2, 3x3, 3x4, 4x2, 4x3, and 4x4 matrices of floating-point numbers. The first number in the typename is the number of columns, the second is the number of rows.

Initialization of matrix values is done with constructors (described in "Constructors") in column-major order.

#### Opaque Types
