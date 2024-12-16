#include "mesh.hpp"

#define POSITION_LOCATION  0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION    2

std::string GetFullPath(const std::string& dir, const aiString& Path)
{
    std::string p(Path.data);

    if (p == "C:\\\\") {
        p = "";
    }
    else if (p.substr(0, 2) == ".\\") {
        p = p.substr(2, p.size() - 2);
    }

    std::string FullPath = dir + "/" + p;

    return FullPath;
}

Mesh::Mesh()
{ 
}

Mesh::~Mesh()
{
    clear();
}

void Mesh::clear()
{
    if (m_buffers[0] != 0) {
        glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_buffers), m_buffers);
    }

    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void Mesh::countVerticesAndIndices(aiNode* node, const aiScene* scene, unsigned int& numVertices, unsigned int& numIndices, const aiMatrix4x4& parentTransform)
{
    aiMatrix4x4 globalTransform = parentTransform * node->mTransformation;

    // Iterate through the meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        const aiMesh* paiMesh = scene->mMeshes[meshIndex];

        // Set offsets and counts in m_meshes
        m_meshes[meshIndex].Name = paiMesh->mName.C_Str();
        m_meshes[meshIndex].MaterialIndex = paiMesh->mMaterialIndex;
        m_meshes[meshIndex].NumIndices = paiMesh->mNumFaces * 3;
        m_meshes[meshIndex].BaseVertex = numVertices;
        m_meshes[meshIndex].BaseIndex = numIndices;
        m_meshes[meshIndex].Transform = globalTransform;

        // Update the total counts
        numVertices += paiMesh->mNumVertices;
        numIndices += m_meshes[meshIndex].NumIndices;
    }

    // Recursively process child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        countVerticesAndIndices(node->mChildren[i], scene, numVertices, numIndices, globalTransform);
    }
}

void Mesh::reserveSpace(unsigned int numVertices, unsigned int numIndices)
{
    m_vertices.reserve(numVertices);
    m_indices.reserve(numIndices);
}

bool Mesh::initScene(const aiScene* pScene, const std::string& filename)
{
    unsigned int numVertices = 0;
    unsigned int numIndices = 0;

    aiMatrix4x4 identity; // Identity matrix
    m_meshes.resize(pScene->mNumMeshes);
    m_materials.resize(pScene->mNumMaterials);

    countVerticesAndIndices(pScene->mRootNode, pScene, numVertices, numIndices, identity);
    reserveSpace(numVertices, numIndices);
    processNode(pScene->mRootNode, pScene);
    
    extractTrianglesFromScene();

    if (!initMaterials(pScene, filename)) {
        return false;
    }

    populateBuffers();

    return GL_CHECK_ERROR();
}

void Mesh::processNode(aiNode* node, const aiScene* scene, int level)
{
    MeshData meshData = MeshData::findByName(m_meshes, node->mName.C_Str());
    printf("%s%s %s\n", std::string(2 * level++, ' ').c_str(), meshData.Name.c_str(), meshData.printPosition().c_str());
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh* paiMesh = scene->mMeshes[node->mMeshes[i]];
        initSingleMesh(paiMesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, level);
    }
}

void Mesh::initSingleMesh(const aiMesh* paiMesh)
{
    Vertex v;
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    
    // Populate the vertex attribute vectors
    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D& pPos = paiMesh->mVertices[i];
        v.position = Vector3f(pPos.x, pPos.y, pPos.z);

        if (paiMesh->mNormals) {
            const aiVector3D& pNormal = paiMesh->mNormals[i];
            v.normal = Vector3f(pNormal.x, pNormal.y, pNormal.z);
        } 
        else 
        {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            v.normal = Vector3f(Normal.x, Normal.y, Normal.z);
        }

        const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
        v.texCoords = Vector2f(pTexCoord.x, pTexCoord.y);

        m_vertices.push_back(v);
    }

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        m_indices.push_back(Face.mIndices[0]);
        m_indices.push_back(Face.mIndices[1]);
        m_indices.push_back(Face.mIndices[2]);
    }
}

