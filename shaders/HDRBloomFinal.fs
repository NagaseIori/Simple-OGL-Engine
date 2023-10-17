#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform float bloomStrength;
uniform int tonemapStyle;

vec3 GenshinTonemap(vec3 color) {
  return (1.36 * color + 0.047) * color /
         ((0.93 * color + 0.56) * color + 0.14);
}

void main() {
  const float gamma = 2.2;
  vec3 hdrColor = texture(scene, TexCoords).rgb;
  vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
  hdrColor += bloomColor * bloomStrength; // additive blending
  // tone mapping
  vec3 result;
  if (tonemapStyle == 0) {
    // Default
    result = vec3(1.0) - exp(-hdrColor * exposure);
  } else if (tonemapStyle == 1) {
    // Genshin
    result = GenshinTonemap(hdrColor * exposure);
  }
  // also gamma correct while we're at it
  result = pow(result, vec3(1.0 / gamma));
  FragColor = vec4(result, 1.0);
}