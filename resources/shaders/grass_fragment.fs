#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;
uniform float power;

void main()
{
    vec4 texColor = texture(texture1, TexCoords);
       if(texColor.a < 0.5)
        discard;
    FragColor = texColor * power;
}