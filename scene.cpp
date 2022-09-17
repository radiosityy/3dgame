#include "scene.h"
#include <cstring>
#include <cmath>
#include <algorithm>

#include "timer.h"
#include <iostream>
#include <sstream>
#include <fstream>

Scene::Scene(Engine3D& engine3d, float aspect_ratio, const Font& font)
    : m_engine3d(engine3d)
{
    engine3d.loadTexture("Bricks059_2K_Color.png");
    engine3d.loadNormalMap("Bricks059_2K_Normal.png");
    m_time_label = std::make_unique<Label>(m_engine3d, 0, 20.0f, font, "", false, HorizontalAlignment::Left, VerticalAlignment::Top);
    m_time_label->setUpdateCallback([&](Label& label)
    {
        const uint32_t h = static_cast<uint32_t>(m_time_of_day) / 3600;
        const uint32_t m = (static_cast<uint32_t>(m_time_of_day) - h * 3600) / 60;
        const uint32_t s = static_cast<uint32_t>(m_time_of_day) - h*3600 - m*60;

        std::stringstream ss;
        ss << "Time: " << h << ":" << m << ":" << s;
        label.setText(ss.str());
    });

    m_camera = std::make_unique<Camera>(aspect_ratio, 0.1f, 5000.0f, degToRad(90.0f));

    /*add sun*/
    m_sun.shadow_map_count = 4;
    m_sun.shadow_map_res_x = 2048;
    m_sun.shadow_map_res_y = 2048;
    engine3d.addDirLight(m_sun);

    m_time_of_day = 11.0f * 3600.0f;
    updateSun();

    loadFromFile("scene.scn");
}

void Scene::loadFromFile(std::string_view filename)
{
    std::ifstream scene_file(filename.data(), std::ios::binary);

    if(!scene_file)
    {
        std::cout << "No scene.scn file found. Creating new scene..." << std::endl;
    }
    else
    {
        uint64_t obj_count = 0;
        scene_file.read(reinterpret_cast<char*>(&obj_count), sizeof(uint64_t));

        for(uint64_t i = 0; i < obj_count; i++)
        {
            addObject(std::make_unique<Object>(m_engine3d, scene_file));
        }

        uint64_t point_light_count = 0;
        scene_file.read(reinterpret_cast<char*>(&point_light_count), sizeof(uint64_t));

        for(uint64_t i = 0; i < point_light_count; i++)
        {
            PointLight point_light(scene_file);

#if !EDITOR_ENABLE
            addPointLight(point_light);
#else
            addStaticPointLight(point_light);
#endif
        }
    }

    /*add sky dome*/
    auto sky_dome = std::make_unique<Object>(m_engine3d, "assets/meshes/sky_dome.3d", RenderMode::Sky, vec3(0.0f), vec3(4000.0f));
#if EDITOR_ENABLE
    sky_dome->setSerializable(false);
#endif
    addObject(std::move(sky_dome));

    /*add player*/
    auto player = std::make_unique<Player>(m_engine3d, "assets/meshes/man.3d", RenderMode::Default, vec3(0.0f, 5.0f, -10.0f));
#if EDITOR_ENABLE
    player->setSerializable(false);
#endif
    m_player = player.get();
    addObject(std::move(player));

    /*add terrain*/
    m_terrain = std::make_unique<Terrain>(m_engine3d);
}

