#version 450 core

in vec4 vertexColor;
out vec4 FragColor;

uniform vec3 lightColor;

void main() {
    FragColor = vec4(lightColor, 1.);
}