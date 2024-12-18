#include "camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    : m_window(nullptr), m_position(position), m_target(target), m_up(up),
      m_distance(glm::distance(position, target)), m_yaw(-90.0f), m_pitch(60.0f)
{
    std::cout << "Distance: " << m_distance << std::endl;
    updateCameraVectors();
}

Camera::~Camera()
{
    std::cout << "Destroying camera" << std::endl;
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_position, m_target, m_up);
}

void Camera::updateCameraVectors()
{
    // Spherical to Cartesian conversion
    float x = m_distance * cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
    float y = m_distance * sin(glm::radians(m_pitch));
    float z = m_distance * cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));

    m_position = m_target + glm::vec3(x, y, z);
}

void Camera::scrollCallback(GLFWwindow* window, double, double yoffset)
{
    processMouseScroll(static_cast<float>(yoffset));
}

void Camera::setWindow(GLFWwindow* window)
{
    m_window = window;
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera::resetView()
{
    m_distance = 0.65f;  // Default distance from target
    m_yaw = -90.0f;     // Default horizontal angle
    m_pitch = 60.0f;     // Default vertical angle
    m_target = glm::vec3(0.0f, 0.125f, 0.0f); // Default target
    updateCameraVectors();
}

void Camera::update()
{
    if (!m_window) return;

    // Get the current mouse position
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);

    if (m_firstMouse)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    // Calculate mouse movement offsets
    float xOffset = static_cast<float>(xpos - m_lastX);
    float yOffset = static_cast<float>(m_lastY - ypos); // Reversed: y-coordinates go from bottom to top
    m_lastX = xpos;
    m_lastY = ypos;

    // Handle camera controls
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        // Orbit (rotate around the target)
        processMouseMovement(xOffset, -yOffset);
    }
    else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        // Pan (translate target and camera)
        processMousePan(xOffset, yOffset);
    }

    // Reset first mouse use if the right or middle button is released
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE &&
        glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE)
    {
        m_firstMouse = true;
    }

    // Handle zoom using the scroll callback (already implemented elsewhere)
    // Reset camera on pressing 'R'
    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS)
    {
        resetView();
    }

    // Exit cursor lock mode with Escape
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_firstMouse = true; // Ensure smooth reactivation
    }
}

void Camera::processMouseScroll(float yOffset)
{
    const float zoomSpeed = 0.2f;
    m_distance -= yOffset * zoomSpeed;
    m_distance = glm::clamp(m_distance, 0.1f, 100.0f); // Prevent zooming too close or too far
    updateCameraVectors();
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    const float sensitivity = 0.2f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    m_yaw += xOffset;
    m_pitch += yOffset;

    // Clamp pitch to avoid flipping
    if (constrainPitch)
    {
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::processMousePan(float xOffset, float yOffset)
{
    const float panSpeed = 0.002f;
    glm::vec3 right = glm::normalize(glm::cross(m_target - m_position, m_up));
    glm::vec3 up = -glm::normalize(glm::cross(right, m_target - m_position));

    m_target += -right * xOffset * panSpeed + up * yOffset * panSpeed;
    updateCameraVectors();
}
