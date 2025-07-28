#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include <glm/glm.hpp>

// Call this once at startup to load/enable the Haar cascade.
// If loading fails, estimateHead() will simply return identity.
void initHeadPose(const std::string& cascadePath);

// Given a camera frame, returns a head‐pose matrix or identity if disabled.
glm::mat4 estimateHead(const cv::Mat& frame);
