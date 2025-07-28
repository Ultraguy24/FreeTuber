#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
  Camera(int width, int height);

  void resize(int width, int height);
  void mouseButton(int button, int action, double x, double y);
  void mouseMove(double x, double y);
  void scroll(double offset);

  // Returns the full view matrix, including pan, orbit and zoom
  glm::mat4 getView() const;

private:
  int    w, h;
  glm::vec3 target{0, 1, 0};
  float  distance{3.0f};
  float  yaw{0}, pitch{0};
  bool   rotating{false}, panning{false};
  double lastX{0}, lastY{0};
  glm::vec2 pan{0, 0};
};
