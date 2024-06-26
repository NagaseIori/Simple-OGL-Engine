#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec2 aTexCoord;
  
out vec4 vertexColor; // specify a color output to the fragment shader
out vec4 vertexPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0); 
    vertexColor = vec4(aCol, 1.0); 
    vertexPos = gl_Position;
    TexCoord = aTexCoord;
}