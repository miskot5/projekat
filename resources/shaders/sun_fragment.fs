#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 sun_color;

void main()
{
    FragColor = vec4(sun_color, 1.0f);
}