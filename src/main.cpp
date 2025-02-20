#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout(location = 0) in vec3 aPos;\n"
                                 "layout(location = 1) in vec2 aTexCoord;\n"
                                 "uniform mat4 model;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "out vec2 TexCoord;\n"
                                 "void main() {\n"
                                 "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                                 "    TexCoord = aTexCoord;\n"
                                 "}\n";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "in vec2 TexCoord;\n"
                                   "uniform sampler2D texture1;\n"
                                   "void main() {\n"
                                   "    FragColor = texture(texture1, TexCoord);\n"
                                   "}\n";

struct Model
{
    unsigned int VAO, VBO, EBO;
    std::vector<unsigned int> indices;
};

struct Renderable
{
    Model model;
    unsigned int texture;
};

unsigned int createShader(unsigned int type, const char *source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    return shader;
}

unsigned int loadTexture(const char *path)
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

Model loadOBJ(const char *path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    Model model;
    std::vector<float> vertices;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
    {
        std::cerr << "Failed to load OBJ file: " << warn << err << std::endl;
        return model;
    }

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
            if (index.texcoord_index >= 0)
            {
                vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
            }
            else
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
            model.indices.push_back(static_cast<unsigned int>(model.indices.size()));
        }
    }

    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);

    glBindVertexArray(model.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return model;
}

Renderable loadRenderable(const char *objPath, const char *texturePath)
{
    Renderable renderable;
    renderable.model = loadOBJ(objPath);
    renderable.texture = loadTexture(texturePath);
    return renderable;
}

void renderObj(unsigned int modelLoc, const Renderable &renderable,
               glm::vec3 position, glm::vec3 rotation, glm::vec3 size)
{
    glm::mat4 objModel = glm::translate(glm::mat4(1.0f), position);
    objModel = glm::rotate(objModel, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    objModel = glm::rotate(objModel, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    objModel = glm::rotate(objModel, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    objModel = glm::scale(objModel, size);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(objModel));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderable.texture);

    glBindVertexArray(renderable.model.VAO);
    glDrawElements(GL_TRIANGLES, renderable.model.indices.size(), GL_UNSIGNED_INT, 0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Rendering Test", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    unsigned int vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    stbi_set_flip_vertically_on_load(true);

    glfwSwapInterval(0);

    Renderable cubeRenderable = loadRenderable("assets/cube.obj", "assets/concrete.png");
    Renderable brickRenderable = loadRenderable("assets/cube.obj", "assets/brick.png");
    Renderable duckRenderable = loadRenderable("assets/duck.obj", "assets/duck.jpg");

    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        renderObj(modelLoc, cubeRenderable, glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(20.0f, 20.0f, 0.1f));

        renderObj(modelLoc, brickRenderable, glm::vec3(-4.0f, -1.0f, -10.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(1.0f, 14.0f, 5.0f));

        float duckX = sin(glfwGetTime() * 0.5f) * 5.0f;
        renderObj(modelLoc, duckRenderable, glm::vec3(duckX, -2.0f, 0.0f), glm::vec3(0.0f, 80.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();

        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0)
        {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    glfwTerminate();
    return 0;
}