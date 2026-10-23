#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include "vectors.hpp"

namespace heart_xd_detail
{
    constexpr double pi = 3.14159265358979323846;
    struct V
    {
        double x, y;
    };
    inline V operator+(V a, V b) { return {a.x + b.x, a.y + b.y}; }
    inline V operator-(V a, V b) { return {a.x - b.x, a.y - b.y}; }
    inline V operator*(V a, double b) { return {a.x * b, a.y * b}; }
    inline double dot(V a, V b) { return a.x * b.x + a.y * b.y; }
    inline double length(V a) { return std::sqrt(dot(a, a)); }
    inline double smooth(double a, double b, double x)
    {
        double u = std::clamp((x - a) / (b - a), 0.0, 1.0);
        return u * u * (3.0 - 2.0 * u);
    }
    inline double segment(V p, V a, V b)
    {
        V v = b - a;
        double vv = dot(v, v);
        double u = vv > 1e-20 ? std::clamp(dot(p - a, v) / vv, 0.0, 1.0) : 0.0;
        return length(p - a - v * u);
    }
    inline double bezier(V pos, V A, V B, V C)
    {
        V a = B - A, b = A - B * 2.0 + C, c = a * 2.0, d = A - pos;
        double bb = dot(b, b);
        if (bb < 1e-20)
            return segment(pos, A, C);
        double kk = 1.0 / bb, kx = kk * dot(a, b);
        double ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0, kz = kk * dot(d, a);
        double p = ky - kx * kx, q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
        double h = q * q + 4.0 * p * p * p;
        if (h >= 0.0)
        {
            h = std::sqrt(h);
            double u = std::clamp(std::cbrt((h - q) * 0.5) + std::cbrt((-h - q) * 0.5) - kx, 0.0, 1.0);
            return length(d + (c + b * u) * u);
        }
        double z = std::sqrt(-p);
        double v = std::acos(std::clamp(q / (2.0 * p * z), -1.0, 1.0)) / 3.0;
        double m = std::cos(v), n = std::sin(v) * 1.732050808;
        double roots[] = {2.0 * m * z - kx, (-n - m) * z - kx, (n - m) * z - kx};
        double best = 1e30;
        for (double u : roots)
        {
            u = std::clamp(u, 0.0, 1.0);
            V delta = d + (c + b * u) * u;
            best = std::min(best, dot(delta, delta));
        }
        return std::sqrt(best);
    }
    inline V heart(double a)
    {
        double s = std::sin(a);
        return {16.0 * s * s * s, -(13.0 * std::cos(a) - 5.0 * std::cos(2 * a) - 2.0 * std::cos(3 * a) - std::cos(4 * a))};
    }
    struct Cache
    {
        double time = std::numeric_limits<double>::quiet_NaN();
        std::array<V, 8> pink, blue;
        void update(double t)
        {
            if (time == t)
                return;
            time = t;
            double phase = -0.5 * t;
            phase = (phase - std::floor(phase)) * 6.28;
            for (int i = 0; i < 8; ++i)
            {
                pink[i] = heart(i * 0.25 + phase) * 0.012;
                blue[i] = heart(3.4 + i * 0.25 + phase) * 0.012;
            }
        }
    };
    inline double trail(V p, const std::array<V, 8> &points)
    {
        V c = (points[0] + points[1]) * 0.5;
        double dist = 1e4;
        for (int i = 0; i < 7; ++i)
        {
            V previous = c;
            c = (points[i] + points[i + 1]) * 0.5;
            dist = std::min(dist, bezier(p, previous, points[i], c));
        }
        return dist;
    }
    inline double lettering(V p)
    {
        double d = segment(p, {-0.075, 0.025}, {-0.027, -0.025});
        d = std::min(d, segment(p, {-0.075, -0.025}, {-0.027, 0.025}));
        d = std::min(d, segment(p, {0.002, -0.039}, {0.002, 0.039}));
        d = std::min(d, segment(p, {0.002, 0.039}, {0.024, 0.039}));
        d = std::min(d, bezier(p, {0.024, 0.039}, {0.076, 0.039}, {0.076, 0.0}));
        d = std::min(d, bezier(p, {0.076, 0.0}, {0.076, -0.039}, {0.024, -0.039}));
        return std::min(d, segment(p, {0.024, -0.039}, {0.002, -0.039}));
    }
}

inline const auto &shader4 = [](vec2 FC, vec2 r, double t, vec4 &o)
{
    using namespace heart_xd_detail;
    thread_local Cache cache;
    cache.update(t);

    V screen = {(FC.x - r.x * 0.5) / r.x, (FC.y - r.y * 0.5) / r.x};
    V pos = {-screen.x, -screen.y + 0.03};
    double dp = trail(pos, cache.pink), db = trail(pos, cache.blue);
    double gp = std::pow(0.015 / std::max(dp, 1e-12), 1.3);
    double gb = std::pow(0.015 / std::max(db, 1e-12), 1.3);
    double core = 10.0 * (1.0 - smooth(0.003, 0.006, dp)) + 10.0 * (1.0 - smooth(0.003, 0.006, db));
    double red = core + gp + gb * 0.1;
    double green = core + gp * 0.05 + gb * 0.4;
    double blue = core + gp * 0.3 + gb;

    // Le texte bouge autour du centre et respire, sans devenir illisible.
    double phase = pi * t;
    double pulse = 0.5 + 0.5 * std::sin(2.0 * phase);
    double zoom = 1.0 + 0.045 * std::sin(2.0 * phase);
    V text = {screen.x, (screen.y - 0.008 - 0.005 * std::sin(phase))};
    double angle = 0.035 * std::sin(phase);
    double ca = std::cos(angle), sa = std::sin(angle);
    text = {(ca * text.x + sa * text.y) / zoom, (-sa * text.x + ca * text.y) / zoom};
    double distance = lettering(text);
    double aa = 0.85 / (r.x * zoom);
    double stroke = 1.0 - smooth(0.0028 - aa, 0.0028 + aa, distance);
    double bloom = std::exp(-distance * 155.0) * (0.38 + 0.20 * pulse);
    double tint = 0.5 + 0.5 * std::sin(phase + text.x * 18.0);
    double shine = 2.5 * stroke;
    red += shine + bloom * (0.15 + 0.85 * tint);
    green += shine + bloom * (0.7 - 0.55 * tint);
    blue += shine + bloom;

    // Meme transformation tonale que le shader d'origine.
    std::uint32_t x = static_cast<std::uint32_t>(FC.x);
    std::uint32_t y = static_cast<std::uint32_t>(FC.y);
    std::uint32_t hash = x * 374761393u + y * 668265263u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    hash ^= hash >> 16;
    double noise = 2.0 * (double(hash) / 4294967295.0) - 1.0;
    noise = std::copysign(1.0 - std::sqrt(std::max(0.0, 1.0 - std::abs(noise))), noise) / 255.0;
    auto finish = [noise](double c)
    {
        return std::clamp(std::pow(-std::expm1(-c), 0.4545) + noise, 0.0, 1.0);
    };
    o = {finish(red), finish(green), finish(blue), 1.0};
};
