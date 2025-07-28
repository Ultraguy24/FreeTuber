#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "VRMLoader.hpp"
#include "HeadPose.hpp"
#include "Camera.hpp"
#include <opencv2/opencv.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Pre‐rotate avatar 180° so it faces the camera
static const glm::mat4 modelMat = glm::rotate(
    glm::mat4(1.0f), glm::radians(180.0f),
    glm::vec3(0,1,0)
);

// Head‐pose smoothing
static const float smoothAlpha = 0.1f;
static glm::quat prevHeadQuat(1,0,0,0);

static std::filesystem::path getExeDir(const char* argv0) {
    std::filesystem::path p(argv0);
    if (!p.is_absolute()) p = std::filesystem::current_path() / p;
    try { p = std::filesystem::canonical(p); }
    catch(...) { p = std::filesystem::absolute(p); }
    return p.parent_path();
}

static void framebuffer_size_callback(GLFWwindow* w,int w_,int h_) {
    auto cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
    if (cam) cam->resize(w_, h_);
    glViewport(0, 0, w_, h_);
}
static void mouse_button_callback(GLFWwindow* w,int b,int a,int){
    double x,y; glfwGetCursorPos(w,&x,&y);
    auto cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
    if (cam) cam->mouseButton(b,a,x,y);
}
static void cursor_pos_callback(GLFWwindow* w,double x,double y){
    auto cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
    if (cam) cam->mouseMove(x,y);
}
static void scroll_callback(GLFWwindow* w,double,double yoff){
    auto cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
    if (cam) cam->scroll(yoff);
}

int main(int argc, char** argv) {
    // Paths & init
    auto exeDir  = getExeDir(argv[0]);
    auto assets  = exeDir / "assets";
    auto shaders = exeDir / "shaders";
    initHeadPose((assets/"haarcascade_frontalface_default.xml").string());

    // GLFW + GLAD
    if (!glfwInit()) return -1;
    auto window = glfwCreateWindow(800,600,"FreeTuber",nullptr,nullptr);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr<<"GLAD init failed\n"; return -1;
    }
    glDisable(GL_CULL_FACE);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Camera & callbacks
    Camera cam(800,600);
    glfwSetWindowUserPointer(window, &cam);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window,    mouse_button_callback);
    glfwSetCursorPosCallback(window,      cursor_pos_callback);
    glfwSetScrollCallback(window,         scroll_callback);
    glViewport(0,0,800,600);

    // Load shaders
    GLuint shader = loadShaderProgram(
        (shaders/"vert.glsl").string().c_str(),
        (shaders/"frag.glsl").string().c_str()
    );
    if (!shader) return -1;
    glUseProgram(shader);

    // Lighting uniforms
    glUniform3f(glGetUniformLocation(shader,"uLightDir"), 0.5f,1.0f,0.3f);
    glUniform3f(glGetUniformLocation(shader,"uAmbient"),  0.2f,0.2f,0.2f);

    // Load VRM (this now forces RGBA upload of textures)
    if (!loadVRM((assets/"model.vrm").string())) {
        std::cerr<<"Failed to load VRM\n"; return -1;
    }

    // Split meshes by name (head vs body) — unchanged from before
    std::vector<int> headMeshIndices, bodyMeshIndices;
    for (int i = 0; i < (int)meshes.size(); ++i) {
        std::string n=meshes[i].name;
        std::transform(n.begin(),n.end(),n.begin(),[](unsigned char c){return std::tolower(c);});
        if (n.find("head")!=std::string::npos ||
            n.find("hair")!=std::string::npos ||
            n.find("face")!=std::string::npos) {
            headMeshIndices.push_back(i);
        } else {
            bodyMeshIndices.push_back(i);
        }
    }

    // Open webcam
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr<<"Webcam open failed\n"; return -1;
    }

    // Projection uniform
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f
    );
    glUniformMatrix4fv(
        glGetUniformLocation(shader,"uProj"),
        1, GL_FALSE, &proj[0][0]
    );

    GLint locModel = glGetUniformLocation(shader,"uModel");
    GLint locView  = glGetUniformLocation(shader,"uView");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Camera view
        glm::mat4 view = cam.getView();
        glUniformMatrix4fv(locView,1,GL_FALSE,&view[0][0]);

        // Head pose & smoothing
        cv::Mat frame; cap>>frame;
        glm::mat4 rawHead = estimateHead(frame);
        glm::mat4 headM   = glm::inverse(rawHead);
        glm::quat HQ      = glm::quat_cast(headM);
        prevHeadQuat      = glm::slerp(prevHeadQuat, HQ, smoothAlpha);
        glm::mat4 smoothHeadM = glm::mat4_cast(prevHeadQuat);

        // Build head transform (same pivot logic as before)
        glm::mat4 Tneg = glm::translate(glm::mat4(1.0f), -headPivot);
        glm::mat4 Tpos = glm::translate(glm::mat4(1.0f),  headPivot);
        glm::mat4 headModel = Tpos * smoothHeadM * Tneg * modelMat;

        // Draw
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // BODY PASS
        glUniformMatrix4fv(locModel,1,GL_FALSE,&modelMat[0][0]);
        for (int idx: bodyMeshIndices) {
            auto& m = meshes[idx];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m.diffuseTex);
            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES,(GLsizei)m.count,GL_UNSIGNED_INT,nullptr);
        }

        // HEAD PASS (transparent textures now blend correctly)
        glUniformMatrix4fv(locModel,1,GL_FALSE,&headModel[0][0]);
        for (int idx: headMeshIndices) {
            auto& m = meshes[idx];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m.diffuseTex);
            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES,(GLsizei)m.count,GL_UNSIGNED_INT,nullptr);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
