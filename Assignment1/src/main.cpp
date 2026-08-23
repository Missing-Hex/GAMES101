#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity(); //Eigen的静态成员函数，返回单位矩阵

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1; //Eigen的逗号初始化语法，初始化矩阵
    //translate矩阵：将相机移动到原点

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    float rad = rotation_angle / 180.0f * MY_PI;

    model << std::cos(rad), -std::sin(rad), 0, 0, 
        std::sin(rad), std::cos(rad), 0, 0, 
        0, 0, 1, 0, 
        0, 0, 0, 1;

    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    float t = zNear * std::tan(eye_fov / 2.0f * MY_PI / 180.f);
    float r = t * aspect_ratio;

    projection << zNear / r, 0, 0, 0,
        0, zNear / t, 0, 0,
        0, 0, (zNear + zFar) / (zNear - zFar), 2 * zNear * zFar / (zNear - zFar),
        0, 0, 1, 0;

    return projection;
}

//绕任意过原点的轴旋转（罗德里格斯旋转公式）
Eigen::Matrix4f get_rotation(Eigen::Vector3f axis, float rotation_angle)
{
    //归一化轴向量
    Eigen::Vector3f n = axis.normalized();

    //角度转弧度
    float rad = rotation_angle / 180.0f * MY_PI;

    //叉乘矩阵N
    Eigen::Matrix3f N;
    N << 0, -n.z(), n.y(),
        n.z(), 0, -n.x(),
        -n.y(), n.x(), 0;

    //核心思想：绕任意轴旋转 = 平行分量不动 + 垂直分量在平面里做 2D 旋转
    //R = cos·I + (1−cos)·n·nᵀ + sin·N
    Eigen::Matrix3f R3 = std::cos(rad) * Eigen::Matrix3f::Identity() + (1 - std::cos(rad)) * n * n.transpose() + std::sin(rad) * N;

    //将R3转换为4x4矩阵
    Eigen::Matrix4f R = Eigen::Matrix4f::Identity();
    R.block<3, 3>(0, 0) = R3;
    return R;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    /*
    例如，运行 ./Rasterizer -r 30 output.png：
    argc = 4
    argv[0] = "./Rasterizer"
    argv[1] = "-r"
    argv[2] = "30"
    argv[3] = "output.png"
    */
    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    //命令行模式分支
    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        //r.set_model(get_model_matrix(angle));
        r.set_model(get_rotation({1, 1, 1}, angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    //交互模式分支
    while (key != 27) { //27是ESC键的ASCII码
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        //帧循环（game loop）
        //每一帧重复四件事： 清屏 → 设置矩阵 → 画三角形 → 显示图片 → 处理按键
        
        //r.set_model(get_model_matrix(angle));
        r.set_model(get_rotation({1, 1, 1}, angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10); //等待10ms捕获按键

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
