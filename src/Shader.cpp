#include "Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open shader: " << path << "\n";
        return {};
    }
    std::ostringstream ss; ss << in.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, ' ');
        glGetShaderInfoLog(s, len, nullptr, &log[0]);
        std::cerr << "Shader compile error:\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint loadShaderProgram(const char* vPath, const char* fPath) {
    auto vs = readFile(vPath), fs = readFile(fPath);
    if (vs.empty() || fs.empty()) return 0;

    GLuint v = compileShader(GL_VERTEX_SHADER,   vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    if (!v || !f) return 0;

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, ' ');
        glGetProgramInfoLog(p, len, nullptr, &log[0]);
        std::cerr << "Program link error:\n" << log << "\n";
        glDeleteProgram(p);
        return 0;
    }

    glDetachShader(p, v);
    glDetachShader(p, f);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// New function to load from source strings directly
GLuint loadShaderProgramFromSource(const char* vSrc, const char* fSrc) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vSrc);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fSrc);
    if (!v || !f) return 0;

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, ' ');
        glGetProgramInfoLog(p, len, nullptr, &log[0]);
        std::cerr << "Program link error:\n" << log << "\n";
        glDeleteProgram(p);
        return 0;
    }

    glDetachShader(p, v);
    glDetachShader(p, f);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}
