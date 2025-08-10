cmake_minimum_required(VERSION 3.10)
project(FreeTuber LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# system deps
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(glm REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# third-party
add_subdirectory(third_party/glad)
add_library(tinygltf STATIC
  third_party/tinygltf/tiny_gltf.cc
)
target_include_directories(tinygltf PUBLIC
  ${CMAKE_SOURCE_DIR}/third_party/tinygltf
)
target_link_libraries(tinygltf PUBLIC
  nlohmann_json::nlohmann_json
)

# our sources
file(GLOB SRC
  src/*.cpp
)

add_executable(FreeTuber ${SRC})

target_include_directories(FreeTuber PRIVATE
  src
  ${CMAKE_SOURCE_DIR}/third_party/glad/include
  ${CMAKE_SOURCE_DIR}/third_party/tinygltf
  ${GLFW_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FreeTuber PRIVATE
  glad
  tinygltf
  glfw
  OpenGL::GL
  glm::glm
  ${OpenCV_LIBS}
)

link_directories(${GLFW_LIBRARY_DIRS})
