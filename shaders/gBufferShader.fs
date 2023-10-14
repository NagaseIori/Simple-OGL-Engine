#version 450 core
#define MATERIAL_MAX_COUNT 32
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

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

void main() {
  vec4 albedo = vec4(0.);
  vec4 spec = vec4(0.);
  vec3 normal = vec3(0.);

  for (int i = 0; i < material.diffuse_c; i++) {
    albedo += texture(material.diffuse[i], TexCoords);
  }
  if(albedo.a < 0.1)
    discard;
  for (int i = 0; i < material.specular_c; i++) {
    spec += texture(material.specular[i], TexCoords);
  }
  for (int i = 0; i < material.normal_c; i++) {
    normal += texture(material.normal[i], TexCoords).rgb;
  }
  normal = normalize(normal+Normal);

  gPosition = vec4(FragPos, 1.);
  gNormal = vec4(normal, 1.);
  gAlbedo = albedo;
  gSpec = vec4(spec.rgb, albedo.a);
}