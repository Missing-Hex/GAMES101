//
// Created by goksu on 4/6/19.
//

#pragma once //防止重复包含

#include "Triangle.hpp"
#include <algorithm>
#include <eigen3/Eigen/Eigen>
using namespace Eigen; //.hpp文件中用using namespace引用命名空间其实不太好

namespace rst { //rasterizer
enum class Buffers //强枚举
{
    Color = 1,
    Depth = 2
};

//inline建议编译器把函数体直接嵌入调用处（而非函数调用跳转），提高性能。适用于短小函数
inline Buffers operator|(Buffers a, Buffers b) //运算符重载
{
    return Buffers((int)a | (int)b); //按位或
}

inline Buffers operator&(Buffers a, Buffers b)
{
    return Buffers((int)a & (int)b); //按位与
}

enum class Primitive //图元
{
    Line,
    Triangle
};

/*
 * For the curious : The draw function takes two buffer id's as its arguments.
 * These two structs make sure that if you mix up with their orders, the
 * compiler won't compile it. Aka : Type safety
 * */
struct pos_buf_id //顶点位置缓冲的ID
{
    int pos_id = 0;
};

struct ind_buf_id //索引位置缓冲的ID
{
    int ind_id = 0;
};

class rasterizer
{
  public:
    rasterizer(int w, int h);
    pos_buf_id load_positions(const std::vector<Eigen::Vector3f>& positions);
    ind_buf_id load_indices(const std::vector<Eigen::Vector3i>& indices);
    //这里const承诺不修改传入的数据，&作为引用传递并不拷贝整个vector

    void set_model(const Eigen::Matrix4f& m);
    void set_view(const Eigen::Matrix4f& v);
    void set_projection(const Eigen::Matrix4f& p);
    //MVP矩阵

    void set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color);
    //设置像素

    void clear(Buffers buff);
    //清除缓冲区

    void draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, Primitive type);
    //核心绘制函数

    std::vector<Eigen::Vector3f>& frame_buffer() { return frame_buf; }
    //获取帧缓冲：存储每个像素的颜色

  private:
    void draw_line(Eigen::Vector3f begin, Eigen::Vector3f end); //Bresenham画线算法
    void rasterize_wireframe(const Triangle& t); //用三条线段画出三角形线框

  private:
    Eigen::Matrix4f model;
    Eigen::Matrix4f view;
    Eigen::Matrix4f projection;

    std::map<int, std::vector<Eigen::Vector3f>> pos_buf;
    std::map<int, std::vector<Eigen::Vector3i>> ind_buf;

    std::vector<Eigen::Vector3f> frame_buf;
    std::vector<float> depth_buf; //深度缓冲
    int get_index(int x, int y); //2D坐标映射到1D数组索引

    int width, height;

    int next_id = 0;
    int get_next_id() { return next_id++; }
};
} // namespace rst
