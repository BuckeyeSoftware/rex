# Models

Models in Rex are described with a JSON5 file

Every model must define properties through a JSON5 file

Information on how to read this schema is described [here](JSON5.md)

The top level schema of a model looks like:
```
{
  name:      required String
  file:      required String
  materials: required Array[#ModelMaterial]
}
```

`#ModelMaterial` is either a:
  * `String` path to a JSON5 containing a `#Material` or,
  * `#Material`

Information on `#Material` is described [here](MATERIAL.md)