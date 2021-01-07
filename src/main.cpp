#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>
#include <learnopengl/camera.h>
#include <rg/DayProp.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int load_cubemap(vector<std::string> faces);
void calculate_day(float angle);
void calculate_night(float angle);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f ;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool firstMouse_box = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float power;
    float constant;
    float linear;
    float quadratic;
};

DayProp sun_prop;
DayProp moon_prop;

struct ProgramState{
    bool ImGuiEnabled=false;
    float SunScale=0.05f;
    float SunSpeed=1.0f;
    bool SunSpeedCheck=false;
    void LoadFromDisk(string path);
    void SaveToDisk(string path);

    Camera camera;

    ProgramState()
            : camera(glm::vec3(0.0f, 1.0f, 10.0f)){}
};

void ProgramState::LoadFromDisk(string path){
    ifstream in(path);
    in>>ImGuiEnabled
    >>SunSpeed
    >>camera.Position.x
    >>camera.Position.y
    >>camera.Position.z
    >>camera.Pitch
    >>camera.Yaw;
}

void ProgramState::SaveToDisk(string path){
    ofstream out(path);
    out<<ImGuiEnabled<<'\n'
    <<SunSpeed<<'\n'
    <<camera.Position.x<<'\n'
    <<camera.Position.y<<'\n'
    <<camera.Position.z<<'\n'
    <<camera.Pitch<<'\n'
    <<camera.Yaw<<'\n';
}

ProgramState* programState;
void DrawImGui(ProgramState* programState);

