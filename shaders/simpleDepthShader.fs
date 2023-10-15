#version 450 core
#define MATERIAL_MAX_COUNT 32
in vec3 Normal;
in vec2 TexCoords;
uniform vec3 lightDir;

struct Material {
  sampler2D diffuse[MATERIAL_MAX_COUNT];
  int diffuse_c;
};

uniform Material material;

void main() {
  float alpha = 0.;
  for (int i = 0; i < material.diffuse_c; i++)
    alpha += texture(material.diffuse[i], TexCoords).a;
  if (alpha < 0.1)
    discard;
  gl_FragDepth = gl_FragCoord.z;
  //   highp float bias = max(0.0001 * (1.0 - dot(Normal, lightDir)), 0.0005);
  //   gl_FragDepth += gl_FrontFacing ? bias : 0.0;
}