#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For transformations like perspective, lookAt, etc.
#include <GLFW/glfw3.h>

class Camera
{
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up);
    ~Camera();
    
    glm::mat4 getViewMatrix();
    glm::vec3 getPosition() { return m_position; }
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);
    void processMousePan(float xOffset, float yOffset);
    void resetView();

    void setWindow(GLFWwindow* window);
    void update();

    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* m_window;

    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;
    // glm::vec3 m_right;

    float m_distance;
    float m_yaw;
    float m_pitch;

    bool m_firstMouse;
    float m_lastX, m_lastY;

    void updateCameraVectors();
};

#endif // CAMERA_HPP