bool Mesh::initMaterials(const aiScene* pScene, const std::string& filename)
{
    std::string dir = utils::disk::getDirFromFilename(filename);

    bool Ret = true;

    printf("Num materials: %d\n", pScene->mNumMaterials);

    // Initialize the materials
    for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        loadTextures(dir, pMaterial, i);
        loadColors(pMaterial, i);
    }

    return Ret;
}

void Mesh::loadTextures(const std::string& dir, const aiMaterial* pMaterial, int Index)
{
    loadDiffuseTexture(dir, pMaterial, Index);
    loadSpecularTexture(dir, pMaterial, Index);

    // PBR
    loadAlbedoTexture(dir, pMaterial, Index);
    loadMetalnessTexture(dir, pMaterial, Index);
    loadRoughnessTexture(dir, pMaterial, Index);
}

void Mesh::loadDiffuseTexture(const std::string& dir, const aiMaterial* pMaterial, int materialIndex)
{
    m_materials[materialIndex].pDiffuse = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                loadDiffuseTextureEmbedded(paiTexture, materialIndex);
            } else {
                loadDiffuseTextureFromFile(dir, Path, materialIndex);
            }
        }
    }
}

void Mesh::loadDiffuseTextureEmbedded(const aiTexture* paiTexture, int materialIndex)
{
    printf("Embeddeded diffuse texture type '%s'\n", paiTexture->achFormatHint);
    m_materials[materialIndex].pDiffuse = new Texture(GL_TEXTURE_2D);
    int buffer_size = paiTexture->mWidth;
    m_materials[materialIndex].pDiffuse->Load(buffer_size, paiTexture->pcData);
}

void Mesh::loadDiffuseTextureFromFile(const std::string& dir, const aiString& Path, int materialIndex)
{
    std::string FullPath = GetFullPath(dir, Path);

    m_materials[materialIndex].pDiffuse = new Texture(GL_TEXTURE_2D, FullPath.c_str());

    if (!m_materials[materialIndex].pDiffuse->Load()) {
        printf("Error loading diffuse texture '%s'\n", FullPath.c_str());
        exit(0);
    }
    else {
        printf("Loaded diffuse texture '%s' at index %d\n", FullPath.c_str(), materialIndex);
    }
}

void Mesh::loadSpecularTexture(const std::string& dir, const aiMaterial* pMaterial, int materialIndex)
{
    m_materials[materialIndex].pSpecularExponent = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                loadSpecularTextureEmbedded(paiTexture, materialIndex);
            } else {
                loadSpecularTextureFromFile(dir, Path, materialIndex);
            }
        }
    }
}

void Mesh::loadSpecularTextureEmbedded(const aiTexture* paiTexture, int materialIndex)
{
    printf("Embeddeded specular texture type '%s'\n", paiTexture->achFormatHint);
    m_materials[materialIndex].pSpecularExponent = new Texture(GL_TEXTURE_2D);
    int buffer_size = paiTexture->mWidth;
    m_materials[materialIndex].pSpecularExponent->Load(buffer_size, paiTexture->pcData);
}

void Mesh::loadSpecularTextureFromFile(const std::string& dir, const aiString& Path, int materialIndex)
{
    std::string FullPath = GetFullPath(dir, Path);

    m_materials[materialIndex].pSpecularExponent = new Texture(GL_TEXTURE_2D, FullPath.c_str());

    if (!m_materials[materialIndex].pSpecularExponent->Load()) {
        printf("Error loading specular texture '%s'\n", FullPath.c_str());
        exit(0);
    }
    else {
        printf("Loaded specular texture '%s'\n", FullPath.c_str());
    }
}

void Mesh::loadAlbedoTexture(const std::string& dir, const aiMaterial* pMaterial, int materialIndex)
{
    m_materials[materialIndex].PBRmaterial.pAlbedo = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                loadAlbedoTextureEmbedded(paiTexture, materialIndex);
            } else {
                loadAlbedoTextureFromFile(dir, Path, materialIndex);
            }
        }
    }
}

void Mesh::loadAlbedoTextureEmbedded(const aiTexture* paiTexture, int materialIndex)
{
    printf("Embeddeded albedo texture type '%s'\n", paiTexture->achFormatHint);
    m_materials[materialIndex].PBRmaterial.pAlbedo = new Texture(GL_TEXTURE_2D);
    int buffer_size = paiTexture->mWidth;
    m_materials[materialIndex].PBRmaterial.pAlbedo->Load(buffer_size, paiTexture->pcData);
}