void Scene::update(float dt, const InputState& input_state) noexcept
{
    for(auto& obj : m_objects)
    {
        obj->update(m_engine3d, dt);
    }

    /*advance time of day (wrap around after a whole day has passed)*/
    m_time_of_day += dt * m_time_scale;

    if(m_time_of_day >= m_day_length)
    {
        m_time_of_day -= m_day_length;
    }

    m_time_of_day = 11.0f * 3600.0f;

    updateSun();

    m_time_label->update(m_engine3d, dt);

    if(m_player_movement)
    {
        playerControls(input_state);

        float dh = 5.0f;
        AABB aabb = m_player->aabbs()[0];
        vec3 player_velocity = m_player->velocity() + m_player->acceleration() * dt;
        vec3 s = player_velocity * dt;
        aabb.transform(m_player->pos() + s, m_player->scale());
        const auto res  = m_terrain->collision(aabb, dh);
        switch (res)
        {
        case CollisionResult::Collision:
            if(player_velocity.y < 0.0f)
            {
                m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
            }
            player_velocity.y = 0.0f;
            return;
        case CollisionResult::ResolvedCollision:
            if(player_velocity.y < 0.0f)
            {
                m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
            }
            player_velocity.y = 0.0f;
            std::cout << "resolved " << dh << std::endl;
            s.y = dh;
            m_player->move(s);
            break;
        case CollisionResult::Airborne:
            std::cout << "airborne" << std::endl;
            //first, if player has no vertical acceleration, check if the player is airborne
            if(m_player->acceleration().y == 0.0f)
            {
//                if(!objectMoveCollision(m_player, vec3(0.0f, -0.001f, 0.0f)))
                {
                    m_player->setAcceleration(vec3(0.0f, -9.8f, 0.0f));
                }
            }
            m_player->move(s);
            break;
        }

        m_player->setVelocity(player_velocity);

#if 0
        //TODO:should we rotate before translation or after?
        if(m_player->rotVelocity() != 0.0f)
        {
            tryPlayerRotation(dt);
        }

        tryPlayerMove(dt);
#endif

        const vec3 player_camera_reference_point = m_player->pos() + vec3(0.0f, 5.0f, 0.0f);

        m_camera->setPos(player_camera_reference_point + sphericalToCartesian(m_player_camera_radius, m_player_camera_theta, m_player_camera_phi));

        const vec3 cam_forward = normalize(player_camera_reference_point - m_camera->pos());
        const vec3 cam_right = cross(vec3(0.0f, 1.0f, 0.0f), cam_forward);
        const vec3 cam_up = cross(cam_forward, cam_right);

        m_camera->setBasis(cam_forward, cam_up, cam_right);
    }
    else
    {
        const float cam_speed = input_state.keyboard[VKeyLShift] ? 42.0f : 14.0f;

        if(input_state.keyboard[VKeyW])
        {
            m_camera->walk(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyS])
        {
            m_camera->walk(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyA])
        {
            m_camera->strafe(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeyD])
        {
            m_camera->strafe(dt*cam_speed);
        }

        if(input_state.keyboard[VKeyC])
        {
            m_camera->tilt(-dt*cam_speed);
        }

        if(input_state.keyboard[VKeySpace])
        {
            m_camera->tilt(dt*cam_speed);
        }
    }
}

void Scene::draw() noexcept
{
    for(auto& obj : m_objects)
    {
        obj->draw(m_engine3d);
    }

    m_terrain->draw(m_engine3d, *m_camera);

    m_time_label->draw(m_engine3d);
}

void Scene::tryPlayerRotation(float dt)
{
    const float rot_angle = m_player->rotVelocity() * dt;
    const quat rot = normalize(rotate(m_player->rot(), rot_angle, vec3(0.0f, 1.0f, 0.0f)));

    if(!objectRotateCollision(m_player, rot))
    {
        m_player->rotateY(rot_angle);
    }
}

void Scene::tryPlayerMove(float dt)
{
    const float xd = 0.5f;
    vec3 player_velocity = m_player->velocity() + m_player->acceleration() * dt;

    for(uint8_t axis = 0; axis < 3; axis++)
    {
        vec3 s = {0.0f, 0.0f, 0.0f};
        s[axis] = player_velocity[axis] * dt;

        if(axis != 1)
        {
            bool collision = false;

            while(objectMoveCollision(m_player, s))
            {
                s.y += 0.05f;

                if(s.y > xd)
                {
                    collision = true;
                    break;
                }
            }

            if(!collision)
            {
                m_player->move(s);
            }
        }
        else if(!objectMoveCollision(m_player, s))
        {
            m_player->move(s);
        }
        else
        {
            if(player_velocity.y < 0.0f)
            {
                m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
            }

            player_velocity.y = 0.0f;
        }
    }

    m_player->setVelocity(player_velocity);
}

bool Scene::objectRotateCollision(const Object* object, const quat& rot)
{
    //TODO: what if object has AABBs?
    for(auto bb : object->bbs())
    {
        bb.transform(object->pos(), rot, object->scale());

        if(objectCollision(object, bb))
        {
            return true;
        }
    }

    return false;
}

bool Scene::objectMoveCollision(const Object* obj, vec3 s)
{
    for(Sphere sphere : obj->spheres())
    {
        sphere.transform(obj->pos() + s, obj->scale());

        if(objectCollision(obj, sphere))
        {
            return true;
        }
    }

    //TODO: what if there's rotation?
    for(AABB aabb : obj->aabbs())
    {
        aabb.transform(obj->pos() + s, obj->scale());

        if(objectCollision(obj, aabb))
        {
            return true;
        }
    }

    for(BoundingBox bb : obj->bbs())
    {
        bb.transform(obj->pos() + s, obj->rot(), obj->scale());

        if(objectCollision(obj, bb))
        {
            return true;
        }
    }

    return false;
}

