#ifndef GRID_HPP
#define GRID_HPP

#include <iostream>
#include <string>

#include <GL/glew.h>

#include "math3d.hpp"

namespace Grid
{
    struct InfiniteGridConfig {
        bool Enabled = true;
        float Size = 10.0f;
        float CellSize = 0.1f;
        Vector4f ColorThin = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
        Vector4f ColorThick = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        float MinPixelsBetweenCells = 2.0f;
    };

    struct GridMatrices {
        glm::mat4 View;
        glm::mat4 Projection;

        GridMatrices(const glm::mat4& view, const glm::mat4& projection)
            : View(view), Projection(projection) {}
    };

    extern const char* VertexShader;
    extern const char* FragmentShader;

    extern InfiniteGridConfig config;
    extern GLuint m_gridProgram;

    GLuint compileShader(GLenum type, const char* source);
    GLuint createShaderProgram();
    void renderGrid(const GridMatrices mats, const Vector3f& cameraPosition);
};

#endif // GRID_HPP