void Mesh::loadAlbedoTextureFromFile(const std::string& dir, const aiString& Path, int materialIndex)
{
    std::string FullPath = GetFullPath(dir, Path);

    m_materials[materialIndex].PBRmaterial.pAlbedo = new Texture(GL_TEXTURE_2D, FullPath.c_str());

    if (!m_materials[materialIndex].PBRmaterial.pAlbedo->Load()) {
        printf("Error loading albedo texture '%s'\n", FullPath.c_str());
        exit(0);
    }
    else {
        printf("Loaded albedo texture '%s'\n", FullPath.c_str());
    }
}

void Mesh::loadMetalnessTexture(const std::string& dir, const aiMaterial* pMaterial, int materialIndex)
{
    m_materials[materialIndex].PBRmaterial.pMetallic = NULL;

    int NumTextures = pMaterial->GetTextureCount(aiTextureType_METALNESS);

    if (NumTextures > 0) {
        printf("Num metalness textures %d\n", NumTextures);

        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_METALNESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                loadMetalnessTextureEmbedded(paiTexture, materialIndex);
            }
            else {
                loadMetalnessTextureFromFile(dir, Path, materialIndex);
            }
        }
    }
}

void Mesh::loadMetalnessTextureEmbedded(const aiTexture* paiTexture, int materialIndex)
{
    printf("Embeddeded metalness texture type '%s'\n", paiTexture->achFormatHint);
    m_materials[materialIndex].PBRmaterial.pMetallic = new Texture(GL_TEXTURE_2D);
    int buffer_size = paiTexture->mWidth;
    m_materials[materialIndex].PBRmaterial.pMetallic->Load(buffer_size, paiTexture->pcData);
}

void Mesh::loadMetalnessTextureFromFile(const std::string& dir, const aiString& Path, int materialIndex)
{
    std::string FullPath = GetFullPath(dir, Path);

    m_materials[materialIndex].PBRmaterial.pMetallic = new Texture(GL_TEXTURE_2D, FullPath.c_str());

    if (!m_materials[materialIndex].PBRmaterial.pMetallic->Load()) {
        printf("Error loading metalness texture '%s'\n", FullPath.c_str());
        exit(0);
    }
    else {
        printf("Loaded metalness texture '%s'\n", FullPath.c_str());
    }
}

void Mesh::loadRoughnessTexture(const std::string& dir, const aiMaterial* pMaterial, int materialIndex)
{
    m_materials[materialIndex].PBRmaterial.pRoughness = NULL;

    int NumTextures = pMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);

    if (NumTextures > 0) {
        printf("Num roughness textures %d\n", NumTextures);

        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                loadRoughnessTextureEmbedded(paiTexture, materialIndex);
            }
            else {
                loadRoughnessTextureFromFile(dir, Path, materialIndex);
            }
        }
    }
}

void Mesh::loadRoughnessTextureEmbedded(const aiTexture* paiTexture, int materialIndex)
{
    printf("Embeddeded roughness texture type '%s'\n", paiTexture->achFormatHint);
    m_materials[materialIndex].PBRmaterial.pRoughness = new Texture(GL_TEXTURE_2D);
    int buffer_size = paiTexture->mWidth;
    m_materials[materialIndex].PBRmaterial.pRoughness->Load(buffer_size, paiTexture->pcData);
}

void Mesh::loadRoughnessTextureFromFile(const std::string& dir, const aiString& Path, int materialIndex)
{
    std::string FullPath = GetFullPath(dir, Path);

    m_materials[materialIndex].PBRmaterial.pRoughness = new Texture(GL_TEXTURE_2D, FullPath.c_str());

    if (!m_materials[materialIndex].PBRmaterial.pRoughness->Load()) {
        printf("Error loading roughness texture '%s'\n", FullPath.c_str());
        exit(0);
    }
    else {
        printf("Loaded roughness texture '%s'\n", FullPath.c_str());
    }
}

