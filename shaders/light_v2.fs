#version 450 core
#define LIGHT_MAX_COUNT 12
#define MATERIAL_MAX_COUNT 8
#define pow2(x) (x * x)
const highp float pi = 3.1415926535 * 2.;

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

  sampler2D shadowMap;
  // samplerCube cubeMap;
  int shadowCast;
  mat4 lightSpace;

  int type; // 0 point 1 direction 2 spot
};

uniform Light lights[LIGHT_MAX_COUNT];

vec4 getMaterialDiffuseColor(Material material, vec2 TexCoords) {
  if (material.diffuse_c == 0)
    return vec4(0.);
  vec4 color = vec4(0.);
  for (int i = 0; i < material.diffuse_c; i++)
    color += texture(material.diffuse[i], TexCoords);
  return color;
}

vec4 getMaterialSpecularColor(Material material, vec2 TexCoords) {
  if (material.specular_c == 0)
    return vec4(0.);
  vec4 color = vec4(0.);
  for (int i = 0; i < material.specular_c; i++)
    color += texture(material.specular[i], TexCoords);
  return color;
}

vec3 getMaterialNormal(Material material, vec2 TexCoords) {
  if (material.normal_c == 0)
    return vec3(0.);
  vec4 color = vec4(0.);
  for (int i = 0; i < material.normal_c; i++)
    color += texture(material.normal[i], TexCoords);
  return vec3(color);
}

float gaussian(vec2 i, float sigma) {
  return 1.0 / (pi * pow2(sigma)) *
         exp(-((pow2(i.x) + pow2(i.y)) / (2.0 * pow2(sigma))));
}

float lightShadowCaculation(Light light, vec3 normal, vec3 lightDir) {
  if (light.shadowCast == 0)
    return 0.0;

  vec4 fragPosLightSpace = light.lightSpace * vec4(FragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;
  float closestDepth = texture(light.shadowMap, projCoords.xy).r;
  float currentDepth = projCoords.z;
  float shadow = 0.;
  float bias = 0.0005;
  int samples = 8;

  vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);

  float weight = 0., accmu = 0.;
  float sigma = 4.;
  for (int x = -samples / 2; x <= samples / 2; ++x) {
    for (int y = -samples / 2; y <= samples / 2; ++y) {
      float pcfDepth =
          texture(light.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
      weight = gaussian(vec2(x, y), sigma);
      // weight = 1;
      shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0) * weight;
      accmu += weight;
    }
  }
  shadow /= accmu;
  return shadow;
}

float pointLightShadowCaculation(Light light, vec3 fragPos) {
  if (light.shadowCast == 0)
    return 0.0;

  // get vector between fragment position and light position
  // vec3 fragToLight = fragPos - light.position;
  // // use the light to fragment vector to sample from the depth map
  // float closestDepth = texture(light.cubeMap, fragToLight).r;
  // // it is currently in linear range between [0,1]. Re-transform back to
  // // original value
  // closestDepth *= light.cutOff;
  // // now get current linear depth as the length between the fragment and light
  // // position
  // float currentDepth = length(fragToLight);
  // // now test for shadows
  // float bias = 0.0005;
  // float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

  // return shadow;
}

vec4 pointLight(Light light) {
  // Attenuation
  float distance = length(light.position - FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));

  // Ambient
  vec4 ambient =
      vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));
  vec3 lightDir = normalize(light.position - FragPos);
  // vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(norm, lightDir), 0.0);
  vec4 diffuse = vec4(light.diffuse, 1.0) * diff *
                 getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular = vec4(light.specular, 1.0) * spec *
                  getMaterialSpecularColor(material, TexCoords);

  // float shadow = pointLightShadowCaculation(light, FragPos);

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
  vec4 ambient =
      vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));

  float diff = max(dot(norm, lightDir), 0.0);
  vec4 diffuse = vec4(light.diffuse, 1.0) * diff *
                 getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular = vec4(light.specular, 1.0) * spec *
                  getMaterialSpecularColor(material, TexCoords);

  float shadow = lightShadowCaculation(light, norm, lightDir);
  return (ambient + (diffuse + specular) * intensity * (1. - shadow)) *
         attenuation;
}

vec4 directionLight(Light light) {
  vec4 ambient =
      vec4(light.ambient, 1.0) * getMaterialDiffuseColor(material, TexCoords);
  // Diffuse
  vec3 norm = normalize(Normal + getMaterialNormal(material, TexCoords));
  vec3 lightDir = normalize(-light.direction);

  float diff = max(dot(lightDir, norm), 0.0);
  vec4 diffuse = vec4(light.diffuse, 1.0) * diff *
                 getMaterialDiffuseColor(material, TexCoords);
  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
  vec4 specular = vec4(light.specular, 1.0) * spec *
                  getMaterialSpecularColor(material, TexCoords);

  float shadow = lightShadowCaculation(light, norm, lightDir);
  return ambient + (diffuse + specular) * (1. - shadow);
  // return vec4(norm, 1.);
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