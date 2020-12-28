#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 moon_color;
uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1,TextCoords)*vec4(moon_color, 1.0f);
}