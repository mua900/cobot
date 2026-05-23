#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

#include <cmath>

float snap_value(float val, float bound1, float bound2, float threshold);

struct ivec2 {
    int x, y;
};

struct vec2 {
    float x = 0, y = 0;
    vec2() {}
    vec2(float p_x, float p_y) : x(p_x), y(p_y) {}
    vec2(float p) : x(p), y(p) {}

    vec2 normalized() const
    {
        float mag = sqrt(x*x+y*y);
        return vec2(x/mag,y/mag);
    }

    float magnitude() const {
        return sqrtf(x * x + y * y);
    }

    void operator+=(const vec2 other)
    {
        x += other.x;
        y += other.y;
    }

    void operator-=(const vec2 other)
    {
        x -= other.x;
        y -= other.y;
    }

    void operator/=(float s)
    {
        x /= s;
        y /= s;
    }

    void operator*=(float s)
    {
        x *= s;
        y *= s;
    }
};

inline float dot2(vec2 a, vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline vec2 operator-(const vec2 v)
{ return vec2(-v.x, -v.y); }

inline vec2 operator+(const vec2 a, const vec2 b)
{
    return vec2(a.x + b.x, a.y + b.y);
}
inline vec2 operator-(const vec2 a, const vec2 b)
{
    return vec2(a.x - b.x, a.y - b.y);
}
inline vec2 operator*(vec2 v, float s)
{
    return vec2(v.x * s, v.y * s);
}
inline vec2 operator*(float s, vec2 v)
{
    return vec2(v.x * s, v.y * s);
}
inline vec2 operator/(vec2 v, float s)
{
    return vec2(v.x / s, v.y / s);
}
inline vec2 operator*(vec2 a, vec2 b)
{
    return vec2(a.x * b.x, a.y * b.y);
}

vec2 lerp2(vec2 a, vec2 b, float t);
vec2 reflect2(vec2 incident, vec2 normal);
vec2 get_direction_vector(float angle);

struct ColorF;

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
    Color() {}
    Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b), a(0xff) {}
    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) {}
    Color(const ColorF& color);
};

struct ColorF {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    ColorF() {}
    ColorF(float r, float g, float b) : r(r), g(g), b(b), a(1.0) {}
    ColorF(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
    ColorF(const Color& color);
};

// simple custom complex number
struct Complex {
	float real = 0.0;
	float imaginary = 0.0;

	Complex() {}
    Complex(float r, float i) : real(r), imaginary(i) {}

    float magnitude() const;
    float winding() const;
};

// overloads for complex

inline Complex operator+(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real + rhs.real, lhs.imaginary + rhs.imaginary);
}

inline Complex operator-(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real - rhs.real, lhs.imaginary - rhs.imaginary);
}

inline Complex operator*(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real * rhs.real - lhs.imaginary * rhs.imaginary, lhs.real * rhs.imaginary + lhs.imaginary * rhs.real);
}

#define CONSTANT_PI  3.14159265359
#define CONSTANT_E   2.71828182846
#define CONSTANT_TAU (CONSTANT_PI * 2.0)

struct Rectangle {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rectangle() {}
    Rectangle(vec2 pos, vec2 scale) : x(pos.x), y(pos.y), w(scale.x), h(scale.y) {}
    Rectangle(float p_x, float p_y, float p_w, float p_h)
        : x(p_x), y(p_y), w(p_w), h(p_h)
    {}

    bool contains_top_left(vec2 p) const;
    bool contains_centered(vec2 p) const;
    Rectangle to_top_left() const {
        return Rectangle(x - w / 2, y - h / 2, w, h);
    }
    Rectangle to_center() const {
        return Rectangle(x + w / 2, y + h / 2, w, h);
    }

    vec2 get_position() const {
        return vec2(x, y);
    }

    vec2 get_scale() const {
        return vec2(w, h);
    }

    vec2 get_top_left() const {
        return vec2(x - w / 2, y - h / 2);
    }
    vec2 get_center() const {
        return vec2(x + w / 2, y +h / 2);
    }
};

#define COLOR_WHITE ((Color){0xff,0xff,0xff,0xff})
#define COLOR_BLACK ((Color){0,0,0,0xff})
#define COLOR_RED   ((Color){0xff,0,0,0xff})
#define COLOR_GREEN ((Color){0,0xff,0,0xff})
#define COLOR_BLUE  ((Color){0,0,0xff,0xff})

#define COLOR_ARG(color) color.r,color.g,color.b,color.a

namespace cobot {
    constexpr int max(int x, int y) { return x > y ? x : y; }
    constexpr int min(int x, int y) { return x > y ? y : x; }
}

#endif // _MATH_UTIL_H
