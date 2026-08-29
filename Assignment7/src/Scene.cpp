//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection inter = intersect(ray);

    if(!inter.happened)
        return Vector3f(0, 0, 0);

    if(inter.m->hasEmission())
        return inter.emit;

    Vector3f L_dir(0, 0, 0);
    {
        Intersection light_inter;
        float light_pdf = 0;
        sampleLight(light_inter, light_pdf);

        Vector3f wi = light_inter.coords - inter.coords;
        float distance_sq = sqrt(dotProduct(wi, wi));
        wi = normalize(wi);

        Ray shadow_ray(inter.coords, wi);
        Intersection t = intersect(shadow_ray);

        if(t.happened && std::abs(distance_sq - t.distance) < 1e-2)
        {
            L_dir = inter.m->eval(ray.direction, wi, inter.normal) * light_inter.emit * dotProduct(wi, inter.normal) * dotProduct(-wi, light_inter.normal) / (distance_sq * distance_sq) / light_pdf;
        } else {
            L_dir = Vector3f(0, 0, 0);
        }
    }

    Vector3f L_indir(0, 0, 0);
    {
        float P_RR = 0.8;
        if(get_random_float() < P_RR)
        {
            Vector3f wi = inter.m->sample(ray.direction, inter.normal);
            Ray r(inter.coords, wi);
            Intersection t = intersect(r);

            if(t.happened && !t.m->hasEmission())
            {
                L_indir = castRay(r, depth + 1) * inter.m->eval(ray.direction, wi, inter.normal) * dotProduct(wi, inter.normal) / inter.m->pdf(ray.direction, wi, inter.normal) / P_RR;
            }
        }
    }

    return L_dir + L_indir;
}