# Vendoring a dependency

Vendoring a dependency in Rex correctly involves making some changes to the
dependency source code to be completely free of external symbols, including
standard C library functions. This can be an involved process if the dependency
wasn't written with this in mind.

To simplify the process, look for "dependency free", "free standing", or
"single file" libraries that allow you to replace external symbols either with your own custom functions, or have their own implementations already. This is
the first line of defense. Most high quality, game-ready, or "hand made"
libraries are usually written this way already, prefer using them if you can.

In the unfortunute event that a dependency is not written in this way, or is
only partially, the idiom Rex uses can be described in three parts.
  * Namespace the external symbols and use them.
  * Use the external symbols if an implementation is not found.
  * Implement the external symbols.

## Namespace the external symbols and use them.
When a library uses an external symbol, namespace the external symbol with the
prefix of the library name and make use of it in the library. This is mostly a
process of search and replace on the whole library. To give an example, imagine
we're vendoring `libmp3` and it directly used `malloc`, offering no way to substitute it. This step would see us replace all instances of `malloc` with
`libmp3_malloc`, effectively namespacing it.

Replacing the symbol name in this fashion is necessary because the vendored
code may still include headers that have those symbols. Had we just replaced
`malloc` with a macro, then the `malloc` function inside `stdlib.h`, possibly
included in the vendored code woulld be textually replaced with our macro,
producing an invalid definition.

## Use the external symbols if an implementation is not found.
Vendored code should maintain the invariant that it will compile and run with
the same symbols it was written to use unless explicitly asked to. To maintain
this invariant, the vendored code should perform a series of preprocessor
macro checks for each and every function namespaced, defaulting to the
original symbol if the symbol name is not defined. To borrow our example from
earlier, the `libmp3` vendored code would contain:
```cpp
#ifndef libmp3_malloc
#include <stdlib.h>
#define libmp3_malloc malloc
#endif
```

## Implement the external symbols.
This is the optional step that allows allows replacement of the external symbols
in the vendored library with our own. This is usually done with some source code
which implements the external symbol before including the vendor code.

From the example earlier, this could look something like:
```cpp
#include "rx/core/memory/system_allocator.h"
#define libmp3_malloc(_size) Rx::Memory::SystemAllocator::instance().allocate((_size))
```

Now to toggle between Rex replacements and the actual default code, all one has
to do is comment out the `#define` and rebuild.