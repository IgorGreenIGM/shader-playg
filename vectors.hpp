#ifndef __VECTORS_HPP_INCLUDED__
#define __VECTORS_HPP_INCLUDED__

#include <cmath>
#include <iostream>

struct vec2
{
    double x;
    double y;

    vec2 yx() { return {y, x}; }
    vec2 xy() { return {x, y}; }
    
    vec2 operator*(double f) { return {x * f, y * f}; }
    vec2 operator/(double f) { return {x / f, y / f}; }
    vec2 operator-(double f) { return {x - f, y - f}; }
    vec2 operator+(double f) { return {x + f, y + f}; }
    vec2 operator-(const vec2 &other) { return {x - other.x, y - other.y}; }
    vec2 operator+(const vec2 &other) { return {x + other.x, y + other.y}; }
    vec2 operator*(const vec2 &other) { return {x * other.x, y * other.y}; }
    vec2& operator+=(const vec2 &other){ x += other.x; y += other.y; return *this; }
    vec2& operator+=(double f){ x += f; y += f; return *this; }
};

inline vec2 cos(const vec2 v) { return {cos(v.x), cos(v.y)}; }
inline vec2 sin(const vec2 v) { return {sin(v.x), sin(v.y)}; }
inline vec2 abs(const vec2 &v) { return {std::abs(v.x), std::abs(v.y)}; }
inline double length(const vec2& v) { return sqrt(v.x*v.x + v.y*v.y); }
inline double dot(const vec2 &v1, const vec2 &v2) { return {v1.x * v2.x + v1.y * v2.y}; };


struct vec3
{
    double x;
    double y;
    double z;
    
    vec3 operator/(double f) { return {x / f, y / f, z / f}; }
    vec3 operator+(double f) { return {x + f, y + f, z + f}; }
    vec3 operator*(double f) { return {x * f, y * f, z * f}; }
    vec3 operator+(const vec2 &other) { return {x + other.x, y + other.y, z}; }
    vec3 operator+(const vec3 &other) { return {x + other.x, y + other.y, z + other.z}; }
    vec3& operator/=(double f){ x /= f; y /= f; z /= f; return *this; }
    vec3& operator+=(const vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
};

inline vec3 _xyy(const vec2& v) { return {v.x, v.y, v.y}; }
inline vec3 cos(const vec3& v) { return {cos(v.x), cos(v.y), cos(v.z)}; }
inline double dot(const vec3 &v1, const vec3 &v2) { return {v1.x * v2.x + v1.y * v2.y + v1.z * v2.z}; };
inline double length(const vec3& v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
inline vec3 normalize(const vec3& v) { auto l = length(v); return {v.x/l, v.y/l, v.z/l}; }
inline vec3 mix(vec3 a, vec3 b, double t) { return a * (1.0 - t) + b * t; }


struct vec4
{
    double x;
    double y;
    double z;
    double w;
    
    vec2 yx() { return {y, x}; }
    vec2 xy() { return {x, y}; }
    vec4 xyyx() { return {x, y, y, x}; }
    vec4 operator+(double f) { return {x + f, y + f, z + f, w + f}; }
    vec4 operator/(double f) { return {x / f, y / f, z / f, w / f}; }
    vec4 operator-(double f) { return {x - f, y - f, z - f, w - f}; }
    vec4 operator*(double f) { return {x * f, y * f, z * f, w * f}; }
    vec4 operator*(const vec4& other) {return {x * other.x, y * other.y, z * other.z, w * other.w};}
    vec4& operator=(const vec2& other) { x=other.x; y= other.y; return *this; }
    vec4& operator+=(const vec2& other){ x += other.x; y += other.y; return *this; }
    vec4& operator+=(const vec4& other){ x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    vec4 operator/(const vec4& other){return {x/other.x, y / other.y, z / other.z, w / other.w}; }
};

inline vec4 _xyyx(vec2 v) { return {v.x, v.y, v.y, v.x}; }
inline vec4 sin(const vec4& v) { return {sin(v.x), sin(v.y), sin(v.z), sin(v.w)}; }
inline vec4 cos(const vec4& v) { return {cos(v.x), cos(v.y), cos(v.z), cos(v.w)}; }
inline vec4 exp(const vec4& v) { return {exp(v.x), exp(v.y), exp(v.z), exp(v.w)}; }
inline vec4 tanh(const vec4& v) { return {tanh(v.x), tanh(v.y), tanh(v.z), tanh(v.w)}; }
inline double length(const vec4& v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }


inline double sign(double x) { return (x > 0.0) - (x < 0.0); }
inline double ncos(double x) { return std::cos(x) / (0.5 + 0.4 * std::abs(std::cos(x))); }

#endif // __VECTORS_HPP_INCLUDED__