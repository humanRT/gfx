#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

#include <assimp/scene.h>

#define INVALID_MATERIAL 0xFFFFFFFF

class MeshData
{
public:
    MeshData()
    {
        Name = "";
        Parent = nullptr;
        NumIndices = 0;
        BaseVertex = 0;
        BaseIndex = 0;
        MaterialIndex = INVALID_MATERIAL;
        Transform = aiMatrix4x4();
    }

    std::string Name;
    MeshData* Parent;
    uint NumIndices;
    uint BaseVertex;
    uint BaseIndex;
    uint MaterialIndex;
    aiMatrix4x4 Transform;

    std::string printPosition()
    {
        std::ostringstream oss;
        oss << "(" << Transform.a4 << ", " << Transform.b4 << ", " << Transform.c4 << ")";
        return oss.str();
    }

    static MeshData* findByName(std::vector<MeshData>& meshes, const std::string& targetName)
    {
        // Check if targetName is not initialized
        if (std::addressof(targetName) == nullptr) {
            return nullptr;
        }

        auto it = std::find_if(meshes.begin(), meshes.end(), [&targetName](const MeshData& mesh) {
            return mesh.Name == targetName;
        });
        
        if (it != meshes.end()) { return &(*it); } 
        else { return nullptr; }
    }

    static MeshData* findByName(std::vector<MeshData>& meshes, const aiString& targetName)
    {
        // Check if targetName is not initialized
        if (std::addressof(targetName) == nullptr) {
            return nullptr;
        }

        auto it = std::find_if(meshes.begin(), meshes.end(), [&targetName](const MeshData& mesh) {
            return mesh.Name == targetName.C_Str();
        });
        
        if (it != meshes.end()) { return &(*it); } 
        else { return nullptr; }
    }
};