#!/bin/env python3
from dataclasses import dataclass
from pathlib import Path
from struct import unpack

@dataclass
class Vec3:
  x: float
  y: float
  z: float

  def as_json5(self):
    return f'[{self.x}, {self.y}, {self.z}]'

def parse_scalar(token):
  return float(token)

def parse_bool(token):
  return True if token == 'true' else False

def parse_color(tokens):
  r = parse_scalar(tokens[0])
  g = parse_scalar(tokens[1])
  b = parse_scalar(tokens[2])
  return Vec3(r, g, b)

def clamp(value, min_value, max_value):
  return max(min_value, min(value, max_value))

@dataclass
class Map:
  file: str
  wrap_s: str
  wrap_t: str

  def as_json5(self, i, type):
    contents  = ' '*((i+0)*2) +  '{\n'
    contents += ' '*((i+1)*2) + f'file: "{self.file}",\n'
    contents += ' '*((i+1)*2) + f'type: "{type}",\n'
    contents += ' '*((i+1)*2) +  'filter: "bilinear",\n'
    contents += ' '*((i+1)*2) +  'wrap: [\n'
    contents += ' '*((i+2)*2) + f'"{self.wrap_s}",\n'
    contents += ' '*((i+2)*2) + f'"{self.wrap_t}"\n'
    contents += ' '*((i+1)*2) +  ']\n'
    contents += ' '*((i+0)*2) +  '}\n'
    return contents

@dataclass
class Material:
  name: str = None
  alpha_test: bool = False
  albedo_color: Vec3 = Vec3(0, 0, 0)
  albedo_map: str = None
  normal_map: str = None
  metalness: float = 0.0
  roughness: float = 1.0

  def as_json5(self, i):
    contents  =  ' '*((i+0)*2) +  '{\n'
    contents +=  ' '*((i+1)*2) + f'name: "{self.name}",\n'
    if self.alpha_test:       contents += ' '*((i+1)*2) +  'alpha_test: true,\n'
    if self.roughness != 1.0: contents += ' '*((i+1)*2) + f'roughness: {self.roughness},\n'
    if self.metalness != 0.0: contents += ' '*((i+1)*2) + f'metalness: {self.metalness},\n'
    if self.albedo_map or self.normal_map:
      contents += ' '*((i+1)*2) + 'textures: [\n'
      if self.albedo_map:
        contents += self.albedo_map.as_json5(i+2, "albedo")
      if self.normal_map:
        if self.albedo_map: contents = contents[:-1] + ",\n"
        contents += self.normal_map.as_json5(i+2, "normal")
      if not self.albedo_map and self.normal_map:
        contents += ' '*((i+1)*2) + '],\n'
      else:
        contents += ' '*((i+1)*2) + ']\n'
    if not self.albedo_map:
      contents += ' '*((i+1)*2) + f'albedo: {self.albedo_color.as_json5()}\n'
    contents +=  ' '*((i+0)*2) + '}\n'
    return contents

def parse_map(base, tokens):
  file = tokens[0]
  wrap_s = "repeat"
  wrap_t = "repeat"
  if len(tokens) > 1: wrap_s = tokens[1]
  if len(tokens) > 2: wrap_t = tokens[2]
  return Map(f'./{base}/{file[2:]}', wrap_s, wrap_t)

def convert_kmat(file_name):
  path = Path(file_name)
  material = Material()
  with open(file_name) as file:
    material.name = path.stem
    for line in file.readlines():
      tokens = line.strip().split(' ')
      if not tokens[0]:
        continue
      if tokens[0] == 'transparent':
        material.alpha_test = parse_bool(tokens[1])
      elif tokens[0] == 'diffuse':
        material.albedo_map = parse_map(path.parent, tokens[1:])
      elif tokens[0] == 'color':
        material.albedo_color = parse_color(tokens[1:])
      elif tokens[0] == 'normal':
        material.normal_map = parse_map(path.parent, tokens[1:])
      elif tokens[0] == 'specular':
        spec_i = parse_scalar(tokens[1]) / 255.0
        spec_p = parse_scalar(tokens[2]) / 1000.0

        material.metalness = 1.0 - spec_i
        material.roughness = clamp(1.0 - spec_p, 0.0, 1.0)

        # Low specularity should produce a rough material
        # The 0.1 value was picked by GGX.
        if spec_i < 0.1:
          material.roughness *= 1.0 - spec_i
          material.metalness *= 0.1
  return material

# Load in the IQM, read the material names, emit the JSON.
def open_iqm(file_name):
  path = Path(file_name)

  file = open(file_name, "rb")
  file.seek(28) # num_text, ofs_text
  num_text = unpack('<I', file.read(4))[0]
  ofs_text = unpack('<I', file.read(4))[0]

  num_meshes = unpack('<I', file.read(4))[0]
  ofs_meshes = unpack('<I', file.read(4))[0]

  materials = []
  file.seek(ofs_meshes)
  for mesh_index in range(num_meshes):
    read = unpack('<IIIIII', file.read(6 * 4))
    materials.append(read[1])

  file.seek(ofs_text)
  string_table = file.read(num_text).decode('utf-8')

  i = 0
  contents  = ' '*((i+0)*2) +  '{\n'
  contents += ' '*((i+1)*2) + f'name: "{path.stem}",\n'
  contents += ' '*((i+1)*2) + f'file: "./{file_name}",\n'
  contents += ' '*((i+1)*2) +  'materials: [\n'
  names = []
  for material in materials:
    string = []
    while True:
      ch = string_table[material + len(string)]
      if ch == chr(0):
        # contents += convert_kmat(f"./{path.parent}/{''.join(string)}.kmat").as_json5(i+2)
        # print(''.join(string))
        name = ''.join(string)
        contents += ' '*((i+2)*2) + '{\n'
        contents += ' '*((i+3)*2) + f'name: "{name}",\n'
        contents += ' '*((i+3)*2) +  'albedo: [1.0, 1.0, 1.0]\n'
        contents += ' '*((i+2)*2) + '}\n'
        if material != materials[-1]:
          contents = contents[:-1] + ',\n'
        break
      string.append(ch)

  # Open all the appropriate kmat files now.
  contents += ' '*((i+1)*2) + ']\n'
  contents += ' '*((i+0)*2) + '}\n'

  with open(f'./{path.parent}/{path.stem}.json5', 'w') as json5:
    json5.write(contents)

# def convert_kmats(path):
for file in Path('./chess').glob('*.iqm'):
  open_iqm(file)
