//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>
#include <stdexcept>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions) //加载顶点位置
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);
    /*
    pos_buf的类型是std::map<int, std::vector<Eigen::Vector3f>>
    .emplace(key, value)是在map中直接构造一个键值对，相当于插入，将id和positions存到map中
    */

    return {id}; //列表初始化，等价于return pos_buf_id{id}
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices) //加载索引
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

// Bresenham's line drawing algorithm
// Code taken from a stack overflow answer: https://stackoverflow.com/a/16405254
void rst::rasterizer::draw_line(Eigen::Vector3f begin, Eigen::Vector3f end)
{
    auto x1 = begin.x();
    auto y1 = begin.y();
    auto x2 = end.x();
    auto y2 = end.y();

    Eigen::Vector3f line_color = {255, 255, 255}; //白色

    int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;

    dx=x2-x1;
    dy=y2-y1;
    dx1=fabs(dx);
    dy1=fabs(dy);
    px=2*dy1-dx1;
    py=2*dx1-dy1; //决策参数，决定下一步是直线方向还是对角线方向

    if(dy1<=dx1) //当直线较平缓时，x 方向步进，y 方向根据决策参数决定是否变化
    {
        if(dx>=0)
        {
            x=x1;
            y=y1;
            xe=x2;
        }
        else
        {
            x=x2;
            y=y2;
            xe=x1;
        }
        Eigen::Vector3f point = Eigen::Vector3f(x, y, 1.0f);
        set_pixel(point,line_color);
        /*   
  Bresenham 算法的核心思想：
  - 每次 x 加 1，判断 y 是否也要加 1 或减 1
  - 决策参数 px 告诉我们：当前像素离真实直线更近的 y
  值是哪个
  - 如果 px < 0，说明直线还在当前 y 行，y 不变
  - 如果 px >= 0，说明直线已经越过半个像素了，y 要变化
        */
        for(i=0;x<xe;i++)
        {
            x=x+1;
            if(px<0)
            {
                px=px+2*dy1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0)) //斜率正
                {
                    y=y+1;
                }
                else //斜率负
                {
                    y=y-1;
                }
                px=px+2*(dy1-dx1);
            }
//            delay(0);
            Eigen::Vector3f point = Eigen::Vector3f(x, y, 1.0f);
            set_pixel(point,line_color);
        }
    }
    else
    {
        if(dy>=0)
        {
            x=x1;
            y=y1;
            ye=y2;
        }
        else
        {
            x=x2;
            y=y2;
            ye=y1;
        }
        Eigen::Vector3f point = Eigen::Vector3f(x, y, 1.0f);
        set_pixel(point,line_color);
        for(i=0;y<ye;i++)
        {
            y=y+1;
            if(py<=0)
            {
                py=py+2*dx1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0))
                {
                    x=x+1;
                }
                else
                {
                    x=x-1;
                }
                py=py+2*(dx1-dy1);
            }
//            delay(0);
            Eigen::Vector3f point = Eigen::Vector3f(x, y, 1.0f);
            set_pixel(point,line_color);
        }
    }
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w); //把三维向量拓展为四维齐次向量用于变换
}

void rst::rasterizer::draw(rst::pos_buf_id pos_buffer, rst::ind_buf_id ind_buffer, rst::Primitive type)
{
    if (type != rst::Primitive::Triangle)
    {
        throw std::runtime_error("Drawing primitives other than triangle is not implemented yet!");
    }
    auto& buf = pos_buf[pos_buffer.pos_id]; //auto&是引用类型，不拷贝数据，只是别名
    auto& ind = ind_buf[ind_buffer.ind_id]; //用operator[]访问map中对应ID的值

    float f1 = (100 - 0.1) / 2.0;
    float f2 = (100 + 0.1) / 2.0;
    //这两个值用于把NDC中的z从[-1,1]映射到深度缓冲的实际范围[0.1,100]
    //在后面的视口变换中会用到vert.z() = vert.z() * f1 + f2;

    Eigen::Matrix4f mvp = projection * view * model;
    //对每个三角形做MVP变换
    for (auto& i : ind)
    {
        Triangle t;

        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        /*
        对每个顶点，从顶点缓冲中取出第i[k]个顶点坐标(Vector3f),用to_vec(..., 1.0f)转化为齐次坐标(x, y, z, 1.0)
        最后乘上mvp矩阵乘法，顶点从局部坐标转化为裁剪坐标
        */

        for (auto& vec : v) {
            vec /= vec.w(); //透视除法
        }

        //视口变换：把NDC坐标[-1, 1]映射到屏幕坐标
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2; //映射到深度缓冲区范围
        }

        //设置三角形顶点和颜色
        //v[i].head<3>()：Eigen 提供的函数，取前 3 个分量，把Vector4f 的前 3 个元素提取为 Vector3f
        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        t.setColor(0, 255.0,  0.0,  0.0);
        t.setColor(1, 0.0  ,255.0,  0.0);
        t.setColor(2, 0.0  ,  0.0,255.0);

        rasterize_wireframe(t); //光栅化，把三角形画成线框
    }
}

void rst::rasterizer::rasterize_wireframe(const Triangle& t) //画三角形线框
{
    //画三角形的三条边
    draw_line(t.c(), t.a());
    draw_line(t.c(), t.b());
    draw_line(t.b(), t.a());
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff) //清空缓冲区
{
    //std::fill：把一段范围的元素全部设为指定值
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h) //构造函数，初始化缓冲区
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y) //根据屏幕坐标(x, y)返回对应的缓冲区索引
{
    return (height-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color) //设置像素点颜色
{
    //old index: auto ind = point.y() + point.x() * width;
    if (point.x() < 0 || point.x() >= width ||
        point.y() < 0 || point.y() >= height) return;
    auto ind = (height-point.y())*width + point.x();
    frame_buf[ind] = color;
}
