#ifndef MATERIAL_H
#define MATERIAL_H

#include "texture.hpp"

struct PBRMaterial
{
    float Roughness = 0.0f;
    bool IsMetal = false;
    Vector3f Color = Vector3f(0.0f, 0.0f, 0.0f);
    Texture* pAlbedo = NULL;
    Texture* pRoughness = NULL;
    Texture* pMetallic = NULL;
    Texture* pNormalMap = NULL;
};

class Material {

 public:

    std::string m_name;

    Vector4f AmbientColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    Vector4f DiffuseColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    Vector4f SpecularColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);

    PBRMaterial PBRmaterial;

    Texture* pDiffuse = NULL; // base color of the material
    Texture* pSpecularExponent = NULL;

    float m_transparencyFactor = 1.0f;
    float m_alphaTest = 0.0f;

    ~Material()
    {
        if (pDiffuse) {
            delete pDiffuse;
        }

        if (pSpecularExponent) {
            delete pSpecularExponent;
        }
    }

    glm::vec3 getAmbientColor() const
    {
        return glm::vec3(AmbientColor.x, AmbientColor.y, AmbientColor.z);
    }

    void setAmbientColor(const glm::vec3& color)
    {
        AmbientColor = Vector4f(color.r, color.g, color.b, 1.0f);
    }

    glm::vec3 getDiffuseColor() const
    {
        return glm::vec3(DiffuseColor.x, DiffuseColor.y, DiffuseColor.z);
    }

    void setDiffuseColor(const glm::vec3& color)
    {
        DiffuseColor = Vector4f(color.r, color.g, color.b, 1.0f);
    }

    glm::vec3 getSpecularColor() const
    {
        return glm::vec3(SpecularColor.x, SpecularColor.y, SpecularColor.z);
    }

    void setSpecularColor(const glm::vec3& color)
    {
        SpecularColor = Vector4f(color.r, color.g, color.b, 1.0f);
    }
};

#endif /* MATERIAL_H */
