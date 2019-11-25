# Console

Rex, like many engines before it comes with a console. The purpose of the console is to provide a quick, reactive way to configure the engine. All engine side configuration is done through the console. The console supports typed variables and typed commands. The documentation for that is provided below

# Variables

There's currently ten console variable types, listed here:
* `bool`
* `string`
* `float`
* `int`
* `vec4f`
* `vec4i`
* `vec3f`
* `vec3i`
* `vec2f`
* `vec2i`

With the exception of `bool` and `string` types, all variables of the other types can be given a specific minimum and maximum range of values. This works per component for the vector types.

The placeholder values `-inf` and `+inf` represent the smallest and largest possible values for the type domain they're used in.

All variables persist between engine invocations and are stored in the engine configuration file, `config.cfg`.

Specifying a value for a variable with a different type than the variable will result in a `"Type mistmatch"` error.

# Commands

Commands are typed functions that are callable from the engine console. Commands can be given arbitrary number of arguments that must be matched when called.

Command signatures are specified with a signature string. The following tokens, when
present in the signature string indicate the type of value expected at that position in the command's argument list.

| token |  type    |
|-------|----------|
| `b`   | `bool`   |
| `s`   | `string` |
| `f`   | `float`  |
| `i`   | `int`    |
| `v4f` | `vec4f`  |
| `v4i` | `vec4i`  |
| `v3f` | `vec3f`  |
| `v3i` | `vec3i`  |
| `v2f` | `vec2f`  |
| `v2i` | `vec2i`  |

As an example, the following command specification string `"bsfv2fi"` would describe a command which takes the following arguments, in the following order.
  * `bool`
  * `string`
  * `float`
  * `vec2f`
  * `int`

Failure to match the types will result in a `"Type mismatch"` error. Failure to provide the correct number of arguments will result in a `"Arity violation"` error.

# Parsing

The parsing of all console input is done in accordance of the following grammar, expressed in EBNF format.

```ebnf
decimal-point   = "." ;
decimal-digit   = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
exponent-marker = "e" | "E" ;
sign-marker     = "-" | "+" ;
exponent        = exponent-marker, [ sign-marker ], decimal-digit, { decimal-digit } ;
floating        = [ sign-marker ] , { decimal-digit } , decimal-point , decimal-digit , { decimal-digit } , [ exponent ]
                | [ sign-marker ] , decimal-digit , { decimal-digit } , [ decimal-point , { decimal-digit } ] , exponent ;
integral        = [ sign-marker ] , decimal-digit , { decimal-digit } ;
boolean         = "true" | "false" ;
letter          = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j"
                | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t"
                | "u" | "v" | "w" | "x" | "y" | "z" | "A" | "B" | "C" | "D"
                | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N"
                | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X"
                | "Y" | "Z" ;
character       = letter | decimal-digit | "_" | "." ;
atom            = letter, { character } ;
terminal        = "'" , character , { character } , "'"
                | '"' , character , { character } , '"' ;
vector          = "{" , floating , { ( "," , floating ) } , "}"
                | "{" , integral , { ( "," , integral ) } , "}" ;
```

The `atom` token in particular is what refers to a variable or command by name. It's always expected as the first token in a string.
