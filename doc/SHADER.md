# Shader

Shaders in Rex are written in a custom shading language that is similar in
nature to GLSL. The major difference is the types.

The following generic types exist:
* `f32`
* `s32`
* `u32`
* `f32x2`
* `f32x3`
* `f32x4`
* `s32x2`
* `s32x3`
* `s32x4`
* `u32x2`
* `u32x3`
* `u32x4`
* `f32x3x3`
* `f32x2x4`
* `f32x3x4`
* `f32x4x4`
* `lb_bones`
* `dq_bones`

The following sampler types exist.
* `rx_sampler1D`
* `rx_sampler2D`
* `rx_sampler3D`
* `rx_samplerCM`

Sampling samplers are done with the builtin functions:
* `rx_texture1D`
* `rx_texture2D`
* `rx_texture3D`
* `rx_textureCM`
* `rx_texture1DLod`
* `rx_texture2DLod`
* `rx_texture3DLod`
* `rx_textureCMLod`

The following builtin variables exist:
* `rx_position`
* `rx_vertex_id`
* `rx_instance_id`
* `rx_point_size`
* `rx_point_coord`

> Output of vertex shaders should write to `rx_position`.

Type casts are done with `as_*` intrinsics:
```
s32   as_s32(f32 x);
s32x2 as_s32x2(f32x2 x);
s32x3 as_s32x3(f32x3 x);
s32x4 as_s32x4(f32x4 x);
s32   as_s32(u32 x);
s32x2 as_s32x2(u32x2 x);
s32x3 as_s32x3(u32x3 x);
s32x4 as_s32x4(u32x4 x);
u32   as_u32(f32 x);
u32x2 as_u32x2(f32x2 x);
u32x3 as_u32x3(f32x3 x);
u32x4 as_u32x4(f32x4 x);
u32   as_u32(s32 x);
u32x2 as_u32x2(s32x2 x);
u32x3 as_u32x3(s32x3 x);
u32x4 as_u32x4(s32x4 x);
f32   as_f32(s32 x);
f32x2 as_f32x2(s32x2 x);
f32x3 as_f32x3(s32x3 x);
f32x4 as_f32x4(s32x4 x);
f32   as_f32(u32 x);
f32x2 as_f32x2(u32x2 x);
f32x3 as_f32x3(u32x3 x);
f32x4 as_f32x4(u32x4 x);
```