void Scene::updateSun()
{
    const float t = m_time_of_day / m_day_length;

    const float azimuth = pi_2 - 2.0f * pi * t;
    float inclination = pi_2 - std::asin(std::sin(m_lat) * std::sin(m_decl) + std::cos(m_lat) * std::cos(m_decl) * std::cos(pi / 12.0f * ((m_time_of_day - m_lst_offset) / 3600.0f)));

    m_visual_sun_pos = vec3(std::cos(azimuth) * std::sin(inclination),
                        std::cos(inclination),
                        std::sin(azimuth) * std::sin(inclination));

    if(m_visual_sun_pos.y < m_sun_radius)
    {
        const float alpha = std::acos(std::clamp(-m_visual_sun_pos.y/m_sun_radius, -1.0f, 1.0f));
        const float visible_sun_area = (2.0f*alpha - std::sin(2.0f*alpha)) / (2.0f * pi);

        inclination -= (4.0f * m_sun_radius * std::pow(std::sin(alpha), 3.0f)) / (3.0f * (2.0f*alpha - std::sin(2.0f*alpha)));

        m_effective_sun_pos = vec3(std::cos(azimuth) * std::sin(inclination),
                                   std::cos(inclination),
                                   std::sin(azimuth) * std::sin(inclination));

        m_sun.color = vec3(0.5f) * visible_sun_area;
    }
    else
    {
        m_effective_sun_pos = m_visual_sun_pos;
        m_sun.color = vec3(0.5f);
    }

    m_sun.dir = -m_effective_sun_pos;

    m_engine3d.updateDirLight(0, m_sun);
}

template<class Collider>
bool Scene::objectCollision(const Object* object, const Collider& collider)
{
    for(const auto& obj : m_objects)
    {
        if(obj.get() == object)
        {
            continue;
        }

        for(Sphere sphere : obj->spheres())
        {
            sphere.transform(obj->pos(), obj->scale());

            if(intersect(collider, sphere))
            {
                return true;
            }
        }

        for(AABB aabb : obj->aabbs())
        {
            aabb.transform(obj->pos(), obj->scale());

            if(intersect(collider, aabb))
            {
                return true;
            }
        }

        for(BoundingBox bb : obj->bbs())
        {
            bb.transform(obj->pos(), obj->rot(), obj->scale());

            if(intersect(collider, bb))
            {
                return true;
            }
        }
    }

    return false;
}

void Scene::onWindowResize(float aspect_ratio) noexcept
{
    m_camera->setAspectRatio(aspect_ratio);
}

Camera& Scene::camera() noexcept
{
    return *m_camera;
}

void Scene::addObject(std::unique_ptr<Object>&& object)
{
    m_objects.push_back(std::move(object));
}

void Scene::removeObject(Object* obj)
{
    for(auto it = m_objects.begin(); it != m_objects.end(); it++)
    {
        if(it->get() == obj)
        {
            m_objects.erase(it);
            return;
        }
    }
}

PointLightId Scene::addPointLight(const PointLight& point_light) const
{
    return m_engine3d.addPointLight(point_light);
}

void Scene::updatePointLight(PointLightId id, const PointLight& point_light) const
{
    m_engine3d.updatePointLight(id, point_light);
}

RenderData Scene::renderData() noexcept
{
    RenderData render_data;
    render_data.visual_sun_pos = m_visual_sun_pos;
    render_data.effective_sun_pos = m_effective_sun_pos;
    render_data.sun_radius = m_sun_radius;

    return render_data;
}

Terrain& Scene::terrain() noexcept
{
    return *m_terrain;
}

void Scene::playerControls(const InputState& input_state)
{
    vec3 v = vec3(0.0f, 0.0f, 0.0f);

    vec3 forward = m_camera->forward();
    forward.y = 0;

    vec3 right = m_camera->right();
    right.y = 0;

    bool walk = false;

    if(input_state.keyboard[VKeyW] && !input_state.keyboard[VKeyS])
    {
        v += forward;
        walk = true;
    }
    else if(input_state.keyboard[VKeyS] && !input_state.keyboard[VKeyW])
    {
        v -= forward;
        walk = true;
    }

    if(input_state.keyboard[VKeyD] && !input_state.keyboard[VKeyA])
    {
        v += right;
        walk = true;
    }
    else if(input_state.keyboard[VKeyA] && !input_state.keyboard[VKeyD])
    {
        v -= right;
        walk = true;
    }

    if(walk)
    {
        m_player->walk(normalize(v));
    }
    else
    {
        m_player->stop();
    }
}

