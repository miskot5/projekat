#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float power;

void main(){
    FragColor = texture(skybox, TexCoords)*power;
}