#version 450 core
in vec4 vertexColor;
in vec4 vertexPos;
in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 ourColor; // we set this variable in the OpenGL code.
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float visibility;

void main() {
  // FragColor = texture(texture1, TexCoord) * vec4(TexCoord.xy, 1., 1.);
  FragColor =
      mix(texture(texture1, TexCoord),
          texture(texture2, vec2(1. - TexCoord.x, TexCoord.y) * 2. + vec2(.5)),
          visibility);
}