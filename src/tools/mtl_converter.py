#!/bin/env python3
from dataclasses import dataclass
from sys import argv

# map_Kd => base color texture
# map_Ks => metallic texture
# map_Ns => roughness texture
# map_Ka => occlusion texture
# map_Ke => emissive texture
# map_Bump => normal map (tangent space)
# Kd => base color
# Ks => metallic value
# Ns => roughness value
# Ka => occlusion value
# Ke => emissive color
# d => alpha
# Tr => 1.0 - alpha

@dataclass
class Vec3:
  x: float
  y: float
  z: float

  def is_all_zero(self):
    return self.x == 0.0 and self.y == 0.0 and self.z == 0.0

  def is_all_one(self):
    return self.x == 1.0 and self.y == 1.0 and self.z == 1.0

  def as_json5(self):
    return f'[{self.x}, {self.y}, {self.z}]'

  def gray(self):
    return self.x*0.1140 + self.y*0.5870 + self.z*0.2989

def parse_scalar(token):
  return float(token)

def parse_color(tokens):
  r = parse_scalar(tokens[0])
  g = parse_scalar(tokens[1])
  b = parse_scalar(tokens[2])
  return Vec3(r, g, b)

@dataclass
class Material:
  name: str
  albedo_color: Vec3
  emissive_color: Vec3
  metalness_value: float
  roughness_value: float
  occlusion_value: float
  alpha: float
  albedo_map: str
  metalness_map: str
  roughness_map: str
  occlusion_map: str
  emissive_map: str
  normal_map: str

  def __init__(self, name):
    self.name = name
    self.albedo_color = Vec3(1.0, 1.0, 1.0)
    self.emissive_color = Vec3(1.0, 1.0, 1.0)
    self.metalness_value = 0.0
    self.roughness_value = 1.0
    self.occlusion_value = 0.0
    self.alpha = 1.0
    self.albedo_map = None
    self.metalness_map = None
    self.roughness_map = None
    self.occlusion_map = None
    self.emissive_map = None
    self.normal_map = None

  def has_maps(self):
    return self.albedo_map or self.metalness_map or self.roughness_map or self.emissive_map or self.normal_map

  def as_json5(self):
    parts = []
    parts.append(f'name: "{self.name}"')
    if not self.albedo_color.is_all_one():
      parts.append(f'albedo: {self.albedo_color.as_json5()}')
    if self.metalness_value != 1.0:
      parts.append(f'metalness: {self.metalness_value}')
    if self.roughness_value != 0.0:
      parts.append(f'roughness: {self.roughness_value}')
    if self.occlusion_value != 0.0:
      parts.append(f'occlusion: {self.occlusion_value}')
    if not self.emissive_color.is_all_one():
      parts.append(f'emission: {self.emissive_color.as_json5()}')
    if self.alpha != 1.0:
      parts.append(f'alpha: {self.alpha}')

    textures = []
    if self.albedo_map:
      textures.append(self.map_as_json5('albedo', self.albedo_map))
    if self.metalness_map:
      textures.append(self.map_as_json5('metalness', self.metalness_map))
    if self.roughness_map:
      textures.append(self.map_as_json5('roughness', self.roughness_map))
    if self.occlusion_map:
      textures.append(self.map_as_json5('occlusion', self.occlusion_map))
    if self.emissive_map:
      textures.append(self.map_as_json5('emissive', self.emissive_map))
    if self.normal_map:
      textures.append(self.map_as_json5('normal', self.normal_map))
    if textures:
      parts.append(f'textures: [' + ', '.join(textures) + ']')
    return '{' + ', '.join(parts) + '}'

  @staticmethod
  def normalize_path(path):
    return path.replace('\\', '/')

  @staticmethod
  def map_as_json5(map, filename):
    parts = []
    parts.append(f'type: "{map}"')
    parts.append(f'wrap: ["repeat", "repeat"]')
    parts.append(f'filter: "bilinear"')
    parts.append(f'file: "{Material.normalize_path(filename)}"')
    return '{' + ', '.join(parts) + '}'

def run(filename):
  materials = []
  material = None

  with open(filename) as file:
    for line in file:
      # Remove leading and trailing whitespace on the line
      line = line.strip()

      # Skip empty lines
      if len(line) == 0:
        continue

      # Split the line token by whitespace
      tokens = line.split(' ')

      if tokens[0] == 'newmtl':
        if material:
          materials.append(material)
        material = Material(tokens[1])

      if material:
        if tokens[0] == 'map_Kd':
          material.albedo_map = tokens[1]
        elif tokens[0] == 'map_Ks':
          material.metalness_map = tokens[1]
        elif tokens[0] == 'map_Ns':
          material.roughness_map = tokens[1]
        elif tokens[0] == 'map_Ka':
          material.occlusion_map = tokens[1]
        elif tokens[0] == 'map_Ke':
          material.emissive_map = tokens[1]
        elif tokens[0] == 'map_Bump':
          material.normal_map = tokens[1]
        elif tokens[0] == 'Kd':
          material.albedo_color = parse_color(tokens[1:])
        elif tokens[0] == 'Ks':
          material.metalness_value = parse_scalar(tokens[1])
        elif tokens[0] == 'Ns':
          material.roughness_value = parse_scalar(tokens[1])
        elif tokens[0] == 'Ka':
          material.occlusion_value = parse_color(tokens[1:]).gray()
        elif tokens[0] == 'Ke':
          material.emissive_color = parse_color(tokens[1:])
        elif tokens[0] == 'd':
          material.alpha = parse_scalar(tokens[1])
        elif tokens[0] == 'Tr':
          material.alpha = 1.0 - parse_scalar(tokens[1])

  if material:
    materials.append(material)

  return materials

if __name__ == '__main__':
  materials = run(argv[1])
  print('{')
  print(f'  name: "{argv[2]}",')
  print(f'  file: "{argv[3]}",')
  print(f'  materials: [')
  for material in materials:
    print('    ' + material.as_json5() + ',')
  print('  ]')
  print('}')
