#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(int width, int height)
  : w(width), h(height) {}

void Camera::resize(int width, int height) {
  w = width; h = height;
}

void Camera::mouseButton(int button, int action, double x, double y) {
  lastX = x; lastY = y;
  if (button == GLFW_MOUSE_BUTTON_LEFT)
    rotating = (action == GLFW_PRESS);
  if (button == GLFW_MOUSE_BUTTON_RIGHT)
    panning  = (action == GLFW_PRESS);
}

void Camera::mouseMove(double x, double y) {
  double dx = (x - lastX) / w;
  double dy = (y - lastY) / h;
  lastX = x; lastY = y;

  if (rotating) {
    yaw   += dx * 180.0f;
    pitch += dy * 180.0f;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
  }
  if (panning) {
    pan += glm::vec2(dx, -dy) * distance;
  }
}

void Camera::scroll(double offset) {
  distance *= std::pow(0.9f, offset);
  distance = std::clamp(distance, 0.2f, 10.0f);
}

glm::mat4 Camera::getView() const {
  // 1) Build lookAt
  glm::vec3 pos;
  pos.x = distance * std::cos(glm::radians(pitch)) * std::sin(glm::radians(yaw));
  pos.y = distance * std::sin(glm::radians(pitch));
  pos.z = distance * std::cos(glm::radians(pitch)) * std::cos(glm::radians(yaw));

  glm::mat4 look = glm::lookAt(pos + target, target, {0,1,0});

  // 2) Apply pan as a translation in view‐space
  glm::mat4 trans = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(-pan, 0.0f));

  // 3) First pan, then orbit
  return look * trans;
}
