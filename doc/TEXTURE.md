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
  generate:   optional #NormalGenerate
}
```

`#TextureType` is a `String` that is one of:
  * `"albedo"`
  * `"normal"`
  * `"metalness"`
  * `"roughness"`
  * `"ambient"`
  * `"emissive"`

Where the types are:
* `"albedo"` an albedo texture, like diffuse but without shadows or highlights.
* `"normal"` tangent space normal map. Tangent space normals assume `0.5` is a _null_ component, `0.0` is a `-1` component and `1.0` is a `+1` component. Where RGB is XYZ respectively.
* `"metalness"` PBR metalness map (like those exported by [Substance Painter](https://www.substance3d.com/products/substance-painter) or [Marmoset Toolbag](https://marmoset.co/toolbag/))
* `"roughness"` PBR roughness map (like those exported by [Substance Painter](https://www.substance3d.com/products/substance-painter) or [Marmoset Toolbag](https://marmoset.co/toolbag/))
* `"ambient"` ambient occlusion map to apply to indirect diffuse lighting.
* `"emissive"` emissive lighting map.

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
| `"nearest"`         | `true`    | nearst_mipmap_nearest | nearest       | Chooses the mipmap that most closely matches the size of the pixel being textured and uses the criteron above to produce a texture value.
| `"bilinear"`        | `false`   | linear                | linear        | Uses weighted average of the four texture elements that are closest to the center of the pixel being textured.
| `"bilinear"`        | `true`    | linear_mipmap_nearest | linear        | Chooses the mipmap that most closely matches the size of pixel being textured and uses the criteron above to produce a texture value.
| `"trilinear"`       | `true`    | linear_mipmap_linear  | linear        | Chooses the two mipmaps that most closely match the size of the pixel being textured and uses the criteron two above to produce a texture value from each mipmap. The final texture value is a weighted average of those two values.

There is no filter combination that produces a minification of `nearest_mipmap_linear`. All trilinear filtering implies bilinear filtering in Rex. This is consistent with UE4 and NV's control panel's "quality" setting which makes `nearest_mipmap_linear` an alias for `linear_mipmap_linear`.

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

Use of `#Texture.generate` is invalid unless `#Texture.type` is `"normal"`. When using generate, you may specify the automatic generation of normal maps.

The top level schema of a `#NormalGenerate` looks like:
```
{
  mode:       required #NormalMode
  kernel:     required #NormalKernel
  multiplier: required Array[Number, 3]
  strength:   required Number
  flags:      required Array[#NormalFlag]
}
```

`#NormalMode` is a `String` that is one of:
  * `"max"` Take the maximum component of the pixel.
  * `"average"` Take the average component of the pixel, this is more accurate for luminance but fails when a given image contains too much baked lighting.

`#NormalKernel` is a `String` that is one of:
  * `"sobel"` Sobel edge detection, calculates the gradient of the image intensity. At each point in the image, the result is either a corresponding gradient vector or the norm of this vector. Produces relatively crude results for high-frequency variations in the image.
  * `"prewitt"` Prewitt operator, calculates the gradient of the image intensity at each point, giving the direction of the largest possible increase from light to dark and the rate of change in that direction. Faster than `"sobel"` but even more crude for high-frequency variations in the image.

`#NormalGenerate.strength` is a scalar which describes how strong the normal displacement should behave as, `2.0` is a safe value for most textures. Negative values will invert the result.

`#NormalGenerate.flags` is an array of `#NormalFlag`.
  * `"invert"` Inverts the given direction of displacement.
  * `"tile"` If the normal map must be tiled, this should be given.
  * `"detail"` Try to preserve large detail in the normal map.
