#version 450 core

in vec4 vertexColor;
in vec3 Normal;
in vec3 FragPos;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
float ambientStrength = 0.1;
float specularStrength = 0.5;

void main() {
    vec3 light = vec3(0.0);
    // Ambient
    light += ambientStrength * lightColor;
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    light += vec3(diff) * lightColor;
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    vec3 specular = specularStrength * spec * lightColor;  
    light += specular;

    FragColor = vec4(objectColor * light, 1.);
}