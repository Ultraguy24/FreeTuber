#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D uBaseColorTexture;
uniform vec3      uLightDir;
uniform vec3      uAmbient;

void main() {
    vec4 tex = texture(uBaseColorTexture, TexCoord);
    // discard fully transparent pixels
    if (tex.a < 0.1) discard;

    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, normalize(uLightDir)), 0.0);
    vec3 color = tex.rgb * (uAmbient + diff);

    FragColor = vec4(color, tex.a);
}
