#version 450 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BloomColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float exposure;
uniform float threshold;
uniform float bound_ratio;

void main() {
  vec3 color = texture(screenTexture, TexCoords).rgb;
  FragColor = vec4(color, 1.);

  // Caculation for bloom
  float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
  // Transition between bound to threshold
  if (brightness > threshold * bound_ratio)
    BloomColor = vec4(FragColor.rgb, 1.0) *
                 min(smoothstep(0.0, 1.0, brightness - threshold * bound_ratio), 1.0);
  else
    BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}