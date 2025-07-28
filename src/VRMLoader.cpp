#include "VRMLoader.hpp"
#include <tiny_gltf.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>   // ← added for std::find

// Globals
std::vector<Mesh>    meshes;
std::vector<float>   meshYMin;
std::vector<float>   meshYMax;
glm::vec3            headPivot(0.0f);

// Helper to read a node’s translation
static glm::vec3 getNodeTranslation(const tinygltf::Node& n) {
    if (n.translation.size() == 3) {
        return {
            (float)n.translation[0],
            (float)n.translation[1],
            (float)n.translation[2]
        };
    }
    if (n.matrix.size() == 16) {
        return {
            (float)n.matrix[12],
            (float)n.matrix[13],
            (float)n.matrix[14]
        };
    }
    return {0.0f, 0.0f, 0.0f};
}

bool loadVRM(const std::string& path) {
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;
    bool ok = false;

    // Load .vrm/.glb first
    auto ext = path.substr(path.find_last_of('.') + 1);
    if (ext == "vrm" || ext == "glb") {
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    }
    if (!ok) {
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    }
    if (!warn.empty()) std::cerr << "Warn: " << warn << "\n";
    if (!err.empty())  std::cerr << "Err:  " << err  << "\n";
    if (!ok) return false;

    // Find head node index
    int headNodeIndex = -1;
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        if (model.nodes[i].name == "J_Bip_C_Head") {
            headNodeIndex = (int)i;
            break;
        }
    }

    // Helper to find a node's parent
    auto findParent = [&](int childIdx) {
        for (size_t i = 0; i < model.nodes.size(); ++i) {
            const auto& children = model.nodes[i].children;
            if (std::find(children.begin(), children.end(), childIdx) != children.end()) {
                return (int)i;
            }
        }
        return -1;
    };

    // Compute global head-pivot by summing translations up the chain
    if (headNodeIndex >= 0) {
        glm::vec3 pivot = getNodeTranslation(model.nodes[headNodeIndex]);
        int parent = findParent(headNodeIndex);
        while (parent >= 0) {
            pivot += getNodeTranslation(model.nodes[parent]);
            parent = findParent(parent);
        }
        headPivot = pivot;
        std::cerr << "Computed global headPivot: "
                  << headPivot.x << ", "
                  << headPivot.y << ", "
                  << headPivot.z << "\n";
    } else {
        std::cerr << "Warning: J_Bip_C_Head node not found, headPivot remains (0,0,0)\n";
    }

    // 2) Upload images → textures
    std::vector<GLuint> glImages(model.images.size());
    for (size_t i = 0; i < model.images.size(); ++i) {
        auto& img = model.images[i];
        GLuint tex; glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        GLenum fmt = (img.component == 4 ? GL_RGBA : GL_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt,
                     img.width, img.height, 0,
                     fmt, GL_UNSIGNED_BYTE,
                     img.image.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glImages[i] = tex;
    }
    std::vector<GLuint> glTextures(model.textures.size());
    for (size_t i = 0; i < model.textures.size(); ++i) {
        glTextures[i] = glImages[ model.textures[i].source ];
    }
    // Fallback white texture
    GLuint whiteTex; glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    unsigned char w[4] = {255,255,255,255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1,1, 0, GL_RGBA, GL_UNSIGNED_BYTE, w);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 3) Upload each mesh primitive (Y-min/max, VAO/VBO/EBO, texture, name)
    for (size_t nodeIdx = 0; nodeIdx < model.nodes.size(); ++nodeIdx) {
        auto& node = model.nodes[nodeIdx];
        if (node.mesh < 0) continue;

        auto& meshDef = model.meshes[node.mesh];
        for (auto& prim : meshDef.primitives) {
            // POSITION
            auto& pAcc  = model.accessors[prim.attributes.at("POSITION")];
            auto& pView = model.bufferViews[pAcc.bufferView];
            const float* pos = reinterpret_cast<const float*>(
                &model.buffers[pView.buffer]
                      .data[pView.byteOffset + pAcc.byteOffset]);

            // NORMAL
            auto& nAcc  = model.accessors[prim.attributes.at("NORMAL")];
            auto& nView = model.bufferViews[nAcc.bufferView];
            const float* nor = reinterpret_cast<const float*>(
                &model.buffers[nView.buffer]
                      .data[nView.byteOffset + nAcc.byteOffset]);

            // TEXCOORD_0 (optional)
            const float* uvPtr = nullptr;
            size_t uvCount = 0;
            if (prim.attributes.count("TEXCOORD_0")) {
                auto& uAcc  = model.accessors[prim.attributes.at("TEXCOORD_0")];
                auto& uView = model.bufferViews[uAcc.bufferView];
                uvPtr = reinterpret_cast<const float*>(
                    &model.buffers[uView.buffer]
                          .data[uView.byteOffset + uAcc.byteOffset]);
                uvCount = uAcc.count;
            }

            // INDICES
            auto& iAcc  = model.accessors[prim.indices];
            auto& iView = model.bufferViews[iAcc.bufferView];
            const uint32_t* idx = reinterpret_cast<const uint32_t*>(
                &model.buffers[iView.buffer]
                      .data[iView.byteOffset + iAcc.byteOffset]);
            size_t idxCount = iAcc.count;

            // Build verts + compute Y-min/max
            float yMin =  1e6f, yMax = -1e6f;
            std::vector<float> verts;
            verts.reserve(pAcc.count * 8);
            for (size_t i = 0; i < pAcc.count; ++i) {
                float y = pos[i*3 + 1];
                yMin = std::min(yMin, y);
                yMax = std::max(yMax, y);

                // x,y,z
                verts.push_back(pos[i*3+0]);
                verts.push_back(pos[i*3+1]);
                verts.push_back(pos[i*3+2]);
                // nx,ny,nz
                verts.push_back(nor[i*3+0]);
                verts.push_back(nor[i*3+1]);
                verts.push_back(nor[i*3+2]);
                // u,v
                if (uvPtr && i < uvCount) {
                    verts.push_back(uvPtr[i*2+0]);
                    verts.push_back(uvPtr[i*2+1]);
                } else {
                    verts.push_back(0.0f);
                    verts.push_back(0.0f);
                }
            }
            meshYMin.push_back(yMin);
            meshYMax.push_back(yMax);

            // Copy indices
            std::vector<uint32_t> ids(idxCount);
            std::memcpy(ids.data(), idx, idxCount * sizeof(uint32_t));

            // Upload to GL
            Mesh out{};
            glGenVertexArrays(1, &out.vao);
            glGenBuffers(1,        &out.vbo);
            glGenBuffers(1,        &out.ebo);

            glBindVertexArray(out.vao);
            glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         verts.size() * sizeof(float),
                         verts.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         ids.size() * sizeof(uint32_t),
                         ids.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);

            out.count = ids.size();
            out.diffuseTex = whiteTex;
            if (prim.material >= 0) {
                auto& mat = model.materials[prim.material].pbrMetallicRoughness;
                if (mat.baseColorTexture.index >= 0) {
                    out.diffuseTex = glTextures[mat.baseColorTexture.index];
                }
            }

            // Tag mesh by node/mesh name
            out.name = !node.name.empty() ? node.name : meshDef.name;
            meshes.push_back(out);
        }
    }

    return true;
}