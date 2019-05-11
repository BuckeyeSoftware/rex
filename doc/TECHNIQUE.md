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
  imports:  optional Array[String]
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

`#ShaderSource` is a `String` that is a subset of GLSL with some modifications
outlined [here](SHADER.md)

The inputs of a vertex shader describe the vertex attributes. The outputs of
a vertex shader are values passed to the fragment shader; as such, the outputs
of the vertex shader **must** match the inputs of the fragment shader.

The outputs of a fragment shader describe the fragment data for a target.
Multiple outputs means the technique can only be used with a render target with
multiple attachments. The types must match the attachment types for the render
target.

`#InOutType` is a `String` that is one of
  * `"vec2i"`
  * `"vec3i"`
  * `"vec4i"`
  * `"vec2f"`
  * `"vec3f"`
  * `"vec4f"`
  * `"vec4b"`

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
  * `"bool"`
  * `"int"`
  * `"float"`
  * `"vec2i"`
  * `"vec3i"`
  * `"vec4i"`
  * `"vec2f"`
  * `"vec3f"`
  * `"vec4f"`
  * `"mat4x4f"`
  * `"mat3x3f"`
  * `"bonesf"`

`#UniformValue` can be many types depending on the associated `#UniformType`
  * `"sampler1D" => @Integer`
  * `"sampler2D" => @Integer`
  * `"sampler3D" => @Integer`
  * `"samplerCM" => @Integer`
  * `"bool"      => Boolean`
  * `"int"       => @Integer`
  * `"float"     => Number`
  * `"vec2i"     => Array[#Integer, 2]`
  * `"vec3i"     => Array[#Integer, 3]`
  * `"vec4i"     => Array[#Integer, 4]`
  * `"vec2f"     => Array[#Float, 2]`
  * `"vec3f"     => Array[#Float, 3]`
  * `"vec4f"     => Array[#Float, 4]`
  * `"mat4x4f"   => Array[Array[#Float, 4], 4]`
  * `"mat3x3f"   => Array[Array[#Float, 3], 3]`

A `value` cannot be specified for `bonesf`

`#When` is a `String` that can only be present if `variants` or `permutes`
exists. It encodes a binary expression that is evaluated to conditionally
include or exclude the given entity. The grammar of the expression is given
```
<ident> := [a-Z0-9_]
<op>    := "&&"" | "||"
<expr>  := <ident> | (<expr>) | <expr> <op> <expr> | !<expr>
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