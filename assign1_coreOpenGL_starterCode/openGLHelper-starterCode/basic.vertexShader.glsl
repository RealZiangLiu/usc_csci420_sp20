#version 150

in vec3 position;
in vec3 position_up;
in vec3 position_down;
in vec3 position_left;
in vec3 position_right;
in vec4 color;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform int mode = 0;

const float eps = 1e-5;

void main()
{
  
  if (mode == 0 || mode == 2) {  // Default mode
    // compute the transformed and projected vertex position (into gl_Position) 
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    // compute the vertex color (into col)
    if (mode == 0) {
      col = color;
    } else {
      float curr_color = max(position[2], 0.3);
      col = vec4(curr_color, curr_color, curr_color, 1.0);
    }
  } else {  // Smooth mode
    float smoothHeight = (position_up[2] / 4 + position_down[2] / 4 + position_left[2] / 4 + position_right[2] / 4);
    // compute the transformed and projected vertex position (into gl_Position) 
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position[0], position[1], smoothHeight, 1.0f);
    col = max(color, vec4(eps)) / max(position[2], eps) * smoothHeight;
    // col = color;
  }
}

