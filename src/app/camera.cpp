#include "camera.hpp"

cobot::vec2 Camera::world_to_screen(cobot::vec2 p) const
{
    return (p * zoom).rotated(rotation) - position;
}

cobot::vec2 Camera::screen_to_world(cobot::vec2 p) const
{
    return (p + position).rotated(-rotation) / zoom;
}
