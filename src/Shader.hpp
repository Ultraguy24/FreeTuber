#pragma once
#include <glad/glad.h>

GLuint loadShaderProgram(const char* vertPath, const char* fragPath);

// New function to load shaders from source code strings
GLuint loadShaderProgramFromSource(const char* vertSrc, const char* fragSrc);
