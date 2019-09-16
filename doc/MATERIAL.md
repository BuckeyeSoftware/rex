# Materials

Materials in Rex are described with a JSON5

Every material must define properties through JSON5

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a material looks like:
```
{
  name:     required String
  textures: required Array[#Texture]
}
```

`#Texture` schema looks like:
```
{
  file:       required String
  type:       required #TextureType
  filter:     required #Filter
  wrap:       required Array[#Wrap]
  mipmaps:    optional Boolean
  alpha_test: optional Boolean
}
```

`#TextureType` is a `String` that is one of:
  * `"albedo"`
  * `"normal"`
  * `"metalness"`
  * `"roughness"`

`#Filter` is a `String` that is one of:
  * `"nearest"`
  * `"bilinear"`
  * `"trilinear"`

When filter is `"trilinear"` `#Texture.mipmaps` is always treated as `true`

`#Wrap` is a `String` that is one of:
  * `"clamp_to_edge"`
  * `"clamp_to_border"`
  * `"mirrored_repeat"`
  * `"repeat"`
  * `"mirror_clamp_to_edge"`