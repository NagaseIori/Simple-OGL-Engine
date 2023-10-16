#version 450 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 view;
uniform mat4 projection;

uniform vec2 windowSize;
uniform int kernelSize = 64;
uniform float radius = 0.5;

void main() {
  // vec2 noiseScale = vec2(1920.0/4.0, 1080.0/4.0);
  vec2 noiseScale = windowSize / textureSize(texNoise, 0);
  vec3 fragPos = texture(gPosition, TexCoords).xyz;
  fragPos = vec3(view * vec4(fragPos, 1.0));
  vec3 normal = texture(gNormal, TexCoords).rgb;
  normal = transpose(inverse(mat3(view))) * normal;
  vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
  vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
  vec3 bitangent = cross(normal, tangent);
  mat3 TBN = mat3(tangent, bitangent, normal);

  float occlusion = 0.0;
  for (int i = 0; i < kernelSize; i++) {
    // get sample position
    vec3 samplePos = TBN * samples[i]; // from tangent to view-space
    samplePos = fragPos + samplePos * radius;

    vec4 offset = vec4(samplePos, 1.0);
    offset = projection * offset;        // from view to clip-space
    offset.xyz /= offset.w;              // perspective divide
    offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
    vec3 samplePos2 = texture(gPosition, offset.xy).xyz;
    samplePos2 = vec3(view * vec4(samplePos2, 1.0));
    float sampleDepth = samplePos2.z;
    float bias = 0.025;
    float rangeCheck =
        smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
    occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
  }
  occlusion = 1.0 - (occlusion / kernelSize);
  FragColor = occlusion;
}