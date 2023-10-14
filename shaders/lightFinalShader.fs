#version 450 core
#define LIGHT_MAX_COUNT 16
#define pow2(x) (x * x)
const highp float pi = 3.1415926535 * 2.;

in vec2 TexCoords;
out vec4 FragCol;

uniform sampler2D gAlbedo;
uniform sampler2D gSpec;
uniform sampler2D gLightAlbedo;
uniform sampler2D gLightSpec;


void main() {
  vec4 albedo = texture(gLightAlbedo, TexCoords) * texture(gAlbedo, TexCoords);
  vec4 spec = texture(gLightSpec, TexCoords) * texture(gSpec, TexCoords);
  FragCol = albedo + spec;
}