void Mesh::populateBuffers()
{
    glNamedBufferStorage(m_buffers[VERTEX_BUFFER], sizeof(m_vertices[0]) * m_vertices.size(), m_vertices.data(), 0);
    glNamedBufferStorage(m_buffers[INDEX_BUFFER], sizeof(m_indices[0]) * m_indices.size(), m_indices.data(), 0);

    glVertexArrayVertexBuffer(m_VAO, 0, m_buffers[VERTEX_BUFFER], 0, sizeof(Vertex));
    glVertexArrayElementBuffer(m_VAO, m_buffers[INDEX_BUFFER]);

    size_t numFloats = 0;

    glEnableVertexArrayAttrib(m_VAO, POSITION_LOCATION);
    glVertexArrayAttribFormat(m_VAO, POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLuint)(numFloats * sizeof(float)));
    glVertexArrayAttribBinding(m_VAO, POSITION_LOCATION, 0);
    numFloats += 3;

    glEnableVertexArrayAttrib(m_VAO, TEX_COORD_LOCATION);
    glVertexArrayAttribFormat(m_VAO, TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, (GLuint)(numFloats * sizeof(float)));
    glVertexArrayAttribBinding(m_VAO, TEX_COORD_LOCATION, 0);
    numFloats += 2;

    glEnableVertexArrayAttrib(m_VAO, NORMAL_LOCATION);
    glVertexArrayAttribFormat(m_VAO, NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLuint)(numFloats * sizeof(float)));
    glVertexArrayAttribBinding(m_VAO, NORMAL_LOCATION, 0);
}

void Mesh::loadColors(const aiMaterial* pMaterial, int index)
{
    aiColor4D AmbientColor(0.0f, 0.0f, 0.0f, 0.0f);
    Vector4f AllOnes(1.0f);

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        printf("Shading model %d\n", ShadingModel);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
        printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b);
        m_materials[index].AmbientColor.r = AmbientColor.r;
        m_materials[index].AmbientColor.g = AmbientColor.g;
        m_materials[index].AmbientColor.b = AmbientColor.b;
    } else {
        m_materials[index].AmbientColor = AllOnes;
    }

    aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
        printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
        m_materials[index].DiffuseColor.r = DiffuseColor.r;
        m_materials[index].DiffuseColor.g = DiffuseColor.g;
        m_materials[index].DiffuseColor.b = DiffuseColor.b;
    }

    aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
        printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g, SpecularColor.b);
        m_materials[index].SpecularColor.r = SpecularColor.r;
        m_materials[index].SpecularColor.g = SpecularColor.g;
        m_materials[index].SpecularColor.b = SpecularColor.b;
    }
}

bool Mesh::loadMesh(const std::string& filename)
{
    bool result = false;
    
    clear();
    
    glCreateVertexArrays(1, &m_VAO);
    glCreateBuffers(ARRAY_SIZE_IN_ELEMENTS(m_buffers), m_buffers);
    
    m_pScene = m_importer.ReadFile(filename.c_str(), ASSIMP_LOAD_FLAGS);

    if (m_pScene) {
        m_globalInverseTransform = m_pScene->mRootNode->mTransformation;
        m_globalInverseTransform = m_globalInverseTransform.Inverse();
        result = initScene(m_pScene, filename);
    }
    else {
        printf("Error parsing '%s': '%s'\n", filename.c_str(), m_importer.GetErrorString());
    }
    
    return result;
}

