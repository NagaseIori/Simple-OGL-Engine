#version 450 core
#define LIGHT_MAX_COUNT 64
#define MATERIAL_MAX_COUNT 8

in vec4 vertexColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 viewPos;
uniform int lightCount;

struct Material {
  sampler2D diffuse[MATERIAL_MAX_COUNT];
  sampler2D specular[MATERIAL_MAX_COUNT];
  sampler2D normal[MATERIAL_MAX_COUNT];
  sampler2D height[MATERIAL_MAX_COUNT];
  int diffuse_c;
  int specular_c;
  int normal_c;
  int height_c;
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

vec4 getMaterialDiffuseColor(Material material, vec2 TexCoords) {
  if(material.diffuse_c == 0) return vec4(0.);
  vec4 color = vec4(0.);
  for(int i=0; i<material.diffuse_c; i++)
    color += texture(material.diffuse[i], TexCoords);
  return color;
}

vec4 getMaterialSpecularColor(Material material, vec2 TexCoords) {
  if(material.specular_c == 0) return vec4(0.);
  vec4 color = vec4(0.);
  for(int i=0; i<material.specular_c; i++)
    color += texture(material.specular[i], TexCoords);
  return color;
}

vec3 getMaterialNormal(Material material, vec2 TexCoords) {
  if(material.normal_c == 0) return vec3(0.);
  vec4 color = vec4(0.);
  for(int i=0; i<material.normal_c; i++)
    color += texture(material.normal[i], TexCoords);
  return vec3(color);
}

vec4 pointLight(Light light) {
  // Attenuation
  float distance = length(light.position - FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));

  // Ambient
  vec4 ambient = vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));
  vec3 lightDir = normalize(light.position - FragPos);
  // vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(norm, lightDir), 0.0);
  vec4 diffuse =
      vec4(light.diffuse, 1.0) * diff * getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular =
      vec4(light.specular, 1.0) * spec * getMaterialSpecularColor(material, TexCoords);

  return (ambient + diffuse + specular) * attenuation;
}

vec4 spotLight(Light light) {
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
  vec4 ambient = vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));

  float diff = max(dot(norm, lightDir), 0.0);
  vec4 diffuse =
      vec4(light.diffuse, 1.0) * diff * getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular =
      vec4(light.specular, 1.0) * spec * getMaterialSpecularColor(material, TexCoords);
  return (ambient + (diffuse + specular) * intensity) * attenuation;
}

vec4 directionLight(Light light) {
  vec4 ambient = vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));
  vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(norm, lightDir), 0.0);
  vec4 diffuse =
      vec4(light.diffuse, 1.0) * diff * getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular =
      vec4(light.specular, 1.0) * spec * getMaterialSpecularColor(material, TexCoords);
  return ambient + diffuse + specular;
}

void main() {
  vec4 color = vec4(0.);
  for (int i = 0; i < lightCount; i++) {
    if (lights[i].type == 0)
      color += pointLight(lights[i]);
    if (lights[i].type == 1)
      color += directionLight(lights[i]);
    if (lights[i].type == 2)
      color += spotLight(lights[i]);
  }
  FragColor = color;
}