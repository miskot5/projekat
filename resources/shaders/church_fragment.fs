#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

struct DirLight {
    vec3 direction;
    float power;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform DirLight light;
uniform vec3 viewPosition;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main() {
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcDirLight(light, normal, viewDir);
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient = vec3(max(light.ambient.x*light.power, 0.1f), max(light.ambient.y*light.power, 0.1f), max(light.ambient.z*light.power, 0.3f)) *
            vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    return ambient + (diffuse + specular)*light.power;
    //return (ambient + diffuse);
}


