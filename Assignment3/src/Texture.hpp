//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

    Eigen::Vector3f getColorBilinear(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;

        int u0 = std::floor(u_img);
        int u1 = std::ceil(u_img);
        int v0 = std::floor(v_img);
        int v1 = std::ceil(v_img);

        float s = u_img - u0;
        float t = v_img - v0;

        cv::Vec3b c00 = image_data.at<cv::Vec3b>(v0, u0);
        cv::Vec3b c10 = image_data.at<cv::Vec3b>(v0, u1);
        cv::Vec3b c01 = image_data.at<cv::Vec3b>(v1, u0);
        cv::Vec3b c11 = image_data.at<cv::Vec3b>(v1, u1);

        auto lerp = [](const cv::Vec3b& a, const cv::Vec3b& b, float w) {
            return cv::Vec3b(
                    (uchar)(a[0] * (1 - w) + b[0] * w),
                    (uchar)(a[1] * (1 - w) + b[1] * w),
                    (uchar)(a[2] * (1 - w) + b[2] * w)
            );
        };

        cv::Vec3b c0 = lerp(c00, c10, s);
        cv::Vec3b c1 = lerp(c01, c11, s);
        cv::Vec3b c = lerp(c0, c1, t);

        return Eigen::Vector3f(c[0], c[1], c[2]);
    }
};
#endif //RASTERIZER_TEXTURE_H
