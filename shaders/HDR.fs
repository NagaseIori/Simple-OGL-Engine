#version 450 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float exposure;

void main()
{ 
    vec3 color = texture(screenTexture, TexCoords).rgb;
    vec3 mapped = vec3(1.0) - exp(-color * exposure);
    const float gamma = 2.2;
    mapped = pow(mapped, vec3(1.0 / gamma));
    FragColor = vec4(mapped, 1.);
}