#version 450 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{ 
    vec4 color = texture(screenTexture, TexCoords);
    FragColor = color;
    // vec4 mapped = color / (color + vec4(1.0));
    // FragColor = vec4(mapped.rgb, 1.);
}