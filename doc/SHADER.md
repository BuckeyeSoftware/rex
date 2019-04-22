# Shader

Shaders in Rex are represented as a subset of GLSL

The following types exist
```
  int, bool, float, vec2i, vec3i, vec4i, vec2f, vec3f, vec4f, mat3x3f, mat4x4f, mat3x4f
```

The following samplers exist
```
rx_sampler1D, rx_sampler2D, rx_sampler3D, rx_samplerCM
```

Output from the vertex shader writes to `rx_position`