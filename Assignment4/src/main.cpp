/*
#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3; //伯恩斯坦多项式

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    if(control_points.size() == 1)
    {
        return control_points[0];
    }

    std::vector<cv::Point2f> new_points;
    for(int i = 0; i < control_points.size() - 1; i++)
    {
        new_points.push_back(control_points[i] + t * (control_points[i + 1] - control_points[i]));
    }

    return recursive_bezier(new_points, t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.
    for(double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = recursive_bezier(control_points, t);
        window.at<cv::Vec3b>(point.y, point.x)[1] = 255;
    }
}

int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            naive_bezier(control_points, window);
            //bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
*/

//interactive version
#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;
int dragging_index = -1;

constexpr int HIT_RADIUS = 15; //命中检测的阈值

void mouse_handler(int event, int x, int y, int flags, void *userdata)
{
    if(event == cv::EVENT_LBUTTONDOWN)
    {
        int nearest = -1;
        float best_distance = (float)HIT_RADIUS * HIT_RADIUS;

        for(int i = 0; i < (int)control_points.size(); i++)
        {
            float dx = control_points[i].x - x;
            float dy = control_points[i].y - y;
            float dist2 = dx * dx + dy * dy;
            if(dist2 < best_distance)
            {
                nearest = i;
                best_distance = dist2;
            }
        }

        if(nearest != -1) {
            dragging_index = nearest;
        } else if(control_points.size() < 4) {
            control_points.emplace_back(x, y);
        } 
    } 
    else if(event == cv::EVENT_MOUSEMOVE && dragging_index != -1) 
    {
        control_points[dragging_index] = cv::Point2f(x, y);
    } 
    else if(event == cv::EVENT_LBUTTONUP) 
    {
        dragging_index = -1;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    if(control_points.size() == 1)
    {
        return control_points[0];
    }

    std::vector<cv::Point2f> new_points;
    for(int i = 0; i < (int)control_points.size() - 1; i++)
    {
        new_points.push_back(control_points[i] + t * (control_points[i + 1] - control_points[i]));
    }

    return recursive_bezier(new_points, t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.
    for(double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = recursive_bezier(control_points, t);
        window.at<cv::Vec3b>(point.y, point.x)[1] = 255;
    }
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3,cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve",cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback("Bezier Curve",mouse_handler, nullptr);

    int key = -1;
    while(key != 27)
    {
        window.setTo(0); //清屏，每帧重画

        for(auto &p : control_points)
        {
            cv::circle(window, p, 5, {255, 255, 255}, -1);
        }

        if(control_points.size() == 4)
        {
            bezier(control_points, window);
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

    return 0;
}