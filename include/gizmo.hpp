#ifndef GIZMO_HPP
#define GIZMO_HPP

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <fcl/fcl.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For transformations like perspective, lookAt, etc.
#include <glm/gtc/type_ptr.hpp> // To convert matrices to arrays
#include <glm/gtx/string_cast.hpp> // To print vectors and matrices

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "camera.hpp"
#include "grid.hpp"
#include "mesh.hpp"
#include "math3d.hpp"
#include "utils.hpp"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1920

class Gizmo : public std::enable_shared_from_this<Gizmo> {
public:
    Gizmo();
    ~Gizmo();

    int init();
    bool loadModel(const std::string& filePath);
    void setCallbacks(GLFWwindow* window);
    void run(int runForSeconds);

    void cbError();
    void cbFramebufferSize(GLFWwindow* window, int width, int height);
    void cbKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
    void cbMouseMotion(GLFWwindow* window, double x, double y);
    void cbRenderCB();
    void cbScroll(GLFWwindow* window, double xoffset, double yoffset);
    void cbSpecialKeyboard(int key, int mouse_x, int mouse_y);
    void cbTimer(int interval);
    
private:
    GLuint compileShader(GLenum type, const char* source);
    GLuint createShaderProgram();
    GLuint createWireframeShaderProgram();
    GLuint createSimpleShaderProgram();
    void drawLightLine(const glm::vec3& lightPos, const glm::vec3& lightTarget, const glm::mat4& mvp, GLuint shaderProgram);

private:
    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;
    static const char* wireframeShaderSource;
    static const char* lineShaderSource;
    static const char* lineFragmentShader;

    bool tick = false;
    bool toggle = false;
    bool runIndifinitely = false;
    glm::mat4 mvp, model, view, projection;
    GLuint m_shaderProgram;
    GLuint m_wireframeProgram;
    GLuint m_lightProgram;
    GLFWwindow *pWindow;
    GLuint MVPLocation;
    Mesh *pMesh = NULL;
    Camera *pCamera = NULL;
    PersProjInfo persProjInfo;
};

#endif // GIZMO_HPP
