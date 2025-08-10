#include "HeadPose.hpp"
#include <iostream>
#include <cmath>

static cv::CascadeClassifier faceCascade;
static bool useCascade = false;

void initHeadPose(const std::string& cascadePath) {
    std::cerr << "initHeadPose: loading " << cascadePath;
    try {
        useCascade = faceCascade.load(cascadePath);
    } catch (const cv::Exception& e) {
        useCascade = false;
        std::cerr << " exception=" << e.what();
    }
    std::cerr << " useCascade=" << (useCascade ? "true" : "false") << "\n";
}

glm::mat4 estimateHead(const cv::Mat& frame) {
    if (!useCascade) {
        return glm::mat4(1.0f);
    }

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(frame, faces, 1.1, 3);
    if (faces.empty()) {
        return glm::mat4(1.0f);
    }

    auto& r = faces[0];
    std::vector<cv::Point2d> imgPts = {
        {r.x + r.width * 0.5, r.y + r.height * 0.3},
        {r.x + r.width * 0.5, r.y + r.height * 0.7},
        {r.x + r.width * 0.2, r.y + r.height * 0.4},
        {r.x + r.width * 0.8, r.y + r.height * 0.4},
        {r.x + r.width * 0.3, r.y + r.height * 0.8},
        {r.x + r.width * 0.7, r.y + r.height * 0.8}
    };
    std::vector<cv::Point3d> mdlPts = {
        { 0.0,    0.0,    0.0},
        { 0.0, -63.6,  -12.5},
        {-43.3,  32.7,  -26.0},
        { 43.3,  32.7,  -26.0},
        {-28.9, -28.9,  -24.1},
        { 28.9, -28.9,  -24.1}
    };

    double f = frame.cols;
    cv::Mat cam = (cv::Mat_<double>(3,3) <<
        f, 0, frame.cols / 2,
        0, f, frame.rows / 2,
        0, 0, 1);
    cv::Mat dist = cv::Mat::zeros(4, 1, CV_64F);
    cv::Mat rvec, tvec;
    cv::solvePnP(mdlPts, imgPts, cam, dist, rvec, tvec);

    cv::Mat R;
    cv::Rodrigues(rvec, R);

    // Print for debug
    double sy = std::sqrt(R.at<double>(0,0)*R.at<double>(0,0) +
                          R.at<double>(1,0)*R.at<double>(1,0));
    bool singular = sy < 1e-6;
    double x, y, z;
    if (!singular) {
        x = std::atan2(R.at<double>(2,1), R.at<double>(2,2));
        y = std::atan2(-R.at<double>(2,0), sy);
        z = std::atan2(R.at<double>(1,0), R.at<double>(0,0));
    } else {
        x = std::atan2(-R.at<double>(1,2), R.at<double>(1,1));
        y = std::atan2(-R.at<double>(2,0), sy);
        z = 0;
    }
    auto toDeg = [](double r){ return r * 180.0 / M_PI; };
    std::cerr << "HeadPose Euler (deg): pitch=" << toDeg(x)
              << " yaw=" << toDeg(y)
              << " roll=" << toDeg(z) << "\n";

    glm::mat4 M(1.0f);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            M[j][i] = R.at<double>(i,j);

    return M;
}
