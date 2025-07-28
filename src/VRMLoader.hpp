#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

// A single mesh primitive
struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint diffuseTex = 0;
    size_t count = 0;
    // Node/primitive name from the VRM (e.g. "J_Bip_C_Head", "Hair", "Body")
    std::string name;
};

// All loaded meshes
extern std::vector<Mesh> meshes;

// Per-mesh Y-bounds (if you still need thresholding)
extern std::vector<float> meshYMin;
extern std::vector<float> meshYMax;

// Head pivot in MODEL SPACE (neck joint position)
extern glm::vec3 headPivot;

// Load a VRM/glb and fill meshes + headPivot
bool loadVRM(const std::string& path);
