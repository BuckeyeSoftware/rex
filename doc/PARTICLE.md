# Particles

Particle emitters in Rex are programmed in a high-level assembly language which
is assembled into a portable binary that targets a bytecode interpreter that
executes SIMD operations on multiple concurrent threads.

# Instruction Set Architecture
The instruction set is a scalar and vector processing machine. Scalars are
represented as IEEE-754 single-precision floats while vectors are represented
by four IEEE-754 single-precision floats.

## Sinks
There are currently three data sinks where data may be read or written to,
these are the "registers" of the ISA.
| Name       | Mnemonic   | Access |
|------------|------------|--------|
| Registers  | `%r{w}{d}` | RW     |
| Channels   | `%c{w}{d}` | RW     |
| Parameters | `%p{w}{d}` | R      |

> `{w}` indicates the "width" and may be `s` for scalar or `v` for vector.

> `{d}` indicates the id, it's a number in the range `0-31`.

Vectors alias scalars, i.s `%rs0` is an alias for the `'x'` component of `%rv0`,
thus vector components are stored in `xyzw` / `rgba` order.

There is a max of 8 vectors (i.e 32 scalars) for each sink.

## Instructions
An instruction can have at most three operands. The operands are given names
`a`, `b`, and `c`, respectively.

With exception to the `mov` instruction, all instructions operate on registers
only. Channels and parameters must be moved into registers before being used.

The following instructions exist
| Mnemonic | Operands | Description                                                     |
|----------|----------|-----------------------------------------------------------------|
| `li`     | 2        | Loads immediate value into reg `a`.                             |
| `mov`    | 2        | Load value from sink `b` and store in sink `a`.                 |
| `add`    | 3        | Add reg `b` and `c` and store in reg `a`.                       |
| `sub`    | 3        | Subtract reg `b` and `c` and store in reg `a`.                  |
| `mul`    | 3        | Multiply reg `b` and `c` and store in reg `a`.                  |
| `mix`    | 3        | Mix values given by reg `a` and `b`, with alpha = reg `c` and clobber `%rs0` if scalar, or `%rv0` if vector with the result
| `rnd`    | 1        | Generate a random number in `[0, 1]` range in reg `a`            |
| `sin`    | 2        | Calculate sine of scalar in reg `b` and store in reg `a`         |
| `cos`    | 2        | Calculate cosine of scalar in reg `b` and strore in reg `a`      |
| `tan`    | 2        | Calculate tangent of scalar in reg `b` and strore in reg `a`     |
| `asin`   | 2        | Calculate arc sine of scalar in reg `b` and store in reg `a`     |
| `acos`   | 2        | Calculate arc cosine of scalar in reg `b` and strore in reg `a`  |
| `atan`   | 2        | Calculate arc tangent of scalar in reg `b` and strore in reg `a` |

## ROM
Immediate data is stored and loaded from a ROM with the `li` instruction. The
ROM has a maximum size of 256 KiB and can store 65536 x IEEE-754 single-precision
floating point values.

## Channels
Channels are output values for the current particle being emitted. The first
few channel ids are built-in and control state values of the particle engine
| ID | Channel Name | Channel Type | Mnemonic |
|----|--------------|--------------|----------|
| 0  | VELOCITY     | VECTOR       | `%cv0`   |
| 1  | ACCELERATION | VECTOR       | `%cv1`   |
| 2  | POSITION     | VECTOR       | `%cv2`   |
| 3  | COLOR        | VECTOR       | `%cv3`   |
| 4  | SIZE         | SCALAR       | `%cs0`   |
| 5  | LIFE         | SCALAR       | `%cs1`   |

> Channels not documented in the table are user channels that may be used for
custom particle effects.

## Parameters
Parameters are input values for the particle program, similar to shader
"uniforms".

Values are supplied from C++ with `Particle::Emitter::operator[]`
```cpp
Particle::Emitter emitter{...};
emitter[0] = 1024.0f; // %ps0 = 1024.0f
```

## Syntax
### Comments
The `#` character introduces a comment. The `#` character and any characters
proceeding it up to a newline is ignored.

### Instructions
Instructions are written one per line. The first operand of an instruction is
separated from the instruction mnemonic by a space, subsequent operands are
comma separated.

### Operands
There are two types of operands: sinks and immediates. With exception to the
`li` "load immediate" instruction, all instructions expect sink operands.

The type is indicated by a prefix
| Type      | Prefix  |
------------|---------|
| Sink      | `%`     |
| Immediate | `$`     |

Immediate values are given literally proceeding the prefix.

> Vector literals have the syntax: `{x, y, z, w}`.

### Example
Here's an example of the language
```py
rnd %rs0                       # Generate random number in rs0
rnd %rs1                       # Generate random number in rs1
mov %rs2, %rs1                 # Copy rs1 to rs2
add %rs0, %rs1, %rs2           # rs0 = rs1 + rs2
li %rv4, ${1.0, 2.0, 3.0, 4.0} # rv4 = {1, 2, 3, 4}
li %rs0, $1024.0               # rs0 = 1024.0
```