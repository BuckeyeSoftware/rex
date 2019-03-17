#version 450 core

in vec3 a_position;

uniform mat4 u_transform;

void main() {
  gl_Position = u_transform * vec4(a_position, 1.0);
}