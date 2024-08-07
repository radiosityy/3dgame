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
    void jump();

    void wave();

    void rotateY(float a);

    void walk(const vec3&);
    void stop();

private:
    vec3 forward() const;
    vec3 right() const;

    bool m_walking_forward = false;
    bool m_walking_back = false;
    bool m_walking_left = false;
    bool m_walking_right = false;
    bool m_turning_right = false;
    bool m_turning_left = false;

    bool m_walking = false;
    bool m_rotating = false;
    vec3 m_moving_direction = vec3(0.0f, 0.0f, 1.0f);
    quat m_target_rotation;

    static constexpr inline float speed = 15.0f;
    static constexpr inline float rot_speed = 5.0 * pi;
};

#endif // PLAYER_H
