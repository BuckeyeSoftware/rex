# Techniques

Techniques in Rex are described with a JSON5 file, a technique describes a
baked set of shaders for rendering.

Every technique must describe shaders, their inputs and outputs as well
as any uniforms that may be utilized.

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a technique looks like
```
{
  name:     required String
  shaders:  required Array[#Shader]
  uniforms: optional Array[#Uniform]
  permutes: optional Array[String]
  variants: optional Array[String]
}
```

There cannot be both `variants` and `permutes`

`#Shader` schema looks like:
```
{
  type:     required #ShaderType
  inputs:   required Array[#InOut]
  outputs:  required Array[#InOut]
  source:   required #ShaderSource
  imports:  optional Array[#Import]
}
```

`#ShaderType` is a `String` that is one of
  * `"vertex"`
  * `"fragment"`
  * `"compute"`

`#InOut` schema looks like:
```
{
  name: required String
  type: required #InOutType
  when: optional #When
}
```

`#ShaderSource` is a `String` that is Rex shading source code. The shading
language is outlined [here](SHADER.md)

`#Import` is a `String` of the module name or `#ImportObject`

`#ImportObject` schema looks like:
```
{
  name: required String
  when: #When
}
```

The inputs of a vertex shader describe the vertex attributes. The outputs of
a vertex shader are values passed to the fragment shader; as such, the outputs
of the vertex shader **must** match the inputs of the fragment shader.

The outputs of a fragment shader describe the fragment data for a target.
Multiple outputs means the technique can only be used with a render target with
multiple attachments. The types must match the attachment types for the render
target.

`#InOutType` is a `String` that is one of
  * `"s32x2"`
  * `"s32x3"`
  * `"s32x4"`
  * `"s32x2"`
  * `"s32x3"`
  * `"s32x4"`

`#Uniform` schema looks like:
```
{
  name:  required String
  type:  required #UniformType
  value: optional #UniformValue
  when:  optional #When
}
```

`#UniformType` is a `String` that is one of
  * `"sampler1D"`
  * `"sampler2D"`
  * `"sampler3D"`
  * `"samplerCM"`
  * `"s32"`
  * `"f32"`
  * `"s32x2"`
  * `"s32x3"`
  * `"s32x4"`
  * `"f32x2"`
  * `"f32x3"`
  * `"f32x4"`
  * `"f32x4x4"`
  * `"f32x3x3"`
  * `"lb_bones"`
  * `"dq_bones"`

`#UniformValue` can be many types depending on the associated `#UniformType`
  * `"sampler1D" => @Integer`
  * `"sampler2D" => @Integer`
  * `"sampler3D" => @Integer`
  * `"samplerCM" => @Integer`
  * `"s32"       => @Integer`
  * `"f32"       => @Float`
  * `"s32x2"     => Array[@Integer, 2]`
  * `"s32x3"     => Array[@Integer, 3]`
  * `"s32x4"     => Array[@Integer, 4]`
  * `"f32x2"     => Array[@Float, 2]`
  * `"f32x3"     => Array[@Float, 3]`
  * `"f32x4"     => Array[@Float, 4]`
  * `"f32x4x4"   => Array[Array[@Float, 4], 4]`
  * `"f32x3x3"   => Array[Array[@Float, 3], 3]`

A `value` cannot be specified for `lb_bones` or `dq_bones`.

`#When` is a `String` that can only be present if `variants` or `permutes`
exists. It encodes a binary expression that is evaluated to conditionally
include or exclude the given entity. The grammar of the expression is given in [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form):
```ebnf
letter     = "A" | "B" | "C" | "D" | "E" | "F" | "G"
           | "H" | "I" | "J" | "K" | "L" | "M" | "N"
           | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
           | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
           | "c" | "d" | "e" | "f" | "g" | "h" | "i"
           | "j" | "k" | "l" | "m" | "n" | "o" | "p"
           | "q" | "r" | "s" | "t" | "u" | "v" | "w"
           | "x" | "y" | "z" ;
digit      = "0" | "1" | "2" | "3" | "4" | "5" | "6"
           | "7" | "8" | "9" ;
identifier = letter , { letter | digit | "_" } ;
expression = expression, "&&", value
           | expression, "||", value
           | value ;
value      = element
           | "!", element ;
element    = "(", expression, ")"
           | identifier ;
```

The purpose of `variants` is to describe a list of tokens used to control shader
variants. A program is created for each variant. The token in the variants array
can then be used to conditionally include or exclude bodies of code in the
source through the use of the preprocerssor as well as through the use of the
`#When` for `#InOut` and `#Uniform` entities.

The purpose of `permutes` is to a describe a list of tokens used to control
shader permutations. A program is created for every permutation of tokens in
the permutes list. Tokens in the permutes array can then be used to
conditionally include or exclude bodies of code in the source through the use of
the preprocessor as well as through the use of the `#When` for `#InOut` and
`#Uniform` entities.
