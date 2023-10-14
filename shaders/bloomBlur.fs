#version 450 core
#define pow2(x) (x * x)
const float pi = 3.1415926535 * 2.;
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform float sigma = 4.0;
uniform int samples = 10;
uniform float scale = 1.0;

float gaussian(float i) {
  return 1.0 / sqrt(pi * pow2(sigma)) * exp(-pow2(i) / (2.0 * pow2(sigma)));
}

void main() {
  vec2 tex_offset = 1.0 / textureSize(image, 0) * scale; // gets size of single texel
  float weight = gaussian(0.);
  vec3 result = texture(image, TexCoords).rgb * weight;
  vec2 blurDir = vec2(0.0, 1.0);
  float accmu = weight;
  if (horizontal)
    blurDir = 1.0 - blurDir;
  for (int i = 1; i < samples / 2; ++i) {
    weight = gaussian(i);
    result +=
        texture(image, TexCoords + tex_offset * i * blurDir).rgb * weight;
    result +=
        texture(image, TexCoords - tex_offset * i * blurDir).rgb * weight;
    accmu += weight * 2.;
  }
  result /= accmu;
  FragColor = vec4(result, 1.0);
}