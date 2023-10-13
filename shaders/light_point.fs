#version 450 core

in vec4 vertexColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 viewPos;

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

uniform Material material;

struct Light {
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

uniform Light light;
uniform vec2 emissionOffset;

void main() {
  // Attenuation
  float distance = length(light.position - FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));

  // Ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
  // Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  // vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoords));

  FragColor = vec4((ambient + diffuse + specular) * attenuation, 1.);
}