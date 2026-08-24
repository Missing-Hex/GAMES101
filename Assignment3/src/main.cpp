#include <iostream>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    // TODO: Use the same projection matrix from the previous assignments
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    float t = zNear * std::tan(eye_fov / 2.0f * MY_PI / 180.f);
    float r = t * aspect_ratio;

    projection << -zNear / r, 0, 0, 0,
        0, -zNear / t, 0, 0,
        0, 0, (zNear + zFar) / (zNear - zFar), 2 * zNear * zFar / (zNear - zFar),
        0, 0, 1, 0;

    return projection;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        // TODO: Get the texture value at the texture coordinates of the current fragment
        //return_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
        return_color = payload.texture->getColorBilinear(payload.tex_coords.x(), payload.tex_coords.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};
    auto l3 = light{{-20, -20, -20}, {500, 500, 500}}; //调试：左下后方补光
    auto l4 = light{{20, -20, -20}, {500, 500, 500}}; //调试：右后下补光

    std::vector<light> lights = {l1, l2, l3, l4};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        Vector3f l = (light.position - point); //光源方向
        Vector3f l_n = l.normalized();
        float r = l.norm(); //光源到像素的距离
        Vector3f v = (eye_pos - point).normalized(); //视线方向
        Vector3f h = (l_n + v).normalized(); //半程向量

        Vector3f spec = ks.cwiseProduct(light.intensity / (r * r)) * std::pow(std::max(0.0f, normal.dot(h)), p);
        Vector3f diff = kd.cwiseProduct(light.intensity / (r * r)) * std::max(0.0f, normal.dot(l_n));

        result_color += (diff + spec);
    }

    result_color += ka.cwiseProduct(amb_light_intensity);

    return result_color * 255.f;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005); //环境系数
    Eigen::Vector3f kd = payload.color; //漫反射系数
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937); //高光系数

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};
    auto l3 = light{{-20, -20, -20}, {500, 500, 500}}; //调试：左下后方补光
    auto l4 = light{{20, -20, -20}, {500, 500, 500}}; //调试：右后下补光

    std::vector<light> lights = {l1, l2, l3, l4};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150; //高光指数

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    /*
    Blinn-Phong 模型：三束光叠加
    一个像素最终颜色 = 环境光 + 漫反射 +高光，对每个光源算后两项：
    ambient   = ka ⊙ I_amb //与光源无关，整个场景加一次
    diffuse   = kd ⊙ (I / r²) · max(0, n·l)
    specular  = ks ⊙ (I / r²) · max(0, n·h)^p
    */
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        Vector3f l = (light.position - point);
        Vector3f l_n = l.normalized();
        float r = l.norm(); //光源到像素的距离
        Vector3f v = (eye_pos - point).normalized(); //视线方向
        Vector3f h = (l_n + v).normalized(); //半程向量

        Vector3f spec = ks.cwiseProduct(light.intensity / (r * r)) * std::pow(std::max(0.0f, normal.dot(h)), p);
        Vector3f diff = kd.cwiseProduct(light.intensity / (r * r)) * std::max(0.0f, normal.dot(l_n));

        result_color += (diff + spec);
    }

    result_color += ka.cwiseProduct(amb_light_intensity);

    return result_color * 255.f;
}



Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color; //漫反射系数
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};
    auto l3 = light{{-20, -20, -20}, {500, 500, 500}}; //调试：左下后方补光
    auto l4 = light{{20, -20, -20}, {500, 500, 500}}; //调试：右后下补光

    std::vector<light> lights = {l1, l2, l3, l4};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;
    
    // TODO: Implement displacement mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Position p = p + kn * n * h(u,v)
    // Normal n = normalize(TBN * ln)
    Eigen::Vector3f n = normal.normalized();
    float x = n.x(), y = n.y(), z = n.z();

    auto h = [&](const Eigen::Vector2f& uv) {
        return payload.texture->getColor(uv.x(), uv.y()).norm();
    }; 
    
    float w = payload.texture->width;
    float htex = payload.texture->height;

    float u = payload.tex_coords.x(), v =payload.tex_coords.y();
    float dU = kh * kn * (h(Eigen::Vector2f(u + 1.0f /w, v)) - h(Eigen::Vector2f(u, v)));
    float dV = kh * kn * (h(Eigen::Vector2f(u, v + 1.0f/ htex)) - h(Eigen::Vector2f(u, v)));

    Eigen::Vector3f t(x * y / std::sqrt(x * x + z * z),
                    std::sqrt(x * x + z * z),
                    z * y / std::sqrt(x * x + z * z));
    Eigen::Vector3f b = n.cross(t);

    Eigen::Matrix3f TBN;
    TBN.col(0) = t;
    TBN.col(1) = b;
    TBN.col(2) = n;

    //位移顶点（沿法线推）
    point = point + kn * n * h(Eigen::Vector2f(u, v));

    Eigen::Vector3f ln(-dU, -dV, 1.0f);
    normal = (TBN * ln).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        Eigen::Vector3f l_dir = light.position - point;
        float r = l_dir.norm();
        Eigen::Vector3f l_n = l_dir.normalized();
        Eigen::Vector3f v_dir = (eye_pos - point).normalized();
        Eigen::Vector3f h_dir = (l_n + v_dir).normalized();

        Eigen::Vector3f diff = kd.cwiseProduct(light.intensity / (r * r)) * std::max(0.0f, normal.dot(l_n));
        Eigen::Vector3f spec = ks.cwiseProduct(light.intensity / (r * r)) * std::pow(std::max(0.0f, normal.dot(h_dir)), p);
        result_color += diff + spec;
    }

    result_color += ka.cwiseProduct(amb_light_intensity);

    return result_color * 255.f;
}


Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color; //漫反射系数
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};
    auto l3 = light{{-20, -20, -20}, {500, 500, 500}}; //调试：左下后方补光
    auto l4 = light{{20, -20, -20}, {500, 500, 500}}; //调试：右后下补光

    std::vector<light> lights = {l1, l2, l3, l4};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;


    float kh = 0.2, kn = 0.1;
    //kh凹凸强度 kn扰动缩放

    // TODO: Implement bump mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Normal n = normalize(TBN * ln)
    Eigen::Vector3f n = normal.normalized();
    float x = n.x(), y = n.y(), z = n.z(); //当前法线

    auto h = [&](const Eigen::Vector2f& uv) {
        return payload.texture->getColor(uv.x(), uv.y()).norm();
    }; //高度函数：hmap.jpg 是灰度图，getColor三分量相等，取长度当高度
    
    float w = payload.texture->width;
    float htex = payload.texture->height; //纹理宽高（+1/w 表示在 UV 上走一个纹理像素）

    //有限差分求高度梯度（当前 UV）
    float u = payload.tex_coords.x(), v =payload.tex_coords.y();
    float dU = kh * kn * (h(Eigen::Vector2f(u + 1.0f /w, v)) - h(Eigen::Vector2f(u, v)));
    float dV = kh * kn * (h(Eigen::Vector2f(u, v + 1.0f/ htex)) - h(Eigen::Vector2f(u, v)));

     Eigen::Vector3f t(x * y / std::sqrt(x * x + z * z),
                    std::sqrt(x * x + z * z),
                    z * y / std::sqrt(x * x + z * z)); //切线向量
    Eigen::Vector3f b = n.cross(t); //副法线 b = n cross t

    //TBN矩阵
    Eigen::Matrix3f TBN;
    TBN.col(0) = t;
    TBN.col(1) = b;
    TBN.col(2) = n;

    //切线空间的扰动法线 ln，变回原空间并归一化
    Eigen::Vector3f ln(-dU, -dV, 1.0f);
    normal = (TBN * ln).normalized(); //覆盖原normal

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = normal;

    return result_color * 255.f;
}

int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 30.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "../models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);
    
    //hamp.jpg为高度图，spot_texture.png为纹理图
    //auto texture_path = "hmap.jpg"; 
    auto texture_path = "spot_texture.png";
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = texture_fragment_shader; //交互模式

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc == 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = displacement_fragment_shader;
        }
    }

    Eigen::Vector3f eye_pos = {0,0,10};

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        //r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10);

        if (key == 'a' )
        {
            angle += 5.0;
        }
        else if (key == 'd')
        {
            angle -= 5.0;
        }

    }
    return 0;
}
