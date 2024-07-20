#include "player.h"

float angleBetweenQuats(const quat& q1, const quat& q2)
{
    auto q = normalize(q1 * glm::inverse(q2));
    auto norm = length(vec3(q.x, q.y, q.z));
    if(norm > 1.0f)
    {
        norm = 1.0f;
    }
    return 2.0f * std::asin(norm);
}

void Player::update(Renderer& renderer, float dt)
{
    if(m_walking && m_rotating)
    {
        const auto angle = angleBetweenQuats(m_target_rotation, m_rot);

        m_velocity -= m_moving_direction * speed;

        if(angle > 0.001f)
        {
            const float t = (dt * rot_speed) / angle;

            if(t >= 1.0f)
            {
                m_rot = m_target_rotation;
                m_rotating = false;
            }
            else
            {
                m_rot = glm::slerp(m_rot, m_target_rotation, t);
            }
        }
        else
        {
            m_rot = m_target_rotation;
            m_rotating = false;
        }

        m_moving_direction = normalize(glm::rotate(m_rot, vec3(0.0f, 0.0f, 1.0f)));
        m_velocity += m_moving_direction * speed;
    }

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
            m_velocity += 2.0f * forward() * speed;
        }
        else if(!m_walking_forward)
        {
            m_walking_forward = true;
            m_velocity += forward() * speed;
        }

        if(m_model->forPlayer1("anim_walkf"))
        {
            m_model->playAnimation("anim_walkf");
        }
    }
    else
    {
        if(m_walking_forward)
        {
            m_walking_forward = false;
            m_velocity -= forward() * speed;

            if(m_model->forPlayer2("anim_walkf"))
            {
                m_model->stopAnimation();
            }
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
            m_velocity -= 2.0f * forward() * speed;
        }
        else if(!m_walking_back)
        {
            m_walking_back = true;
            m_velocity -= forward() * speed;
        }

        if(m_model->forPlayer1("anim_walkb"))
        {
            m_model->playAnimation("anim_walkb");
        }
    }
    else
    {
        if(m_walking_back)
        {
            m_walking_back = false;
            m_velocity += forward() * speed;

            if(m_model->forPlayer2("anim_walkb"))
            {
                m_model->stopAnimation();
            }
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
            m_velocity += 2.0f * right() * speed;
        }
        else if(!m_walking_right)
        {
            m_walking_right = true;
            m_velocity += right() * speed;
        }

        if(m_model->forPlayer1("anim_walkr"))
        {
            m_model->playAnimation("anim_walkr");
        }
    }
    else
    {
        if(m_walking_right)
        {
            m_walking_right = false;
            m_velocity -= right() * speed;

            if(m_model->forPlayer2("anim_walkr"))
            {
                m_model->stopAnimation();
            }
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
            m_velocity -= 2.0f * right() * speed;
        }
        else if(!m_walking_left)
        {
            m_walking_left = true;
            m_velocity -= right() * speed;
        }

        if(m_model->forPlayer1("anim_walkl"))
        {
            m_model->playAnimation("anim_walkl");
        }
    }
    else
    {
        if(m_walking_left)
        {
            m_walking_left = false;
            m_velocity += right() * speed;

            if(m_model->forPlayer2("anim_walkl"))
            {
                m_model->stopAnimation();
            }
        }
    }
}

void Player::turnRight(bool b)
{
    if(b)
    {
        if(m_turning_left)
        {
            m_turning_left = false;
            m_turning_right = true;
            m_rot_velocity += 2*rot_speed;
        }
        else if(!m_turning_right)
        {
            m_turning_right = true;
            m_rot_velocity += rot_speed;
        }
    }
    else
    {
        if(m_turning_right)
        {
            m_turning_right = false;
            m_rot_velocity -= rot_speed;
        }
    }
}

void Player::turnLeft(bool b)
{
    if(b)
    {
        if(m_turning_right)
        {
            m_turning_right = false;
            m_turning_left = true;
            m_rot_velocity -= 2*rot_speed;
        }
        else if(!m_turning_left)
        {
            m_turning_left = true;
            m_rot_velocity -= rot_speed;
        }
    }
    else
    {
        if(m_turning_left)
        {
            m_turning_left = false;
            m_rot_velocity += rot_speed;
        }
    }
}

void Player::jump()
{
    setVelocity(velocity() + vec3(0.0f, 6.0f, 0.0f));
//    animateToPose("pose_jump_start");
}

void Player::wave()
{
    m_model->playAnimation("anim_wave");
}

void Player::rotateY(float a)
{
    if(a != 0.0f)
    {
        if(m_walking_forward)
        {
            m_velocity -= forward() * speed;
        }
        else if(m_walking_back)
        {
            m_velocity += forward() * speed;
        }

        if(m_walking_right)
        {
            m_velocity -= right() * speed;
        }
        else if(m_walking_left)
        {
            m_velocity += right() * speed;
        }

        m_rot = normalize(glm::rotate(m_rot, a, vec3(0.0f, 1.0f, 0.0f)));

        if(m_walking_forward)
        {
            m_velocity += forward() * speed;
        }
        else if(m_walking_back)
        {
            m_velocity -= forward() * speed;
        }

        if(m_walking_right)
        {
            m_velocity += right() * speed;
        }
        else if(m_walking_left)
        {
            m_velocity -= right() * speed;
        }

    }
}

void Player::walk(const vec3& direction)
{
    if(!m_walking)
    {
        m_walking = true;

        m_velocity += m_moving_direction * speed;

        if(m_model->forPlayer1("anim_run"))
        {
            m_model->playAnimation("anim_run");
        }
    }

    m_target_rotation = glm::rotation(vec3(0.0f, 0.0f, 1.0f), direction);

    const auto angle = angleBetweenQuats(m_target_rotation, m_rot);
    m_rotating = angle > 0.001f;

    //TODO: the above is an attempt to achieve "realisting" turning, but it's a bit clunky
    //the below effectively makes the player turn instantaneously because it feels better
    //In the future this can be replaced with better turning implementetion (possibly coupled with dedicated turning animation)
    m_rotating = false;
    m_rot = m_target_rotation;
    m_velocity -= m_moving_direction * speed;
    m_moving_direction = normalize(glm::rotate(m_rot, vec3(0.0f, 0.0f, 1.0f)));
    m_velocity += m_moving_direction * speed;
}

void Player::stop()
{
    if(m_walking)
    {
        m_velocity -= m_moving_direction * speed;
        m_model->stopAnimation();
    }

    m_walking = false;
}

vec3 Player::forward() const
{
    return glm::rotate(m_rot, vec3(0.0f, 0.0f, 1.0f));
}

vec3 Player::right() const
{
    return glm::rotate(m_rot, vec3(1.0f, 0.0f, 0.0f));
}
