#pragma once
#include <cmath>

namespace simulation {


struct Vec2 {
    float x;
    float y;

    bool isZero() const {
        return (x == 0.0f) && (y == 0.0f);
    }
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    Vec2 normalized() const {
        if (isZero()) return {0, 0};
        return (*this) / length();
    }

    Vec2 operator + (Vec2 other) const {
        return {x + other.x, y + other.y};
    }
    Vec2 operator - (Vec2 other) const {
        return {x - other.x, y - other.y};
    }
    Vec2 operator * (Vec2 other) const {
        return {x * other.x, y * other.y};
    }
    Vec2 operator * (float num) const {
        return {x * num, y * num };
    }
    Vec2 operator / (float num) const {
        return {x / num, y / num };
    }

    void operator += (Vec2 other) {
        x += other.x;
        y += other.y;
    }
    void operator -= (Vec2 other) {
        x -= other.x;
        y -= other.y;
    }
    void operator *= (float num) {
        x *= num;
        y *= num;
    }
    void operator /= (float num) {
        x /= num;
        y /= num;
    }

    bool operator == (Vec2 other) const {
        return (x == other.x) && (y == other.y);
    }
    bool operator != (Vec2 other) const {
        return !((*this) == other);
    }

    Vec2 operator - () const {
        return {-x, -y};
    }
};

inline float dot(Vec2 v1, Vec2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

struct SizeInt {
    size_t width;
    size_t height;

    Vec2 toVec() const {
        return {static_cast<float>(width), static_cast<float>(height)};
    }
};


}