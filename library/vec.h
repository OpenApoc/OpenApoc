#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace OpenApoc
{
template <typename T>
using Vec3 = glm::detail::tvec3<T, glm::highp>;

template <typename T>
using Vec2 =  glm::detail::tvec2<T, glm::highp>;

}
