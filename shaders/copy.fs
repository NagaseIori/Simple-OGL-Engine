#version 450 core

uniform sampler2D texIn;
in vec2 TexCoords;
out vec4 FragColor;

void main() {
  FragColor = texture(texIn, TexCoords);
}