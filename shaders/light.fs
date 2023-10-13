#version 450 core
#define LIGHT_MAX_COUNT 64

in vec4 vertexColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 viewPos;
uniform int lightCount;

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

uniform Material material;

struct Light {
  vec3 position;
  vec3 direction;
  float cutOff;
  float outerCutOff;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  float constant;
  float linear;
  float quadratic;

  int type; // 0 point 1 direction 2 spot
};

uniform Light lights[LIGHT_MAX_COUNT];

vec3 pointLight(Light light) {
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

  return (ambient + diffuse + specular) * attenuation;
}

vec3 spotLight(Light light) {
  vec3 lightDir = normalize(light.position - FragPos);
  // Soft Outer Shadow
  float theta = dot(lightDir, normalize(-light.direction));
  float epsilon = light.cutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  // Attenuation
  float distance = length(light.position - FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));
  // Ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
  // Diffuse
  vec3 norm = normalize(Normal);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoords));
  return (ambient + (diffuse + specular) * intensity) * attenuation;
}

vec3 directionLight(Light light) {
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
  // Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoords));
  return ambient + diffuse + specular;
}

void main() {
  vec3 color = vec3(0.);
  for (int i = 0; i < lightCount; i++) {
    if (lights[i].type == 0)
      color += pointLight(lights[i]);
    if (lights[i].type == 1)
      color += directionLight(lights[i]);
    if (lights[i].type == 2)
      color += spotLight(lights[i]);
  }
  FragColor = vec4(color, 1.);
}