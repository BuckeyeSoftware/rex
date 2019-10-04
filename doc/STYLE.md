# Style guide
* 2-space indentaiton
* function in parameters are prefixed with `_`, also includes constructors
* function out parameters are suffixed with `_`, rvalue references are often out params too
* private or protected class fields are prefixed with `m_`
* constants are prefixed with `k_`
* globals are prefixed with `g_`
* no `get` or `set` prefix on class methods
* `//` comments only
* macro identifiers are capitalized
* macro arguments are lower-case
* starting curly brace `{` always on the same line
* ending curly brace `}` on its' own line
* space after `if`, `for`, `while` and `switch`, e.g `if (x)` and not `if(x)`
* uniform-initialization for everything, e.g `int foo{0};` and not `int foo = 0;`
* classes should inherit from `concepts::no_move` if they're not movable,
  `concepts::no_copy` if they're not copyable and `concepts::interface` if
  they're an interface
* iterate through collections with `.each` methods and lambdas
* for loop counters should use `rx_size` and not `int` unless the value is an `int`
* namespaces are not indented, nested ones are, comment the end of namespaces
* use namespace name `detail` for hidden details
* header guards are `#ifdef RX_{SCOPE}_FILENAME_H`, comment the end of header guards
* use ref-qualifiers on const methods, e.g `const &` and not `const`
* prefer `emplace` to `push` on collections
* prefer `utility::move(value)` to copies
* prefer `utility::forward<Ts>(value...)` to copies
* don't use `new` or `delete`, use `array<rx_byte>` for raw memory or
  appropriate allocator interfaces, construct objects with `utility::construct<T>(...)`
  and destruct them with `utility::destruct<T>(...)`, allocate and construct
  with `allocator::create`, destruct and deallocate with `allocator::destroy`.
* no exceptions, no runtime-type information (no `dynamic_cast` or `typeid`)
* no multiple inheritence, `concepts::*` classes don't count
* string format with `string::format`, no `va_list`, use `Ts...` instead
* no naked static globals, use `RX_GLOBAL<T> g_name{...}` instead, globals are
  grouped in _global groups_ use `RX_GLOBAL_GROUP` to define them.
* avoid naked `lock` and `unlock` with syncronization primitives, use `scope_lock`
  and `scope_unlock` instead
* avoid plain C arrays, use `array<T[E]>`
* avoid multi-dimensional arrays, static or not (emulate with 1d indexing patterns)
* no STL or C++ standard library headers, this includes the C ones like `<cstdio>`
* no `<math.h>` use `rx/core/math` instead
* no `volatile` or `register`
* use `restrict` if pointers don't overlap
* make everything as `const` as possible, excluding function parameters
* use `[[fallthrough]]` in switch fallthroughs
* use `RX_HINT_UNREACHABLE` if the code is unreachable
* use `RX_HINT_{LIKELY, UNLIKELY}` if the branches have heavy weight in a direction
* use `RX_ASSERT` for pre conditions and post condition checking
