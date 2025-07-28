#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

void main() {
    vec4 worldPos = uModel * vec4(aPos,1.0);
    FragPos = worldPos.xyz;
    Normal  = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aUV;
    gl_Position = uProj * uView * worldPos;
}