float moon_rotate=0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window,key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io=ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


    glEnable(GL_DEPTH_TEST);

    programState=new ProgramState();

    programState->LoadFromDisk("resources/programState.txt");
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(programState->ImGuiEnabled)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    Shader church_shader("church_vertex.vs", "church_fragment.fs");
    Model church_model(FileSystem::getPath("resources/objects/church/aberkios_100k_texture.obj"));

    Shader sun_shader("sun_vertex.vs", "sun_fragment.fs");
    Model sun_model(FileSystem::getPath("resources/objects/planet/planet.obj"));

    Shader moon_shader("moon_vertex.vs", "moon_fragment.fs");
    Model moon_model(FileSystem::getPath("resources/objects/moon/planet.obj"));

    sun_model.SetShaderTextureNamePrefix("material.");
    moon_model.SetShaderTextureNamePrefix("material.");

    Shader skybox_shader("skybox_vertex.vs", "skybox_fragment.fs");

    Shader grass_shader("grass_vertex.vs", "grass_fragment.fs");

    float vertices[] = {
            // positions          // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // top right
            0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
            -0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float planeVertices[] = {
            // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
            5.0f, -0.5f,  5.0f,  1.0f, 0.0f,
            -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,

            5.0f, -0.5f,  5.0f,  1.0f, 0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,
            5.0f, -0.5f, -5.0f,  1.0f, 1.0f
    };

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/grass_circle.png").c_str());

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
    };

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };

    unsigned int cubemap_texture = load_cubemap(faces);

    skybox_shader.use();
    skybox_shader.setInt("skybox", 0);

    DirLight sun_light;
    sun_light.direction = glm::vec3(4.0f, 40.f, 0.0f);
    sun_light.ambient = glm::vec3(0.5f);
    sun_light.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    sun_light.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    DirLight moon_light;
    moon_light.direction = glm::vec3(4.0f, 40.f, 0.0f);
    moon_light.ambient = glm::vec3(0.1f, 0.1f, 0.15f);
    moon_light.diffuse = glm::vec3(0.02f, 0.02f, 0.1f);
    moon_light.specular = glm::vec3(0.05f, 0.05f, 0.3f);

    PointLight pointLight;
    pointLight.position=glm::vec3 (0.5,0.7,0.5);
    pointLight.ambient=glm::vec3(0.2f,0.2f,0.2f);
    pointLight.diffuse=glm::vec3(0.2f,0.2f,0.0f);
    pointLight.specular=glm::vec3(1.0f,1.0f,0.0f);
    pointLight.power=0.0f;
    pointLight.constant=0.5f;
    pointLight.linear=0.01f;
    pointLight.quadratic=0.2f;

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    float degrees=0.00f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        processInput(window);

        glClearColor(sun_prop.sky_color.x, sun_prop.sky_color.y, sun_prop.sky_color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        degrees+=0.5f * programState->SunSpeed;
        moon_rotate =abs(degrees - 0.5f*programState->SunSpeed);

        calculate_day(degrees);
        calculate_night(degrees+180);
        if(programState->SunSpeedCheck)
            programState->SunSpeed = 1.0f;

        church_shader.use();

        if(sun_prop.active) {
            church_shader.setVec3("light.direction", sun_prop.position);
            church_shader.setVec3("viewPosition", programState->camera.Position);
            church_shader.setVec3("light.ambient", sun_light.ambient);
            church_shader.setVec3("light.diffuse", sun_light.diffuse);
            church_shader.setVec3("light.specular", sun_prop.specular);
            church_shader.setFloat("material.shininess", 0.5f);
            church_shader.setFloat("light.power", sun_prop.light_power);
        }

        if(moon_prop.active) {
            church_shader.setVec3("light.direction", moon_prop.position);
            church_shader.setVec3("viewPosition", programState->camera.Position);
            church_shader.setVec3("light.ambient", moon_light.ambient);
            church_shader.setVec3("light.diffuse", moon_light.diffuse);
            church_shader.setVec3("light.specular", moon_prop.specular);
            church_shader.setFloat("material.shininess", 0.1f);
            church_shader.setFloat("light.power", moon_prop.light_power);
        }

        church_shader.setVec3("pointLight.position",pointLight.position);
        church_shader.setVec3("pointLight.ambient", pointLight.ambient);
        church_shader.setVec3("pointLight.diffuse",pointLight.diffuse);
        church_shader.setFloat("pointLight.power",moon_prop.light_power);
        church_shader.setVec3("pointLight.specular",pointLight.specular);
        church_shader.setFloat("pointLight.constant", pointLight.constant);
        church_shader.setFloat("pointLight.linear",pointLight.linear);
        church_shader.setFloat("pointLight.quadratic",pointLight.quadratic * (sin(moon_rotate*0.5)/4+0.5));

        church_shader.setMat4("projection", projection);
        church_shader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model,glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.2f));	// it's a bit too big for our scene, so scale it down
        church_shader.setMat4("model", model);

        church_model.Draw(church_shader);

        if(sun_prop.active) {
            sun_shader.use();

            sun_shader.setMat4("projection", projection);
            sun_shader.setMat4("view", view);
            sun_shader.setVec3("sun_color", sun_prop.color);

            model = glm::mat4(1.0f);
            model = glm::translate(model, sun_prop.position);
            model = glm::scale(model, glm::vec3(0.13f));    // it's a bit too big for our scene, so scale it down
            sun_shader.setMat4("model", model);
            sun_model.Draw(sun_shader);
        }

        if(moon_prop.active) {
            moon_shader.use();

            moon_shader.setMat4("projection", projection);
            moon_shader.setMat4("view", view);
            moon_shader.setVec3("moon_color", moon_prop.color);

            model = glm::mat4(1.0f);
            model = glm::translate(model, moon_prop.position);
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, moon_rotate/20.0f, glm::vec3(-1.0f, -1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.18f));    // it's a bit too big for our scene, so scale it down
            moon_shader.setMat4("model", model);
            moon_model.Draw(moon_shader);

        }

        grass_shader.use();
        // floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.1f, 0.0f));
        model = glm::scale(model, glm::vec3(2.6f));
        grass_shader.setMat4("model", model);
        grass_shader.setMat4("projection", projection);
        grass_shader.setMat4("view", view);
        if(sun_prop.active)
            grass_shader.setFloat("power", sun_prop.light_power * 0.65f);
        else
            grass_shader.setFloat("power", moon_prop.light_power * 0.1f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if(!sun_prop.active) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            skybox_shader.use();

            model = glm::mat4(1.0f);
            model = glm::rotate(model, moon_rotate*0.007f, glm::vec3(-0.4f, 1.0f, -0.4f));
            skybox_shader.setMat4("model", model);
            skybox_shader.setMat4("view", glm::mat4(glm::mat3(view)));
            skybox_shader.setMat4("projection", projection);
            skybox_shader.setFloat("power", moon_prop.light_power);

            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        if(programState->ImGuiEnabled)
            DrawImGui(programState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToDisk("resources/programState.txt");

    //ImGui cleanup
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    delete programState;

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(programState->ImGuiEnabled==false)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);

}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    programState->camera.ProcessMouseScroll(yoffset);
}

void calculate_day(float angle){
    sun_prop.calc_day_properties(angle);
}

void calculate_night(float angle){
    moon_prop.calc_night_properties(angle);
}

unsigned int load_cubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++){
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else{
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
        }
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_CLAMP_TO_EDGE obaveno
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void DrawImGui(ProgramState* programState){
    //ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Sun properties");
        ImGui::DragFloat("Sun scale",&programState->SunScale,0.01f,0.05f,0.3f);
        ImGui::DragFloat("SunSpeed",&programState->SunSpeed,0.1f,0.0f,2.0f);
        ImGui::Checkbox("Default speed",&programState->SunSpeedCheck);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        ImGui::Text("Camera position: (%f, %f, %f)",programState->camera.Position.x,programState->camera.Position.y,programState->camera.Position.z);
        ImGui::End();
    }

    //ImGui render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key==GLFW_KEY_Q && action==GLFW_PRESS){
        programState->ImGuiEnabled=!programState->ImGuiEnabled;
        if(programState->ImGuiEnabled){
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else{
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
