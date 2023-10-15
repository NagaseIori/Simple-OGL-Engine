#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);

  mat3 normalMatrix = transpose(inverse(mat3(model)));
  FragPos = vec3(model * vec4(aPos, 1.0));
  TexCoords = aTexCoords;

  // Caculate TBN Matrix
  vec3 T = normalize(normalMatrix * aTangent);
  vec3 N = normalize(normalMatrix * aNormal);
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);
  TBN = mat3(T, B, N);

  mat3 tTBN = transpose(mat3(T, B, N));
  TangentFragPos = tTBN * FragPos;
  TangentViewPos = tTBN * viewPos;
}