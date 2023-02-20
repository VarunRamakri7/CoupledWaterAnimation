#include "Aabb.h"

aabb2D::aabb2D(glm::vec2 box_min, glm::vec2 box_max):mMin(box_min), mMax(box_max)
{

}

aabb2D::aabb2D():mMin(+1e20f), mMax(-1e20f)
{

}
