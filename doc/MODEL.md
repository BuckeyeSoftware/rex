# Models

Models in Rex are described with a JSON5 file

Every model must define properties through a JSON5 file

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a model looks like:
```
{
  name:      required String
  file:      required String
  transform: optional #ModelTransform
  materials: required Array[#ModelMaterial]
}
```

`#ModelTransform` schema looks like:
```
{
  scale:     optional Array[@Float, 3]
  rotate:    optional Array[@Float, 3]
  translate: optional Array[@Float, 3]
}
```

The `#ModelTransform` is used to transform all vertices in the model on load.
  * `scale` scale in `xyz` order.
  * `rotate` rotation in Euler angles (0 to 360°) in `xyz` order. Angles outside the range saturate.
  * `translate` translation in `xyz` order.

> Rex uses a +Y up coordinate system where +X goes right.

`#ModelMaterial` is either a:
  * `String` path to a JSON5 containing a `#Material` or,
  * `#Material`

Information on `#Material` is described [here](MATERIAL.md)