# JSON5

Rex makes heavy use of JSON5 as a data description format to describe stuff,
however JSON5 on it's own isn't enough so Rex also describes things with
a given schema for the purposes of documentation with minimal engine enforcement.

The following keywords exist
  * `optional` indicates the entity is optional
  * `required` indicates the entity is required

The following standard types exist
  * `Boolean`
  * `Object`
  * `Array`
  * `Null`
  * `String`
  * `Number`

Non-standard types either begin with a `'#'` or `'@'`.

When a type begins with a `'#'` it means the type is defined by the
documentation in that same file.

When a type begins with a `'@'` it means the type is defined by the engine. The
following engine defined types exist
  * `@Integer` A `Number` with no fractional component
  * `@Float` A `Number` that can fit in an [IEEE 754 single-precision float](https://en.wikipedia.org/wiki/IEEE_754)

Arrays support an optional syntax with the square brackets
  * `Array[T]` each element **will be** type `T` (_T is a type name_)
  * `Array[N]` **exactly** `N` elements (_N is an integer literal_)
  * `Array[T, N]` **exactly** `N` elements and each **will be** type `T`