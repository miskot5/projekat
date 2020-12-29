//
// modified by miodrag on 26.12.20..
//

#ifndef INC_2_MODEL_LIGHTING_FS_DAYPROP_H
#define INC_2_MODEL_LIGHTING_FS_DAYPROP_H

#include <glm/glm.hpp>

class DayProp{
public:
    float angle;
    float radians;
    float light_power;
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 specular;
    bool active;
    glm::vec3 sky_color;

    void calc_day_properties(float angle) {
        this->angle = angle;
        radians = glm::radians(angle);
        light_power = max(sin(radians), 0.0f);
        if(light_power == 0.0f)
            active=false;
        else
            active=true;
        position = glm::vec3(5.0f, sin(radians) * 10.0f - 2.0f, -cos(radians) * 12.0f);
        color = glm::vec3(light_power * 0.2 + 0.8, light_power * 0.85 + 0.15, light_power * 0.3) *
                light_power;
        specular = glm::vec3(light_power * 0.2 + 0.8, light_power * 0.85 + 0.15, light_power * 0.3);
        if(active)
            sky_color = glm::vec3(min(light_power  + 0.3f, 0.5f), min(light_power, 0.5f), light_power) * light_power;
        else
            sky_color = glm::vec3(0.0f);
    }

    void calc_night_properties(float angle) {
        this->angle = angle;
        radians = glm::radians(angle);
        light_power = max(sin(radians), 0.0f);
        if(light_power == 0.0f)
            active = false;
        else
            active = true;
        position = glm::vec3(5.0f, sin(radians) * 7.0f - 1.0f, -cos(radians) * 8.0f);
        color = glm::vec3(light_power*0.8, light_power*0.8, light_power) * light_power;
        specular = color;
        sky_color=glm::vec3(0.0f);
    }
};

#endif //INC_2_MODEL_LIGHTING_FS_DAYPROP_H

