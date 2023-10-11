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
  sampler2D emission;
  float emissionStrength;
  float shininess;
};

uniform Material material;

struct Light {
  vec3 position;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform Light light;
uniform vec2 emissionOffset;

void main() {
  // Ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
  // Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoords));

  vec3 emission = material.emissionStrength *
                  vec3(texture(material.emission, TexCoords + emissionOffset));

  FragColor = vec4(ambient + diffuse + specular + emission, 1.);
}