#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace OpenApoc
{
#if (GLM_VERSION_MAJOR == 0 && GLM_VERSION_MINOR <= 9 && GLM_VERSION_PATCH <= 5)
template <typename T> using Vec3 = glm::detail::tvec3<T, glm::highp>;

template <typename T> using Vec2 = glm::detail::tvec2<T, glm::highp>;
#else
template <typename T> using Vec3 = glm::tvec3<T, glm::highp>;

template <typename T> using Vec2 = glm::tvec2<T, glm::highp>;
#endif

static inline float mix(float a, float b, float factor) { return a * (1.0f - factor) + b * factor; }
}
