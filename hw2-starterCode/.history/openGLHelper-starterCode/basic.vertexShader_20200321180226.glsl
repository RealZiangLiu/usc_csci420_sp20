// #version 150

// in vec3 position;
// in vec4 color;
// out vec4 col;

// uniform mat4 modelViewMatrix;
// uniform mat4 projectionMatrix;

// void main()
// {
  // gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  // col = color;
// }

#version 150

in vec3 position;
in vec3 normal;

out vec3 viewPosition;
out vec3 viewNormal;

uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // view-space position of the vertex
  vec4 viewPosition4 = modelViewMatrix * vec4(position, 1.0f);
  viewPosition = viewPosition4.xyz;
  // final position in the normalized device coordinates space
  gl_Position = projectionMatrix * viewPosition4;
  // view-space normal
  viewNormal = normalize((normalMatrix*vec4(normal, 0.0f)).xyz);
}