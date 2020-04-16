#version 150

in vec2 tc; // input tex coordinates (computed by the interpolator)
out vec4 c; // output color (the final fragment color)

uniform sampler2D textureImage; // the texture image

void main()
{
 // compute the final fragment color,
 // by looking up into the texture map
 c = texture(textureImage, tc);
} 