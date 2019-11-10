# Materials

Materials in Rex are described JSON5.

Every material must define properties through JSON5.

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a material looks like:
```
{
  name:       required String
  textures:   required Array[#MaterialTexture]
  alpha_test: optional Boolean
  roughness:  optional Float
  metalness:  optional Float
  transform:  optional #MaterialTransform
}
```

`#MaterialTexture` is either a:
  * `String` path to a JSON5 file containing a `#Texture` or,
  * `#Texture`

Information on `#Texture` is described [here](TEXTURE.md)

* `alpha_test` controls if surfaces with this material will be alpha tested or not.
* `roughness` saturated value in [0, 1] range which acts as the roughness for the whole surface when a roughness map isn't present or as a multiplier if present. A val
* `metalness` saturated value in [0, 1] range which acts as the metalness for the whole surface when a metalness map isn't present or as a multiplier if present.

`#MaterialTransform` schema looks like:
```
{
  scale:     optional Array[@Float, 2]
  rotate:    optional Array[@Float, 2]
  translate: optional Array[@Float, 2]
}
```

The `#MaterialTransform` is used to transform texture coordinates of the surface where the material is applied.
  * `scale` scale in `st` order.
  * `rotate` rotation in Euler angles (0 to 360°) in `st` order. Angles outside the range saturate.
  * `translate` translation in `st` order.

What is `S` and `T` depends on how the surface the material is applied on is modeled. The orientation depends on how the content was authored.

Artists should try to maintain a **+T is up and +S is right** coordinate system when UV mapping surfaces to follow the conventions of the engine which uses a **+Y up and +X right** coordinate system. This will decrease the confusion when using `#MaterialTransform`.

The behavior of material transforms is **highly** dependent on the wrap mode set in the textures of the material.
