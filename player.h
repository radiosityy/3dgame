#ifndef PLAYER_H
#define PLAYER_H

#include "object.h"

class Player : public Object
{
public:
    using Object::Object;

    void update(Renderer& renderer, float dt);

    void walkForward(bool);
    void walkBack(bool);
    void walkRight(bool);
    void walkLeft(bool);
    void turnRight(bool);
    void turnLeft(bool);

    void wave();

    vec3 forward() const;
    void setForward(const vec3&);
    vec3 right() const;

    vec3 walkDir() const;
    void setWalkDir(const vec3&);
    void walk(const vec3&);
    void stop();

private:
    bool m_walking_forward = false;
    bool m_walking_back = false;
    bool m_walking_left = false;
    bool m_walking_right = false;

    bool m_walking = false;
    vec3 m_forward = vec3(0.0f, 0.0f, 1.0f);
    vec3 m_right = vec3(1.0f, 0.0f, 0.0f);
    vec3 m_walk_dir = vec3(0.0f, 0.0f, 0.0f);

    vec3 m_velocityP = vec3(0.0f, 0.0f, 0.0f);
    float m_speed = 15.0f;
};

#endif // PLAYER_H
