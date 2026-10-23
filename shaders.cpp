#include <algorithm>
#include <cmath>
#include <cstdint>

#include "xD.hpp"
#include "vectors.hpp"

const auto& shader1 = [](vec2 FC, vec2 r, double t, vec4 &o)
{
    const double SPEED = 1.0;
    const double ZOOM = 1.0;
    const double GLOW = 5.0;

    t *= SPEED;
    vec2 p = (FC * 2.0 - r) / r.y * ZOOM;
    vec2 l = {0.0, 0.0};
    l += 4.0 - 4.0 * std::abs(0.7 - dot(p, p));
 
    vec2 v = p * l;
    #pragma GCC unroll 8
    for (int k = 1; k <= 8; ++k)
    {
        double iy = k;
        vec2 i = {0.0, iy};
        v += cos(v.yx() * iy + i + t) / iy + 0.7;
        o += (sin(_xyyx(v)) + 1.0) * std::abs(v.x - v.y);
    }
 
    vec4 c = {-1.0, 1.0, 2.0, 0.0};
    vec4 exponent = c * (-p.y) + (l.x - 4.0);
    o = tanh(exp(exponent) * GLOW / o);
};

const auto& shader2 = [](vec2 FC, vec2 r, double t, vec4 &o)
{
    vec3 p;
    vec4 tmp = {0, 1, 2, 0};
    vec3 n = normalize(_xyy(r)*-1 + FC*2.);
    for(double i=.0,z=.0,d=.0,l=.0;i++<8e1;o+=(cos(tmp+log(l)*3.-t)+1.)/d)
        p=n*z,p.z+=.1,p/=l=dot(p,p),z+=d=sqrt(l)/80.*length(cos(p+cos(p/.27+t))+.8);   
    o=tanh(o/2e5);
};

const auto& shader3 = [](vec2 FC, vec2 r, double t, vec4 &o)
{
    const double SCROLL = 0.3;
    const double SPEED = 0.5;
    vec2 s = (FC * 2.0 - r) / r.y;
    double v_dist = (s.y + 1.0) * (s.y + 1.0) * 0.25;
    s.y -= 1.2;
    double per = 2.0 / std::abs(s.y);
    vec3 col = {0.0, 0.0, 0.0};
    for(double z = 0.0; z < 1.0; z += 0.1) {
        double d = 1.0 + 0.4 * z;
        vec2 p = vec2{s.x * d, s.y + d} * per;
        vec2 uv = p;
        uv.y += SCROLL * t;
        double shift = std::cos(z / 0.1);
        double wave = ncos(uv.y) + ncos(uv.y * 0.6);
        uv.x += shift + (wave) / (1.0 + 0.01 * per * per);
        double w = uv.x;
        double l = std::sin(uv.y * 0.5 + z / 0.1 + SPEED * t * sign(shift));
        double intensity = std::exp(std::min(l, -l / 0.3 / (1.0 + 4.0 * w * w)));
        vec3 colorA = {1.0, 0.2, 0.2};
        vec3 colorB = {1.0, 1.0, 0.9};
        double m = std::tanh(shift / 0.1) * 0.5 + 0.5;
        vec3 layerCol = mix(colorA, colorB, m);
        col += layerCol * (intensity / (std::abs(w) + 0.01 * per) * per);
    }
    col.x = std::tanh(col.x / 20.0);
    col.y = std::tanh(col.y / 20.0);
    col.z = std::tanh(col.z / 20.0);
    o = {col.x * col.x, col.y * col.y, col.z * col.z, 1.0};
};

