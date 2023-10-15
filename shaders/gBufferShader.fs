#version 450 core
#define MATERIAL_MAX_COUNT 32
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gSpec;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

struct Material {
  sampler2D diffuse[MATERIAL_MAX_COUNT];
  sampler2D specular[MATERIAL_MAX_COUNT];
  sampler2D normal[MATERIAL_MAX_COUNT];
  sampler2D height[MATERIAL_MAX_COUNT];
  int diffuse_c;
  int specular_c;
  int normal_c;
  int height_c;
};

uniform Material material;
uniform float height_scale = 0.05;

float getHeightAt(vec2 texCoords) {
  float height = 0.;
  for (int i = 0; i < material.height_c; i++) {
    height += texture(material.height[i], texCoords).r;
  }
  return height;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
  if (material.height_c == 0)
    return texCoords;
  // number of depth layers
  const float minLayers = 8;
  const float maxLayers = 32;
  float numLayers =
      mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
  // calculate the size of each layer
  float layerDepth = 1.0 / numLayers;
  // depth of current layer
  float currentLayerDepth = 0.0;
  // the amount to shift the texture coordinates per layer (from vector P)
  vec2 P = viewDir.xy / viewDir.z * height_scale;
  vec2 deltaTexCoords = P / numLayers;

  // get initial values
  vec2 currentTexCoords = texCoords;
  float currentDepthMapValue = getHeightAt(currentTexCoords);

  while (currentLayerDepth < currentDepthMapValue) {
    // shift texture coordinates along direction of P
    currentTexCoords -= deltaTexCoords;
    // get depthmap value at current texture coordinates
    currentDepthMapValue = getHeightAt(currentTexCoords);
    // get depth of next layer
    currentLayerDepth += layerDepth;
  }

  // get texture coordinates before collision (reverse operations)
  vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

  // get depth after and before collision for linear interpolation
  float afterDepth = currentDepthMapValue - currentLayerDepth;
  float beforeDepth =
      getHeightAt(prevTexCoords) - currentLayerDepth + layerDepth;

  // interpolation of texture coordinates
  float weight = afterDepth / (afterDepth - beforeDepth);
  vec2 finalTexCoords =
      prevTexCoords * weight + currentTexCoords * (1.0 - weight);

  return finalTexCoords;
}

void main() {
  vec4 albedo = vec4(0.);
  vec4 spec = vec4(0.);
  vec3 normal = vec3(0.);
  vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
  vec2 texCoords = ParallaxMapping(TexCoords, viewDir);

  for (int i = 0; i < material.diffuse_c; i++) {
    albedo += texture(material.diffuse[i], texCoords);
  }
  if (albedo.a < 0.1) {
    discard;
  }
  for (int i = 0; i < material.specular_c; i++) {
    spec += texture(material.specular[i], texCoords);
  }
  for (int i = 0; i < material.normal_c; i++) {
    normal += texture(material.normal[i], texCoords).rgb;
  }
  if (material.normal_c == 0) {
    normal = vec3(0.5, 0.5, 1.);
  }
  normal = 2.0 * normal - 1.0;
  normal = normalize(TBN * normal);

  gPosition = vec4(FragPos, 1.);
  gNormal = vec4(normal, 1.);
  gAlbedo = albedo;
  gSpec = vec4(spec.rgb, albedo.a);
}