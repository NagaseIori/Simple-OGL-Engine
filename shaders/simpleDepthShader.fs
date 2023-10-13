#version 330 core
in vec3 Normal;
uniform vec3 lightDir;

void main() {
  gl_FragDepth = gl_FragCoord.z;
//   highp float bias = max(0.0001 * (1.0 - dot(Normal, lightDir)), 0.0005);
//   gl_FragDepth += gl_FrontFacing ? bias : 0.0;
}