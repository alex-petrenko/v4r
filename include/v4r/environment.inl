#ifndef V4R_ENVIRONMENT_INL_INCLUDED
#define V4R_ENVIRONMENT_INL_INCLUDED

#include <iostream>

#include <v4r/environment.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace v4r {

uint32_t Environment::addInstance(uint32_t model_idx,
                                  uint32_t material_idx,
                                  const glm::mat4x4& matrix)
{
    return addInstance(model_idx, material_idx, glm::mat4x3(matrix));
}

const glm::mat4x3& Environment::getInstanceTransform(uint32_t inst_id) const
{
    const auto& p = index_map_[inst_id];
    return transforms_[p.first][p.second];
}

void Environment::updateInstanceTransform(uint32_t inst_id,
                                          const glm::mat4x3& mat)
{
    const auto& p = index_map_[inst_id];
    transforms_[p.first][p.second] = mat;
}

void Environment::updateInstanceTransform(uint32_t inst_id,
                                          const glm::mat4& mat)
{
    updateInstanceTransform(inst_id, glm::mat4x3(mat));
}

void Environment::setInstanceMaterial(uint32_t inst_id, uint32_t material_idx)
{
    const auto& p = index_map_[inst_id];
    materials_[p.first][p.second] = material_idx;
}

uint32_t Environment::addCamera(const glm::mat4& mat)
{
    views_.emplace_back(mat);
    actives_.emplace_back(1);
    return views_.size() - 1;
}

void Environment::setCameraActive(uint32_t camera_idx, uint8_t is_active)
{
    actives_[camera_idx] = is_active;
}

void Environment::setCameraView(uint32_t camera_idx,
                                const glm::vec3& eye,
                                const glm::vec3& look,
                                const glm::vec3& up)
{
    setCameraView(camera_idx, glm::lookAt(eye, look, up));
}

const glm::mat4& Environment::getCameraView(uint32_t camera_idx) const
{
    return views_[camera_idx];
}

void Environment::setCameraView(uint32_t camera_idx, const glm::mat4& m)
{
    views_[camera_idx] = m;
}

void Environment::rotateCamera(uint32_t camera_idx,
                               float angle,
                               const glm::vec3& axis)
{
    views_[camera_idx] = glm::rotate(views_[camera_idx], angle, axis);
}

void Environment::translateCamera(uint32_t camera_idx, const glm::vec3& v)
{
    views_[camera_idx] = glm::translate(views_[camera_idx], v);
}

}  // namespace v4r

#endif
