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
}
```

`#ShaderType` is a `String` that is one of
  * `"vertex"`
  * `"fragment"`
  * `"compute"`

`#ShaderSource` is a `String` that is a subset of GLSL with some modifications
outlined [here](SHADER.md)

`#InOut` schema looks like:
```
{
  name: required String
  type: required #InOutType
  when: optional #When
}
```

The inputs of a vertex shader describe the vertex attributes, the outputs of
a vertex shader are values passed to the fragment shader, as such the outputs
of the vertex shader **must** match the inputs of the fragment shader.

The outputs of a fragment shader describe the fragment data, multiple outputs
means the technique can only be used with a render target with multiple
attachments. The types must match the attachment types for the render target.

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

As an example, the identity matrix for a `mat4x4f` would look like
```
[
  [1, 0, 0, 0],
  [0, 1, 0, 0],
  [0, 0, 1, 0],
  [0, 0, 0, 1]
]
```

`#When` is a `String` that can only be present if `variants` or `permutes`
exists. It encodes a binary expression that is evaluated to conditionally
include or exclude the given entity. The grammar of the expression is given
```
<ident> := [a-Z0-9_]
<op>    := "&&"" | "||"
<expr>  := <ident> | (<expr>) | <expr> <op> <expr> | !<expr>
```

Here's an example expression

`(a && b) || c && !d`