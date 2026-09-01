#pragma once
#include <cmath>
#include <numbers>

#include "vec2.hpp"


namespace simulation {


struct Transform {
    Vec2 pos;  // meter
    Vec2 size;  // meter
    float angle = 0;  // radians (0 = rightside)

    Transform(Vec2 pos, Vec2 size): pos(pos), size(size) {}

    // getter
    float width() const { return size.x; }
    float height() const { return size.y; }

    float top() const {
        return pos.y + size.y;
    }
    float bottom() const {
        return pos.y;
    }
    float left() const {
        return pos.x;
    }
    float right() const {
        return pos.x + size.x;
    }
    float centerX() const {
        return pos.x + size.x/2;
    }
    float centerY() const {
        return pos.y + size.y/2;
    }
    Vec2 center() const {
        return pos + size/2;
    }

    // setter
    void setCenter(Vec2 center) { pos = center - size/2; }
    void setCenter(float xPos, float yPos) { setCenter({xPos, yPos}); }
    void move(Vec2 vec) { pos += vec; }
    void rotate(float angle) { this->angle += angle; }

    void setTop(float y) {
        pos.y = y - size.y;
    }
    void setBottom(float y) {
        pos.y = y;
    }
    void setRight(float x) {
        pos.x = x - size.x;
    }
    void setLeft(float x) {
        pos.x = x;
    }

    void setCenterX(float x) {
        pos.x = x - 0.5f * size.x;
    }
    void setCenterY(float y) {
        pos.y = y - 0.5f * size.y;
    }

    // utils units
    Vec2 rightUnit() const {
        return {std::cos(angle), std::sin(angle)};
    }
    Vec2 upUnit() const {
        float top_angle = angle + static_cast<float>(std::numbers::pi)/2;
        return {std::cos(top_angle), std::sin(top_angle)};
    }
    Vec2 leftUnit() const {
        return -rightUnit();
    }
    Vec2 downUnit() const {
        return -upUnit();
    }


};


}


