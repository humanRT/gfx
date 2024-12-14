#ifndef TEXTURE_H
#define TEXTURE_H

#include <cassert>
#include <cstdint>
#include <string>
#include <unistd.h>

#include <GL/glew.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include "utils.hpp"

class Texture
{
public:
    Texture(GLenum TextureTarget);

    Texture(GLenum TextureTarget, const std::string& FileName);

    // Should be called once to load the texture
    bool Load();

    void Load(unsigned int BufferSize, void* pImageData);

    void Load(const std::string& Filename);

    void LoadRaw(int Width, int Height, int BPP, const unsigned char* pImageData);

    void LoadF32(int Width, int Height, const float* pImageData);

    // Must be called at least once for the specific texture unit
    void Bind(GLenum TextureUnit);

    void GetImageSize(int& ImageWidth, int& ImageHeight)
    {
        ImageWidth = m_imageWidth;
        ImageHeight = m_imageHeight;
    }

    GLuint GetTexture() const { return m_textureObj; }

private:
    void LoadInternal(const void* pImageData);
    void LoadInternalNonDSA(const void* pImageData);
    void LoadInternalDSA(const void* pImageData);    

    void BindInternalNonDSA(GLenum TextureUnit);
    void BindInternalDSA(GLenum TextureUnit);

    std::string m_fileName;
    GLenum m_textureTarget;
    GLuint m_textureObj;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    int m_imageBPP = 0;
};

#endif  /* TEXTURE_H */
