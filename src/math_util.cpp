#include "math_util.hpp"

float Complex::magnitude() const
{
    return sqrtf(real*real+imaginary*imaginary);
}

float Complex::winding() const
{
	return atan2f(imaginary, real);
}

float snap_value(float val, float bound1, float bound2, float threshold)
{
  if (fabsf(val - bound1) <= threshold) {
    val = bound1;
  }
  else if (fabsf(val - bound2) <= threshold) {
    val = bound2;
  }
  else if (fabsf(val - (bound1 + bound2) / 2) <= threshold) {
    val = (bound1 + bound2) / 2;
  }

  return val;
}

Color::Color(const ColorF& color) {
    float coef = 255.0;
    r = int(color.r * coef);
    g = int(color.g * coef);
    b = int(color.b * coef);
    a = int(color.a * coef);
}

ColorF::ColorF(const Color& color) {
    float coef = 1.0 / 255.0;
    r = (float)color.r * coef;
    g = (float)color.g * coef;
    b = (float)color.b * coef;
    a = (float)color.a * coef;
}

vec2 lerp2(vec2 a, vec2 b, float t)
{
    return vec2(std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t));
}

vec2 reflect2(vec2 incident, vec2 normal)
{
    return incident - 2.0f * dot2(normal, incident) * normal;
}

vec2 get_direction_vector(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    return vec2(c, s);
}

Rectangle merge_volumes(Rectangle v1, Rectangle v2)
{
    Rectangle res = {};
    vec2 p = vec2(v1.x, v1.y) - vec2(v1.w / 2, v1.h / 2);
    vec2 q = vec2(v2.x, v2.y) - vec2(v2.w / 2, v2.h / 2);
    vec2 r = vec2(v1.x, v1.y) + vec2(v1.w / 2, v1.h / 2);
    vec2 w = vec2(v2.x, v2.y) + vec2(v2.w / 2, v2.h / 2);

    vec2 min = vec2(cobot::min(p.x, q.x), cobot::min(p.y, q.y));
    vec2 max = vec2(cobot::max(r.x, w.x), cobot::max(r.y, w.y));

    res.x = (min.x + max.x) / 2;
    res.y = (min.y + max.y) / 2;
    res.w = max.x - min.x;
    res.h = max.y - min.y;

    return res;
}

bool Rectangle::contains_top_left(vec2 p) const
{
    return p.x >= x && p.x <= x + w &&
        p.y >= y && p.y <= y + h;
}

bool Rectangle::contains_centered(vec2 p) const
{
    return p.x >= x - w / 2 && p.x <= x + w / 2 &&
        p.y >= y - h / 2 && p.y <= y + h / 2;
}
