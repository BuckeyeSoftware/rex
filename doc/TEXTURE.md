# Textures

Textures in Rex are described with JSON5.

Every texture must define properties through JSON5.

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a texture looks like:
```
{
  file:       required String
  type:       required #TextureType
  filter:     required #TextureFilter
  wrap:       required Array[#TextureWrap]
  mipmaps:    optional Boolean
  border:     optional Array[@Float, 4]
}
```

`#TextureType` is a `String` that is one of:
  * `"albedo"`
  * `"normal"`
  * `"metalness"`
  * `"roughness"`
  * `"ambient"`

Where the types are:
* `"albedo"` an albedo texture, like diffuse but without shadows or highlights.
* `"normal"` tangent space normal map. Tangent space normals assume `0.5` is a _null_ component, `0.0` is a `-1` component and `1.0` is a `+1` component. Where RGB is XYZ respectively.
* `"metalness"` PBR metalness map (like those exported by [Substance Painter](https://www.substance3d.com/products/substance-painter) or [Marmoset Toolbag](https://marmoset.co/toolbag/))
* `"roughness"` PBR roughness map (like those exported by [Substance Painter](https://www.substance3d.com/products/substance-painter) or [Marmoset Toolbag](https://marmoset.co/toolbag/))
* `"ambient"` PBR ambient occlusion map (like those exported by [Substance Painter](https://www.substance3d.com/products/substance-painter) or [Marmoset Toolbag](https://marmoset.co/))

See the [Physically-Based Shading at Disney](https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf) paper for more information on PBR materials and the shading Rex uses.

`#TextureFilter` is a `String` that is one of:
  * `"nearest"`
  * `"bilinear"`
  * `"trilinear"`

When filter is `"trilinear"`, `#Texture.mipmaps` is always treated as `true`.

This provides five possible filter configurations and what they do are listed:

| `#TextureFilter`    | `mipmaps` | Minification          | Magnification | Description
| ------------------- | --------- | :-------------------: | :-----------: | -----------
| `"nearest"`         | `false`   | nearest               | nearest       | Uses element that is nearest, (in [Manhattan distance](https://en.wikipedia.org/wiki/Taxicab_geometry)) to the center of pixel being textured.
| `"nearest"`         | `true`    | nearst_mipmap_nearest | nearest       | Chooses the mipmap that most closely matches the size of the pixel being textured and uses the criteron one above (texture element nearest to center of the pixel) to produce a texture value.
| `"bilinear"`        | `false`   | linear                | linear        | Uses weighted average of the foru texture elements that are closest to the center of the pixel being textured.
| `"bilinear"`        | `true`    | linear_mipmap_nearest | linear        | Chooses the mipmap that most closely matches the size of pixel being textured and uses the criteron one above (a weighted average of the four texture elements that are closest to the center of the pixel) to produce a texture value.
| `"trilinear"`       | `true`    | linear_mipmap_linear  | linear        | Chooses the two mipmaps that most closely match the size of the pixel being textured and uses the criteron two above (a weighted average of the four texture elements that are closest to the center of the pixel) to produce a texture value from each mipmap. The final texture value is a weighted average of those two values.

There is no filter combination that produces a minification of `nearest_mipmap_linear`. All trilinear filtering implies bilinear filtering in Rex. This is consistent with UE4 and NV's control panel's "quality" setting which makes `nearest_mipmap_linear` and alias for `linear_mipmap_linear`.

`#TextureWrap` is a `String` that is one of:
  * `"clamp_to_edge"`
  * `"clamp_to_border"`
  * `"mirrored_repeat"`
  * `"repeat"`
  * `"mirror_clamp_to_edge"`

`#Texture.wrap` is an array where each element indicates how to wrap that axis. The axes being `s`, `t` and `p`.

* `"clamp_to_edge"` causes the coordinates to be clamped to the range `(1/2n, 1-1/2n)`, where `n` is the size of the texture in the direction of clamping.
* `"clamp_to_border"` evaluates coordinates in a similar manner to `"clamp_to_edge"`. However, in cases where clamping would have occured in `"clamp_to_edge"`, the fetched color is substituted with the color specified by `#Texture.border`.
* `"repeat"` causes the integer part of the coordinate to be ignored; Rex only uses the fractional part, thereby creating a repeating pattern.
* `"mirrored_repeat"` causes the coordinate to be set to the fractional part of the texture coordinate if the integer part of it is even; if the integer part is odd, then the coordinate is set to `1 - frac(c)`, where `frac(c)` represents the fractional part of the coordinate.
* `"mirror_clamp_to_edge"` causes the coordinate to be repeated for `"mirrored_repeat"` for one repretition of the texture, at which point the coordinate to be clamped as in `"clamp_to_edge"`.

Use of `#Texture.border` is invalid unless one of the axis specified by `#Texture.wrap` is `"clamp_to_border"`.