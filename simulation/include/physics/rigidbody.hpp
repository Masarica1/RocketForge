#pragma once
#include "vec2.hpp"

namespace simulation {


class RigidBody {
private:
    Vec2 velocity_ = {0, 0};
    Vec2 force_ = {0, 0};

    float angularVel_ = 0;
    float torque_ = 0;

public:
    float mass;
    float moi;

    RigidBody(float mass, float moi): mass(mass), moi(moi) {}

    // setter
    void addForce(Vec2 f) {force_ += f;}
    void setForce(Vec2 f) {force_ = f;}
    void addTorque(float t) {torque_ += t;}
    void setTorque(float t) {torque_ = t;}

    void setLinearVel(Vec2 v) {velocity_ = v;}
    void setAngularVel(float v) {angularVel_ = v;}

    void reset() {
        velocity_ = {0., 0.};
        force_ = {0., 0.};
        angularVel_ = 0;
        torque_ = 0;
    }

    // getter
    Vec2 linearVel() const { return velocity_; }
    float angularVel() const { return angularVel_; }

    Vec2 linearAcc() const { return force_ / mass;}
    float angularAcc() const { return torque_ / moi; }

    // applier
    void update(const float dt) {
        velocity_ += linearAcc() * dt;
        angularVel_ += angularAcc() * dt;

        force_ = {0.0f, 0.0f};
        torque_ = 0.0f;
    }

};


}