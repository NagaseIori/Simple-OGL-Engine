#version 450 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
const float gamma = 2.2;

void main()
{ 
    vec4 color = texture(screenTexture, TexCoords);
    FragColor = vec4(pow(color.rgb, vec3(1./gamma)), color.a);
    // vec4 mapped = color / (color + vec4(1.0));
    // FragColor = vec4(mapped.rgb, 1.);
}