const auto& shader5 = [](vec2 FC, vec2 r, double t, vec4& o)
{
    constexpr int POINT_COUNT = 8;
    constexpr double speed = -0.5;
    constexpr double len = 0.25;
    constexpr double scale = 0.012;
    constexpr double intensity = 1.3;
    constexpr double radius = 0.015;

    auto sdBezier = [](vec2 pos, vec2 A, vec2 B, vec2 C) -> double
    {
        vec2 a = B - A;
        vec2 b = A - B * 2.0 + C;
        vec2 c = a * 2.0;
        vec2 d = A - pos;

        double bb = dot(b+b, b*b);

        if (bb < 1e-20) {
            vec2 ab = C - A;
            double denominator = dot(ab, ab);

            double u = denominator > 0.0
                ? std::clamp(dot(pos - A, ab) / denominator, 0.0, 1.0)
                : 0.0;

            vec2 delta = A + ab * u - pos;
            return std::sqrt(dot(delta, delta));
        }

        double kk = 1.0 / bb;
        double kx = kk * dot(a, b);
        double ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0;
        double kz = kk * dot(d, a);

        double p = ky - kx * kx;
        double q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
        double h = q * q + 4.0 * p * p * p;

        if (h >= 0.0) {
            h = std::sqrt(h);

            double u = std::cbrt((h - q) * 0.5);
            double v = std::cbrt((-h - q) * 0.5);
            double s = std::clamp(u + v - kx, 0.0, 1.0);

            vec2 delta = d + (c + b * s) * s;
            return std::sqrt(dot(delta, delta));
        }

        double z = std::sqrt(-p);
        double angle = std::acos(
            std::clamp(q / (2.0 * p * z), -1.0, 1.0)
        ) / 3.0;

        double m = std::cos(angle);
        double n = std::sin(angle) * 1.732050808;

        double roots[3] = {
            2.0 * m * z - kx,
            (-n - m) * z - kx,
            (n - m) * z - kx
        };

        double distanceSquared = 1e30;

        for (double root : roots) {
            double s = std::clamp(root, 0.0, 1.0);
            vec2 delta = d + (c + b * s) * s;

            distanceSquared = std::min(
                distanceSquared, dot(delta, delta)
            );
        }

        return std::sqrt(distanceSquared);
    };

    auto getHeartPosition = [](double angle) -> vec2
    {
        double s = std::sin(angle);

        return {
            16.0 * s * s * s,
            -(13.0 * std::cos(angle)
              - 5.0 * std::cos(2.0 * angle)
              - 2.0 * std::cos(3.0 * angle)
              - std::cos(4.0 * angle))
        };
    };

    auto getSegment = [&](vec2 pos, double offset) -> double
    {
        vec2 points[POINT_COUNT];

        double phase = speed * t;
        phase = (phase - std::floor(phase)) * 6.28;

        for (int i = 0; i < POINT_COUNT; ++i) {
            points[i] = getHeartPosition(offset + i * len + phase);
        }

        vec2 c = (points[0] + points[1]) * 0.5;
        double dist = 10000.0;

        for (int i = 0; i < POINT_COUNT - 1; ++i) {
            vec2 previous = c;
            c = (points[i] + points[i + 1]) * 0.5;

            dist = std::min(
                dist,
                sdBezier(pos, previous * scale,
                         points[i] * scale, c * scale)
            );
        }

        return std::max(0.0, dist);
    };

    auto whiteCore = [](double dist) -> double
    {
        double u = std::clamp((0.006 - dist) / 0.003, 0.0, 1.0);
        return 10.0 * u * u * (3.0 - 2.0 * u);
    };

    vec2 pos = {
        0.5 - FC.x / r.x,
        (0.5 - FC.y / r.y) / (r.x / r.y) + 0.03
    };

    double distPink = getSegment(pos, 0.0);
    double distBlue = getSegment(pos, 3.4);

    double glowPink = std::pow(
        radius / std::max(distPink, 1e-12), intensity
    );
    double glowBlue = std::pow(
        radius / std::max(distBlue, 1e-12), intensity
    );

    double core = whiteCore(distPink) + whiteCore(distBlue);

    vec3 greenColor = {0.0, 1.0, 0.0};
    vec3 redColor = {1.0, 0.0, 0.0};
    vec3 yellowColor = {1.0, 1.0, 0.0};

    vec3 pinkColor = redColor;
    vec3 blueColor = greenColor;

    vec3 col = {
        core + glowPink * pinkColor.x + glowBlue * blueColor.x,
        core + glowPink * pinkColor.y + glowBlue * blueColor.y,
        core + glowPink * pinkColor.z + glowBlue * blueColor.z
    };

    std::uint32_t x = static_cast<std::uint32_t>(FC.x);
    std::uint32_t y = static_cast<std::uint32_t>(FC.y);
    std::uint32_t hash = x * 374761393u + y * 668265263u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    hash ^= hash >> 16;

    double noise = 2.0 * (double(hash) / 4294967295.0) - 1.0;

    noise = std::copysign(
        1.0 - std::sqrt(std::max(0.0, 1.0 - std::abs(noise))),
        noise
    ) / 255.0;

    auto finish = [noise](double channel) -> double
    {
        double mapped = 1.0 - std::exp(-channel);
        return std::pow(mapped, 0.4545) + noise;
    };

    o = {finish(col.x), finish(col.y), finish(col.z), 1.0};
};

const auto& shader6 = [](vec2 FC, vec2 r, double t, vec4& o) {
    vec2 p;

    for (double i = -1.; i < 1.; i += .1) {
        p = (FC.xy() * 2. - r) / r.y + i;
        double a = t - i - length(p) * .3;
        // Animate the pattern itself: length(q) nearly cancels the rotation below.
        vec2 v = p * sin(p / .2 + t);
        vec2 q = {v.x * cos(a) + v.y * cos(a - 33.), v.x * cos(a - 11.) + v.y * cos(a) };
        vec4 tm = {0., 1., 2., 0.};
        o += cos(tm + i/.3 + 1.)
             / (length(q) + i * i);  
    }

    vec4 tmp_o = o*o;
    o = tanh(tmp_o / 4e2);
}; 

static inline unsigned char toByte(double value)
{ 
    return static_cast<unsigned char>(
        std::clamp(value, 0.0, 1.0) * 255.0);
}

extern "C" void render(unsigned char* pixels, int width, int height, double t)
{
    const vec2 resolution = {double(width), double(height)};
    
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            vec4 color = {0.0, 0.0, 0.0, 0.0};
            vec2 position = {x + 0.5, height - y - 0.5};
            
            shader5(position, resolution, t, color);
  
            int i = (y * width + x) * 3;
            pixels[i]     = toByte(color.x);
            pixels[i + 1] = toByte(color.y);
            pixels[i + 2] = toByte(color.z);
        }
    }
}
