#include "grid.hpp"

namespace Grid
{
    const char* VertexShader = R"(
    #version 330 core

    uniform mat4 gVP;
    uniform float gGridSize;
    uniform vec3 gCameraWorldPos;

    out vec3 WorldPos;

    const vec3 Pos[4] = vec3[4](
        vec3(-1.0, 0.0, -1.0),  // bottom-left
        vec3( 1.0, 0.0, -1.0),  // bottom-right
        vec3( 1.0, 0.0,  1.0),  // top-right
        vec3(-1.0, 0.0,  1.0)   // top-left
    );

    const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

    void main()
    {
        int Index = Indices[gl_VertexID];
        vec3 vPos3 = Pos[Index] * gGridSize;

        // Position the quad around the camera for testing
        vPos3.x += gCameraWorldPos.x;
        vPos3.z += gCameraWorldPos.z;

        gl_Position = gVP * vec4(vPos3, 1.0);
        WorldPos = vPos3;
    }
    )";

    const char* FragmentShader = R"(
    #version 330 core

    in vec3 WorldPos;

    layout(location = 0) out vec4 FragColor;

    uniform vec3 gCameraWorldPos;
    uniform float gGridSize = 100.0;
    uniform float gGridMinPixelsBetweenCells = 2.0;
    uniform float gGridCellSize = 0.025;
    uniform vec4 gGridColorThin = vec4(0.5, 0.5, 0.5, 1.0);
    uniform vec4 gGridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

    float log10(float x) {
        return log(x) / log(10.0);
    }

    float satf(float x) {
        return clamp(x, 0.0, 1.0);
    }

    vec2 satv(vec2 x) {
        return clamp(x, vec2(0.0), vec2(1.0));
    }

    float max2(vec2 v) {
        return max(v.x, v.y);
    }

    void main()
    {
        // Compute screen-space derivatives to determine LOD
        vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x));
        vec2 dvy = vec2(dFdx(WorldPos.z), dFdy(WorldPos.z));
        float lx = length(dvx);
        float ly = length(dvy);
        vec2 dudv = vec2(lx, ly);

        float l = length(dudv);

        float LOD = max(0.0, log10(l * gGridMinPixelsBetweenCells / gGridCellSize) + 1.0);
        float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
        float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
        float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;

        // Increase sample frequency to reduce aliasing
        dudv *= 4.0;

        // For each LOD level, determine if we're on a line
        vec2 mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod0) / dudv;
        float Lod0a = max2(vec2(1.0) - abs(satv(mod_div_dudv)*2.0 - vec2(1.0)));

        mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod1) / dudv;
        float Lod1a = max2(vec2(1.0) - abs(satv(mod_div_dudv)*2.0 - vec2(1.0)));

        mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod2) / dudv;
        float Lod2a = max2(vec2(1.0) - abs(satv(mod_div_dudv)*2.0 - vec2(1.0)));

        float LOD_fade = fract(LOD);

        vec4 Color;
        float lineAlpha = 0.0;

        // Determine which lines to draw
        if (Lod2a > 0.0) {
            // Thick lines
            Color = gGridColorThick;
            lineAlpha = Lod2a;
        } else if (Lod1a > 0.0) {
            // Medium lines (blend thick and thin based on LOD)
            Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
            lineAlpha = Lod1a;
        } else if (Lod0a > 0.0) {
            // Thin lines
            Color = gGridColorThin;
            lineAlpha = Lod0a * (1.0 - LOD_fade);
        } else {
            // Not on a line, make transparent
            Color = vec4(0.0, 0.0, 0.0, 0.0);
            lineAlpha = 0.0;
        }

        // Optional distance-based fadeout
        float OpacityFalloff = (1.0 - satf(length(WorldPos.xz - gCameraWorldPos.xz) / gGridSize));
        Color.a = lineAlpha * OpacityFalloff;

        FragColor = Color;
    }
    )";

    InfiniteGridConfig config;
    GLuint m_gridProgram = -1;

    GLuint compileShader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        return shader;
    }

    GLuint createGridProgram()
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShader);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShader);

        GLuint m_gridProgram = glCreateProgram();
        glAttachShader(m_gridProgram, vertexShader);
        glAttachShader(m_gridProgram, fragmentShader);
        glLinkProgram(m_gridProgram);

        GLint success;
        glGetProgramiv(m_gridProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_gridProgram, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return m_gridProgram;
    }

    void renderGrid(const GridMatrices gMats, const Vector3f& cameraPosition)
    {
        GLuint VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        
        if (m_gridProgram == -1) {
            m_gridProgram = createGridProgram();
        }
        
        glUseProgram(m_gridProgram);

        glm::mat4 gVP = gMats.Projection * gMats.View;
        GLint gVPUniform = glGetUniformLocation(m_gridProgram, "gVP");
        glUniformMatrix4fv(gVPUniform, 1, GL_FALSE, glm::value_ptr(gVP));

        // Set uniforms
        GLint gridSizeUniform = glGetUniformLocation(m_gridProgram, "gGridSize");
        glUniform1f(gridSizeUniform, config.Size);

        GLint gridCellSizeUniform = glGetUniformLocation(m_gridProgram, "gGridCellSize");
        glUniform1f(gridCellSizeUniform, config.CellSize);

        GLint cameraPosUniform = glGetUniformLocation(m_gridProgram, "gCameraWorldPos");
        glUniform3f(cameraPosUniform, cameraPosition.x, cameraPosition.y, cameraPosition.z);

        GLint colorThinUniform = glGetUniformLocation(m_gridProgram, "gGridColorThin");
        glUniform4f(colorThinUniform, config.ColorThin.x, config.ColorThin.y, config.ColorThin.z, config.ColorThin.w);

        GLint colorThickUniform = glGetUniformLocation(m_gridProgram, "gGridColorThick");
        glUniform4f(colorThickUniform, config.ColorThick.x, config.ColorThick.y, config.ColorThick.z, config.ColorThick.w);

        GLint minPixelsUniform = glGetUniformLocation(m_gridProgram, "gGridMinPixelsBetweenCells");
        glUniform1f(minPixelsUniform, config.MinPixelsBetweenCells);

        // Render using glDrawArrays
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisable(GL_BLEND);

        // Unbind the shader program
        glUseProgram(0);
    }
};