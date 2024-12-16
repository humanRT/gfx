
#include "gizmo.hpp"

const char* Gizmo::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* Gizmo::fragmentShaderSource = R"(
#version 330 core

in vec3 FragPos;  // Position of the fragment
in vec3 Normal;   // Normal at the fragment

out vec4 FragColor;

uniform vec3 lightPos;    // Position of the light
uniform vec3 lightColor;  // Color of the light
uniform vec3 objectColor; // Color of the object
uniform vec3 viewPos;     // Position of the viewer/camera

// Attenuation factors
uniform float constant;
uniform float linear;
uniform float quadratic;

void main()
{
    // Normalize the normal vector
    vec3 norm = normalize(Normal);

    // Calculate light direction
    vec3 lightDir = normalize(lightPos - FragPos);

    // Calculate view direction
    vec3 viewDir = normalize(viewPos - FragPos);

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess factor
    vec3 specular = spec * lightColor;

    // Ambient lighting
    vec3 ambient = 0.1 * lightColor; // Adjust the ambient intensity

    // Calculate attenuation
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // Combine all lighting components with attenuation
    vec3 result = (ambient + diffuse + specular) * objectColor * attenuation;

    FragColor = vec4(result, 1.0);
}
)";

const char* Gizmo::wireframeShaderSource = R"(
#version 330 core
uniform vec3 wireframeColor; // Add this uniform
out vec4 FragColor;

void main() {
    FragColor = vec4(wireframeColor, 1.0); // Use wireframe color
}
)";

const char* Gizmo::lineFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 lineColor;

void main() {
    FragColor = vec4(lineColor, 1.0);
}
)";

const char* Gizmo::lineShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 mvp; // Model-View-Projection matrix

void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

Gizmo::Gizmo()
{
    glClearColor(0.2f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    float FOV = 45.0f;
    float zNear = 1.0f;
    float zFar = 100.0f;

    persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
}

Gizmo::~Gizmo()
{
    delete pMesh;
    delete pCamera;
}

GLuint Gizmo::compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint Gizmo::createShaderProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint Gizmo::createWireframeShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource); // Use existing vertex shader
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, wireframeShaderSource); // Use wireframe fragment shader

    GLuint wireframeProgram = glCreateProgram();
    glAttachShader(wireframeProgram, vertexShader);
    glAttachShader(wireframeProgram, fragmentShader);
    glLinkProgram(wireframeProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(wireframeProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(wireframeProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return wireframeProgram;
}

GLuint Gizmo::createSimpleShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, lineShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, lineFragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void Gizmo::drawLightLine(const glm::vec3& lightPos, const glm::vec3& lightTarget, const glm::mat4& mvp, GLuint shaderProgram) {
    // Define the line vertices (start and end points)
    glDisable(GL_DEPTH_TEST);
    glm::vec3 lineVertices[] = { lightPos, lightTarget };

    // Create VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind and buffer data
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Use the shader program
    glUseProgram(shaderProgram);

    // Pass uniforms
    GLuint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    GLuint colorLoc = glGetUniformLocation(shaderProgram, "lineColor");
    glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // Yellow line

    // Draw the line
    glDrawArrays(GL_LINES, 0, 2);

    // Cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glUseProgram(0); // Reset shader program
    glEnable(GL_DEPTH_TEST);
}

int Gizmo::init()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);     

    pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "HRT 2024", nullptr, nullptr);
    if (pWindow == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetWindowUserPointer(pWindow, this);
    glfwMakeContextCurrent(pWindow);

    setCallbacks(pWindow);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    m_shaderProgram = createShaderProgram();
    m_wireframeProgram = createWireframeShaderProgram();
    m_lightProgram = createSimpleShaderProgram();

    pCamera = new Camera(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    pCamera->setWindow(pWindow);

    glEnable(GL_MULTISAMPLE);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glLineWidth(2.0f);
    
    return 0;
}

bool Gizmo::loadModel(const std::string& filePath)
{
    pMesh = new Mesh();
    if (pMesh->loadMesh(filePath))
    {
        std::string title = "Failed to load mesh: " + filePath;
        std::cout << "\033[31m" << title << "\033[0m" << std::endl;
        return false;
    }

    return true;
}

void Gizmo::setCallbacks(GLFWwindow* window)
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::string title = "Error: " + std::to_string(error) + " - " + description;
        std::cout << "\033[31m" << title << "\033[0m" << std::endl;
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        Gizmo* gizmo = static_cast<Gizmo*>(glfwGetWindowUserPointer(window));
        gizmo->cbFramebufferSize(window, width, height);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        Gizmo* gizmo = static_cast<Gizmo*>(glfwGetWindowUserPointer(window));
        gizmo->cbKeyboard(window, key, scancode, action, mods);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        Gizmo* gizmo = static_cast<Gizmo*>(glfwGetWindowUserPointer(window));
         if (!gizmo) {
            std::cerr << "Error: gizmo is null in scroll callback." << std::endl;
            return;
        }
        gizmo->cbMouseMotion(window, xpos, ypos);
    });
   
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        Gizmo* gizmo = static_cast<Gizmo*>(glfwGetWindowUserPointer(window));
        gizmo->cbScroll(window, xoffset, yoffset);
    });
}

void Gizmo::cbFramebufferSize(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void Gizmo::cbKeyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void Gizmo::cbMouseMotion(GLFWwindow* /*window*/, double xpos, double ypos)
{
}

void Gizmo::cbScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    pCamera->scrollCallback(window, xoffset, yoffset);
}

void Gizmo::run()
{
    float lightAngle = 0.0f; // Initial angle for light rotation
    const float lightRadius = 4.0f; // Distance from the origin
    const float rotationSpeed = 0.025f; // Speed of rotation (radians per frame)
    glm::vec3 lightTarget = glm::vec3(0.0f, 1.0f, 0.0f); // Target for the light

    while (!glfwWindowShouldClose(pWindow)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pCamera->update();
        view = pCamera->getViewMatrix();
        projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

        // Calculate light position
        float lightX = lightRadius * cos(lightAngle);
        float lightY = lightRadius * sin(lightAngle);
        glm::vec3 lightPos = glm::vec3(lightX, 1.0f, lightY); // Z-axis rotation

        // Update light angle
        lightAngle += rotationSpeed;
        if (lightAngle > 2.0f * glm::pi<float>()) {
            lightAngle -= 2.0f * glm::pi<float>();
        }

        glUseProgram(m_shaderProgram);
        GLuint colorLoc = glGetUniformLocation(m_shaderProgram, "wireframeColor");
        glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // Set to red

        // Set light properties
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos)); // Fixed

        glm::vec3 objectColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "objectColor"), 1, glm::value_ptr(objectColor));

        // Set viewer position
        glm::vec3 viewPos = pCamera->getPosition();
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));

        // Set attenuation factors
        glUniform1f(glGetUniformLocation(m_shaderProgram, "constant"), 1.0f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "linear"), 0.09f);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "quadratic"), 0.032f);

        // Render the mesh
        pMesh->render(m_shaderProgram, view, projection);
        // pMesh->drawTriangles(m_wireframeProgram, mvp);
        // drawLightLine(lightPos, lightTarget, mvp, m_lightProgram);

        glfwSwapBuffers(pWindow);
        glfwPollEvents();
    }
}