void Mesh::drawTriangles(GLuint wireframeProgram, const glm::mat4& mvp) {
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    float normalLength = 0.25f;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> centroids;
    std::vector<glm::vec3> normalLines;

    for (const auto& tri : m_triangles) {
        vertices.push_back(tri.v0);
        vertices.push_back(tri.v1);
        vertices.push_back(tri.v2);
        
        glm::vec3 centroid = (tri.v0 + tri.v1 + tri.v2) / 3.0f;
        centroids.push_back(centroid);
    }

    for (size_t i = 0; i < m_indices.size(); i+=3) {
        glm::vec3 v0 = glm::vec3(m_vertices[m_indices[i]].position.x, m_vertices[m_indices[i]].position.y, m_vertices[m_indices[i]].position.z);
        glm::vec3 v1 = glm::vec3(m_vertices[m_indices[i + 1]].position.x, m_vertices[m_indices[i + 1]].position.y, m_vertices[m_indices[i + 1]].position.z);
        glm::vec3 v2 = glm::vec3(m_vertices[m_indices[i + 2]].position.x, m_vertices[m_indices[i + 2]].position.y, m_vertices[m_indices[i + 2]].position.z);

        glm::vec3 n0 = glm::vec3(m_vertices[m_indices[i]].normal.x, m_vertices[m_indices[i]].normal.y, m_vertices[m_indices[i]].normal.z);
        glm::vec3 n1 = glm::vec3(m_vertices[m_indices[i + 1]].normal.x, m_vertices[m_indices[i + 1]].normal.y, m_vertices[m_indices[i + 1]].normal.z);
        glm::vec3 n2 = glm::vec3(m_vertices[m_indices[i + 2]].normal.x, m_vertices[m_indices[i + 2]].normal.y, m_vertices[m_indices[i + 2]].normal.z);

        //Average normal
        glm::vec3 n = (n0 + n1 + n2) / 3.0f;
        glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;

        normalLines.push_back(centroid);
        normalLines.push_back(centroid + n * normalLength);
    }

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind and fill VBO with vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    // Enable and set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Use wireframe shader program
    glUseProgram(wireframeProgram);

    // Set uniform for wireframe color
    GLuint colorLoc = glGetUniformLocation(wireframeProgram, "wireframeColor");
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // Red color

    // Set uniform for MVP matrix
    GLuint mvpLoc = glGetUniformLocation(wireframeProgram, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    // Render in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLuint centroidVBO;
    glGenBuffers(1, &centroidVBO);
    glBindBuffer(GL_ARRAY_BUFFER, centroidVBO);
    glBufferData(GL_ARRAY_BUFFER, centroids.size() * sizeof(glm::vec3), centroids.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Set uniform for centroid color (green)
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);

    // Render points
    glPointSize(3.0f); // Adjust point size as needed
    glDrawArrays(GL_POINTS, 0, centroids.size());

    // Render normals as lines
    GLuint normalVBO;
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(glm::vec3), normalLines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Set uniform for normal color (cyan)
    glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f);

    // Render lines
    glDrawArrays(GL_LINES, 0, normalLines.size());

    // Cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glUseProgram(0); // Reset shader program
}

void Mesh::extractTrianglesFromScene() {
    for (unsigned int meshIndex = 0; meshIndex < m_pScene->mNumMeshes; ++meshIndex) {
        const aiMesh* mesh = m_pScene->mMeshes[meshIndex];

        // Extract vertex positions
        std::vector<glm::vec3> vertices;
        vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D& pos = mesh->mVertices[i];
            vertices.emplace_back(pos.x, pos.y, pos.z);
        }

        // Extract triangles from faces
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];

            // Assimp guarantees all faces are triangles with the default settings
            if (face.mNumIndices == 3) {
                glm::vec3 v0 = vertices[face.mIndices[0]];
                glm::vec3 v1 = vertices[face.mIndices[1]];
                glm::vec3 v2 = vertices[face.mIndices[2]];

                m_triangles.push_back({v0, v1, v2});
            }
        }
    }
    std::cout << "Num triangles: " << m_triangles.size() << std::endl;
}

void Mesh::render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection)
{
    glUseProgram(shaderProgram);

    // Set the view and projection matrices
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(m_VAO);

    for (unsigned int meshIndex = 0; meshIndex < m_meshes.size(); meshIndex++) {
        // Convert Assimp's aiMatrix4x4 to glm::mat4
        aiMatrix4x4 aiTransform = m_meshes[meshIndex].Transform;
        glm::mat4 transform = glm::transpose(glm::make_mat4(&aiTransform.a1));

        // Set the model matrix
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

        // Draw the mesh
        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 m_meshes[meshIndex].NumIndices,
                                 GL_UNSIGNED_INT,
                                 (void*)(sizeof(unsigned int) * m_meshes[meshIndex].BaseIndex),
                                 m_meshes[meshIndex].BaseVertex);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}