#include "camera.hpp"

cobot::vec2 Camera::world_to_screen(cobot::vec2 p) const
{
    p = (p * zoom).rotated(rotation) - position;
    return cobot::vec2(p.x, -p.y) + offset;
}

cobot::vec2 Camera::screen_to_world(cobot::vec2 p) const
{
    p = p - offset;
    return (cobot::vec2(p.x, -p.y) + position).rotated(-rotation) / zoom;
}
