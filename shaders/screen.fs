#version 450 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{ 
    vec4 color = texture(screenTexture, TexCoords);
    color = clamp(color, 0.0, 1.0);
    FragColor = color;
}