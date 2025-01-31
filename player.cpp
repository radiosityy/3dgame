#include "player.h"

void Player::update(Renderer& renderer, float dt)
{
    Object::update(renderer, dt);
}

void Player::walkForward(bool b)
{
    if(b)
    {
        if(m_walking_back)
        {
            m_walking_back = false;
            m_walking_forward = true;
            m_velocity += 2.0f * forward() * m_speed;
            m_velocityP += vec3(0.0f, 0.0f, 2.0f * m_speed);
        }
        else if(!m_walking_forward)
        {
            m_walking_forward = true;
            m_velocity += forward() * m_speed;
            m_velocityP += vec3(0.0f, 0.0f, m_speed);
        }

        if(m_model->forPlayer1("anim_walkf"))
        {
            m_model->playAnimation("anim_walkf");
        }
    }
    else if(m_walking_forward)
    {
        m_walking_forward = false;
        m_velocity -= forward() * m_speed;
        m_velocityP -= vec3(0.0f, 0.0f, m_speed);

        if(m_model->forPlayer2("anim_walkf"))
        {
            m_model->stopAnimation();
        }
    }
}

void Player::walkBack(bool b)
{
    if(b)
    {
        if(m_walking_forward)
        {
            m_walking_forward = false;
            m_walking_back = true;
            m_velocity -= 2.0f * forward() * m_speed;
            m_velocityP -= vec3(0.0f, 0.0f, 2.0f * m_speed);
        }
        else if(!m_walking_back)
        {
            m_walking_back = true;
            m_velocity -= forward() * m_speed;
            m_velocityP -= vec3(0.0f, 0.0f, m_speed);
        }

        if(m_model->forPlayer1("anim_walkb"))
        {
            m_model->playAnimation("anim_walkb");
        }
    }
    else if(m_walking_back)
    {
        m_walking_back = false;
        m_velocity += forward() * m_speed;
        m_velocityP += vec3(0.0f, 0.0f, m_speed);

        if(m_model->forPlayer2("anim_walkb"))
        {
            m_model->stopAnimation();
        }
    }
}

void Player::walkRight(bool b)
{
    if(b)
    {
        if(m_walking_left)
        {
            m_walking_left = false;
            m_walking_right = true;
            m_velocity += 2.0f * m_right * m_speed;
            m_velocityP += vec3(2.0f * m_speed, 0.0f, 0.0f);
        }
        else if(!m_walking_right)
        {
            m_walking_right = true;
            m_velocity += m_right * m_speed;
            m_velocityP += vec3(m_speed, 0.0f, 0.0f);
        }

        if(m_model->forPlayer1("anim_walkr"))
        {
            m_model->playAnimation("anim_walkr");
        }
    }
    else if(m_walking_right)
    {
        m_walking_right = false;
        m_velocity -= m_right * m_speed;
        m_velocityP -= vec3(m_speed, 0.0f, 0.0f);

        if(m_model->forPlayer2("anim_walkr"))
        {
            m_model->stopAnimation();
        }
    }
}

void Player::walkLeft(bool b)
{
    if(b)
    {
        if(m_walking_right)
        {
            m_walking_right = false;
            m_walking_left = true;
            m_velocity -= 2.0f * m_right * m_speed;
            m_velocityP -= vec3(2.0f * m_speed, 0.0f, 0.0f);
        }
        else if(!m_walking_left)
        {
            m_walking_left = true;
            m_velocity -= m_right * m_speed;
            m_velocityP -= vec3(m_speed, 0.0f, 0.0f);
        }

        if(m_model->forPlayer1("anim_walkl"))
        {
            m_model->playAnimation("anim_walkl");
        }
    }
    else if(m_walking_left)
    {
        m_walking_left = false;
        m_velocity += m_right * m_speed;
        m_velocityP += vec3(m_speed, 0.0f, 0.0f);

        if(m_model->forPlayer2("anim_walkl"))
        {
            m_model->stopAnimation();
        }
    }
}

vec3 Player::walkDir() const
{
    return m_walk_dir;
}

void Player::setWalkDir(const vec3& dir)
{
    m_walk_dir = dir;
    m_velocity = m_speed * dir;
}

void Player::walk(const vec3& dir)
{
    setWalkDir(m_walk_dir + dir);
}

void Player::wave()
{
    m_model->playAnimation("anim_wave");
}

void Player::setForward(const vec3& forward)
{
    m_forward = forward;
    const vec3 up = vec3(0.0f, 1.0f, 0.0f);
    m_right = glm::normalize(glm::cross(up, forward));

    glm::mat4x4 rot(
                vec4(m_right, 0.0f),
                vec4(up, 0.0f),
                vec4(forward, 0.0f),
                vec4(0.0f, 0.0f, 0.0f, 1.0f)
                );

    setRot(rot);
}

void Player::stop()
{
    if(m_walking)
    {
        m_velocity -= m_forward * m_speed;
        m_model->stopAnimation();
    }

    m_walking = false;
}

vec3 Player::forward() const
{
    return m_forward;
}

vec3 Player::right() const
{
    return m_right;
}