/*--- event handler ---*/

void Scene::onInputEvent(const Event& event, const InputState& input_state)
{
    if(event.event == EventType::MouseMoved)
    {
        if(!m_player_movement)
        {
            m_camera->rotate(event.cursor_delta.x * 0.005f);
            m_camera->pitch(event.cursor_delta.y * 0.005f);
        }
        else
        {
            m_player_camera_phi -= event.cursor_delta.x * 0.01f;
            if(m_player_camera_phi >= 2.0f * pi)
            {
                m_player_camera_phi -= 2.0f * pi;
            }
            else if(m_player_camera_phi <= 0.0f)
            {
                m_player_camera_phi += 2.0f * pi;
            }

            m_player_camera_theta = std::clamp(m_player_camera_theta - event.cursor_delta.y * 0.01f, 0.001f, pi - 0.001f);
        }
    }
    else if(event.event == EventType::MouseScrolledUp)
    {
        m_player_camera_radius -= 1.0f;

        if(m_player_camera_radius < 3.0f)
        {
            m_player_camera_radius = 3.0f;
        }
    }
    else if(event.event == EventType::MouseScrolledDown)
    {
        m_player_camera_radius += 1.0f;

        if(m_player_camera_radius > 10.0f)
        {
            m_player_camera_radius = 10.0f;
        }
    }

    if(event.event == EventType::KeyPressed)
    {
        if(event.key == VKeyM)
        {
            m_player_movement = !m_player_movement;
        }
        else if(event.key == VKeyF)
        {
            m_player->wave();
        }
        else if(event.key == VKeyR)
        {
            m_player->setPos(vec3(0.0f, 5.0f, -10.0f));
            m_player->walkBack(false);
            m_player->walkForward(false);
            m_player->walkRight(false);
            m_player->walkLeft(false);
            m_player->setVelocity(vec3(0.0f, 0.0f, 0.0f));
            m_player->setAcceleration(vec3(0.0f, 0.0f, 0.0f));
        }
        else if(event.key == VKeySpace)
        {
            if(m_player->velocity().y == 0)
            {
                m_player->jump();
            }
        }
    }
}

#if EDITOR_ENABLE

Object* Scene::pickObject(const Ray& rayW, float& d)
{
    d = std::numeric_limits<float>::max();
    Object* closest_obj = nullptr;

    for(const auto& obj : m_objects)
    {
        float _d;
        if(obj->rayIntersetion(rayW, d, _d))
        {
            d = std::min(d, _d);
            closest_obj = obj.get();
        }
    }

    return closest_obj;
}

void Scene::addStaticPointLight(const PointLight& point_light)
{
    m_static_point_light_ids.push_back(addPointLight(point_light));
    m_static_point_lights.push_back(point_light);
}

void Scene::removeStaticPointLight(uint32_t id)
{
    m_engine3d.removePointLight(m_static_point_light_ids[id]);
    m_static_point_lights.erase(m_static_point_lights.begin() + id);
    m_static_point_light_ids.erase(m_static_point_light_ids.begin() + id);
}

void Scene::updateStaticPointLight(uint32_t id, const PointLight& point_light)
{
    m_static_point_lights[id] = point_light;
    updatePointLight(m_static_point_light_ids[id], point_light);
}

std::vector<PointLight>& Scene::staticPointLights()
{
    return m_static_point_lights;
}

void Scene::serialize(std::string_view filename) const
{
    std::ofstream outfile(filename.data(), std::ios::binary);

    if(!outfile)
    {
        throw std::runtime_error(std::string("Failed to open file: ") + filename.data());
    }

    uint64_t obj_count = 0;

    for(const auto& obj : m_objects)
    {
        if(obj->isSerializable())
        {
            obj_count++;
        }
    }

    outfile.write(reinterpret_cast<const char*>(&obj_count), sizeof(uint64_t));

    for(const auto& obj : m_objects)
    {
        if(obj->isSerializable())
        {
            obj->serialize(outfile);
        }
    }

    const uint64_t point_light_count = m_static_point_lights.size();
    outfile.write(reinterpret_cast<const char*>(&point_light_count), sizeof(uint64_t));

    for(const auto& point_light : m_static_point_lights)
    {
        point_light.serialize(outfile);
    }
}

#endif
